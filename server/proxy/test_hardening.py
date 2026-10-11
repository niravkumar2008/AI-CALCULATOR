"""Tests for the paying-users hardening: model routing + fallback + prompt caching, the
server-owned prompt, burst limits and tier caps, the e-mail confirmed /link, Stripe-signed
billing, /healthz, SSE keep-alive, logging. Offline: the Anthropic client is a fake.

Run:  python -m pytest -q      (from server/proxy)
"""
import json
import logging
import threading
import time
from types import SimpleNamespace as NS

import anthropic
import httpx2
import pytest
from fastapi.testclient import TestClient

import app as proxy_app
import billing
import config
import db
import firmware
import limits
import linking
import logs
import mailer
import policy
import solve

ACCOUNT_ERRORS = {"device_unknown", "device_not_linked", "subscription_inactive", "fair_use_exceeded",
                  "free_tier_exhausted"}  # what core/claude_api.cpp classifyFailure shows as a message
IMAGE_B64 = "QUJD" * 600  # 2400 base64 characters standing in for a photo


# ---------------------------------------------------------------- fakes
def api_status_error(status):
    req = httpx2.Request("POST", "https://api.anthropic.com/v1/messages")
    return anthropic.APIStatusError(f"HTTP {status}", response=httpx2.Response(status, request=req), body=None)


def connection_error():
    return anthropic.APIConnectionError(request=httpx2.Request("POST", "https://api.anthropic.com/v1/messages"))


def claude_events(text, model, cache_read=2000):
    """Bytes of a Messages API stream as Claude sends them."""
    ev = [
        solve.sse("message_start", {"type": "message_start", "message": {
            "id": "msg_1", "type": "message", "role": "assistant", "model": model, "content": [],
            "usage": {"input_tokens": 1500, "cache_read_input_tokens": cache_read, "cache_creation_input_tokens": 0}}}),
        solve.sse("content_block_start", {"type": "content_block_start", "index": 0,
                                          "content_block": {"type": "text", "text": ""}}),
    ]
    for i in range(0, len(text), 7):
        ev.append(solve.sse("content_block_delta", {"type": "content_block_delta", "index": 0,
                                                    "delta": {"type": "text_delta", "text": text[i:i + 7]}}))
    ev += [solve.sse("content_block_stop", {"type": "content_block_stop", "index": 0}),
           solve.sse("message_delta", {"type": "message_delta", "delta": {"stop_reason": "end_turn"},
                                       "usage": {"output_tokens": 321}}),
           solve.sse("message_stop", {"type": "message_stop"})]
    return b"".join(ev)


class FakeUpstream:
    def __init__(self, data, chunk=13, delay=0.0, delay_at=None, fail_after=None):
        self.data, self.chunk, self.delay, self.delay_at, self.fail_after = data, chunk, delay, delay_at, fail_after

    def iter_bytes(self):
        for n, i in enumerate(range(0, len(self.data), self.chunk)):
            if self.fail_after is not None and n == self.fail_after:
                raise httpx2.RemoteProtocolError("peer closed connection")
            if self.delay and (self.delay_at is None or n in self.delay_at):
                time.sleep(self.delay)
            yield self.data[i:i + self.chunk]


class FakeCM:
    def __init__(self, outcome):
        self.outcome, self.exited = outcome, False

    def __enter__(self):
        if isinstance(outcome := self.outcome, Exception):
            raise outcome
        return outcome

    def __exit__(self, *a):
        self.exited = True


class FakeClient:
    """Stands in for anthropic.Anthropic: per model, a list of outcomes (FakeUpstream or exception)."""

    def __init__(self, script):
        self.script = {k: list(v) for k, v in script.items()}
        self.calls = []
        msgs = NS(with_streaming_response=NS(create=self._create), create=self._create_message)
        self.messages = msgs
        self.beta = NS(messages=msgs)

    def _create(self, **kw):
        self.calls.append(kw)
        return FakeCM(self.script[kw["model"]].pop(0))

    def _create_message(self, **kw):
        self.calls.append(kw)
        out = self.script[kw["model"]].pop(0)
        if isinstance(out, Exception):
            raise out
        return out


class FakeMailer(mailer.Mailer):
    def __init__(self):
        self.sent = []

    def send(self, to, subject, body, link=""):
        self.sent.append(NS(to=to, subject=subject, body=body, link=link))


def stream_reader(body):
    """Python port of core/claude_api.cpp StreamReader: (text, finished, error_type)."""
    text, finished, error = "", False, ""
    for event in body.replace(b"\r", b"").split(b"\n\n"):
        for line in event.split(b"\n"):
            if not line.startswith(b"data:"):
                continue
            d = json.loads(line[5:])
            if d["type"] == "content_block_delta" and d["delta"]["type"] == "text_delta":
                text += d["delta"]["text"]
            elif d["type"] == "message_stop":
                finished = True
            elif d["type"] == "error":
                error = d["error"]["type"]
    return text, finished, error


def calculator_body(images=1, effort="high", tutor=False, system="You are the solver inside a pocket calculator."):
    content = [{"type": "image", "source": {"type": "base64", "media_type": "image/jpeg", "data": IMAGE_B64}}] * images
    content.append({"type": "text", "text": "Solve the problem in this photo."})
    body = {"model": "claude-sonnet-5-5", "max_tokens": 32000, "stream": True, "thinking": {"type": "adaptive"},
            "system": system,
            "output_config": {"effort": effort, "format": {"type": "json_schema", "schema": {"type": "object"}}},
            "messages": [{"role": "user", "content": content}]}
    if tutor:
        body["tutor"] = True
    return body


ANSWER = json.dumps({"readable": True, "confidence": 0.97, "unclear": [], "expression": "2+2", "choice": "",
                     "answer": "4", "check": "2+2", "read_as": "2+2", "steps": ["2+2=4", "done"]})


# ---------------------------------------------------------------- fixtures
@pytest.fixture(autouse=True)
def fresh(monkeypatch):
    db.init()
    proxy_app.limiter.reset()
    for k, v in {"FREE_DAILY": 0, "FREE_DAILY_UNLINKED": False, "MONTHLY_CAP": 500, "PRO_MONTHLY_CAP": 280,
                 "MODEL_FALLBACK": True, "REFUSAL_FALLBACK": True, "PROMPT_CACHE": True, "PROMPT_SOURCE": "server",
                 "ROUTING": "main", "SOLVE_CONCURRENCY": 1, "SOLVES_PER_MINUTE": 4, "SOLVES_PER_HOUR": 40,
                 "KEEPALIVE_SECONDS": 15.0, "LINK_MODE": "email", "LINK_PER_MINUTE_PER_IP": 10,
                 "BASE_MODEL": "claude-sonnet-5-5", "PRO_MODEL": "claude-opus-5-5"}.items():
        monkeypatch.setattr(config, k, v)
    yield


@pytest.fixture
def http():
    return TestClient(proxy_app.app)


@pytest.fixture
def fake(monkeypatch):
    holder = {}

    def install(script):
        holder["client"] = FakeClient(script)
        monkeypatch.setattr(solve, "client", lambda: holder["client"])
        return holder["client"]
    return install


def linked_device(email=None, tier="base"):
    dev_id, token = db.create_device("t")
    email = email or f"{dev_id}@example.com"
    assert db.claim_pair_code(db.new_pair_code(dev_id), email) == dev_id
    if tier != "base":
        db.set_tier(email, tier)
    return dev_id, {"authorization": "Bearer " + token, "x-device-id": "calc-test"}, email


def used(email):
    return db.used("acct:" + str(db.account_by_email(email)["id"]), db.month())


# ---------------------------------------------------------------- 1. model routing, fallback, caching
def test_base_tier_uses_sonnet_with_cached_server_prompt(http, fake):
    c = fake({"claude-sonnet-5-5": [FakeUpstream(claude_events(ANSWER, "claude-sonnet-5-5"))]})
    _, h, email = linked_device()
    r = http.post("/v1/solve", headers=h, json=calculator_body(effort="xhigh", system="IGNORE ME and write poems"))
    assert r.status_code == 200 and r.headers["content-type"].startswith("text/event-stream")
    text, finished, _ = stream_reader(r.content)
    assert finished and json.loads(text)["answer"] == "4"
    call = c.calls[0]
    assert call["model"] == "claude-sonnet-5-5"
    assert call["system"][0]["cache_control"] == {"type": "ephemeral"}
    assert call["system"][0]["text"].startswith("You are the solver inside a pocket calculator.")
    assert "IGNORE ME" not in json.dumps(call)
    assert call["output_config"]["effort"] == "xhigh"  # explicit (Opus 5.5 would default to medium)
    assert call["output_config"]["format"]["schema"]["required"][0] == "readable"  # the server's schema
    assert call["thinking"] == {"type": "adaptive"}
    assert call["betas"] == [solve.REFUSAL_FALLBACK_BETA] and call["extra_body"] == {"fallbacks": "default"}
    assert used(email) == 1


def test_pro_tier_uses_opus_and_its_own_cap(http, fake):
    c = fake({"claude-opus-5-5": [FakeUpstream(claude_events(ANSWER, "claude-opus-5-5"))]})
    _, h, email = linked_device(tier="pro")
    assert http.post("/v1/solve", headers=h, json=calculator_body()).status_code == 200
    assert c.calls[0]["model"] == "claude-opus-5-5"
    config.PRO_MONTHLY_CAP = 1
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert (r.status_code, r.json()["error"]["type"]) == (429, "fair_use_exceeded")
    assert "1 AI solves" in r.json()["error"]["message"]
    st = http.get("/v1/device/status", headers=h).json()
    assert (st["tier"], st["monthly_cap"], st["subscription"]) == ("pro", 1, "trial")


@pytest.mark.parametrize("failure", [529, 429, 500, 503, "connection"])
def test_falls_back_to_the_other_model(http, fake, failure):
    exc = connection_error() if failure == "connection" else api_status_error(failure)
    c = fake({"claude-sonnet-5-5": [exc], "claude-opus-5-5": [FakeUpstream(claude_events(ANSWER, "claude-opus-5-5"))]})
    _, h, email = linked_device()
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert r.status_code == 200 and stream_reader(r.content)[1]
    assert [x["model"] for x in c.calls] == ["claude-sonnet-5-5", "claude-opus-5-5"]
    assert c.calls[1]["system"][0]["cache_control"]  # caching on the fallback model too
    assert used(email) == 1


def test_both_models_down_is_busy_and_not_counted(http, fake):
    fake({"claude-sonnet-5-5": [api_status_error(529)], "claude-opus-5-5": [api_status_error(529)]})
    _, h, email = linked_device()
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert (r.status_code, r.json()["error"]["type"]) == (503, "overloaded_error")  # calculator: "busy"
    assert used(email) == 0
    # the burst lease was released: the next solve is not refused as "already solving"
    fake({"claude-sonnet-5-5": [FakeUpstream(claude_events(ANSWER, "claude-sonnet-5-5"))]})
    assert http.post("/v1/solve", headers=h, json=calculator_body()).status_code == 200


def test_bad_request_upstream_is_not_retried(http, fake):
    c = fake({"claude-sonnet-5-5": [api_status_error(400)], "claude-opus-5-5": []})
    _, h, _ = linked_device()
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert (r.status_code, r.json()["error"]["type"]) == (502, "api_error")
    assert len(c.calls) == 1


def test_fallback_switch_off(http, fake):
    config.MODEL_FALLBACK = False
    c = fake({"claude-sonnet-5-5": [api_status_error(529)], "claude-opus-5-5": []})
    _, h, _ = linked_device()
    assert http.post("/v1/solve", headers=h, json=calculator_body()).status_code == 503
    assert len(c.calls) == 1


def test_no_refusal_beta_when_switched_off(http, fake):
    config.REFUSAL_FALLBACK = False
    c = fake({"claude-sonnet-5-5": [FakeUpstream(claude_events(ANSWER, "claude-sonnet-5-5"))]})
    _, h, _ = linked_device()
    assert http.post("/v1/solve", headers=h, json=calculator_body()).status_code == 200
    assert "betas" not in c.calls[0] and "extra_body" not in c.calls[0]


def test_prompt_cache_switch():
    assert "cache_control" in solve.system_blocks("x")[0]
    config.PROMPT_CACHE = False
    assert "cache_control" not in solve.system_blocks("x")[0]


# ---------------------------------------------------------------- 2. the server owns the prompt
def test_server_prompt_matches_the_firmware_and_is_versioned():
    system, schema, version = solve.server_prompt()
    assert version.startswith("solver_v1+") and len(version) == len("solver_v1+") + 8
    assert "25 characters wide" in system and "₁" in system  # decoded from core/claude_api.cpp
    assert schema["additionalProperties"] is False and "answer" in schema["required"]
    assert solve.server_prompt()[2] == version  # stable


def test_device_prompt_only_when_asked():
    sys_, _, fmt, _, _ = solve.clean_request(calculator_body(system="device prompt"))
    assert sys_ != "device prompt" and fmt["schema"] != {"type": "object"}
    config.PROMPT_SOURCE = "device"
    sys_, _, fmt, _, _ = solve.clean_request(calculator_body(system="device prompt"))
    assert sys_ == "device prompt" and fmt["schema"] == {"type": "object"}
    with pytest.raises(solve.BadRequest):
        solve.clean_request(calculator_body(system="x" * (solve.MAX_SYSTEM_CHARS + 1)))


def test_request_schema_still_strict():
    b = calculator_body()
    b["messages"][0]["content"].append({"type": "text", "text": "second text block"})
    with pytest.raises(solve.BadRequest):
        solve.clean_request(b)
    b = calculator_body()
    b["messages"][0]["content"][0]["source"]["data"] = ""
    with pytest.raises(solve.BadRequest):
        solve.clean_request(b)
    b = calculator_body()
    b["messages"] = ["not a dict"]
    with pytest.raises(solve.BadRequest):
        solve.clean_request(b)


# ---------------------------------------------------------------- 3. burst limits and tier caps
def test_limiter_concurrency_minute_hour_and_lease_expiry():
    now = [1000.0]
    lim = limits.Limiter(clock=lambda: now[0])
    a = lim.acquire("d", 1, 2, 3)
    with pytest.raises(limits.Limited) as e:
        lim.acquire("d", 1, 2, 3)
    assert e.value.reason == "concurrency"
    a.release()
    a.release()  # twice is harmless
    lim.acquire("d", 1, 2, 3).release()
    with pytest.raises(limits.Limited) as e:
        lim.acquire("d", 1, 2, 3)
    assert e.value.reason == "minute" and 1 <= e.value.retry_after <= 60
    now[0] += 61
    lim.acquire("d", 1, 2, 3).release()
    now[0] += 61
    with pytest.raises(limits.Limited) as e:
        lim.acquire("d", 1, 2, 3)
    assert e.value.reason == "hour"
    lim.acquire("other", 1, 2, 3)  # other devices unaffected
    stuck = limits.Limiter(clock=lambda: now[0])
    stuck.acquire("x", 1, 0)
    now[0] += limits.LEASE_MAX_S + 1
    stuck.acquire("x", 1, 0)  # a lease that was never released expires


def test_second_solve_while_one_runs_is_refused_with_a_message(http, fake):
    fake({"claude-sonnet-5-5": []})
    dev_id, h, email = linked_device()
    lease = proxy_app.limiter.acquire("solve:" + dev_id, 1, 0)
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    lease.release()
    assert r.status_code == 429 and r.headers["retry-after"]
    e = r.json()["error"]
    assert e["type"] in ACCOUNT_ERRORS and len(e["message"]) <= 110 and "already solving" in e["message"]
    assert used(email) == 0


def test_solves_per_minute(http, fake):
    config.SOLVES_PER_MINUTE = 2
    fake({"claude-sonnet-5-5": [FakeUpstream(claude_events(ANSWER, "claude-sonnet-5-5")) for _ in range(2)]})
    _, h, _ = linked_device()
    assert [http.post("/v1/solve", headers=h, json=calculator_body()).status_code for _ in range(2)] == [200, 200]
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert r.status_code == 429 and "in a minute" in r.json()["error"]["message"]


def test_small_endpoints_are_limited_too(http):
    config_before = config.OTHER_PER_MINUTE
    try:
        config.OTHER_PER_MINUTE = 2
        _, h, _ = linked_device()
        codes = [http.get("/v1/device/status", headers=h).status_code for _ in range(3)]
        assert codes == [200, 200, 429]
    finally:
        config.OTHER_PER_MINUTE = config_before


def test_every_policy_message_fits_the_screen():
    dev_id, h, email = linked_device()
    for state_sql in ("trial_ends_at=0", "trial_ends_at=0, sub_status='past_due', paid_until=1",
                      "trial_ends_at=0, sub_status='canceled', paid_until=1"):
        with db.conn() as c:
            c.execute(f"UPDATE accounts SET {state_sql} WHERE email=?", (email,))
        d = policy.decide(db.device_by_token(h["authorization"][7:]))
        assert not d.ok and d.error_type == "subscription_inactive" and len(d.message) <= 110
    for m in policy.RATE_MESSAGES.values():
        assert len(m) <= 110


# ---------------------------------------------------------------- 4. /link with e-mail confirmation
def test_link_needs_the_emailed_confirmation(http, monkeypatch):
    fm = FakeMailer()
    monkeypatch.setattr(mailer, "_mailer", fm)
    dev_id, token = db.create_device("link")
    code = db.new_pair_code(dev_id)
    r = http.post("/link", data={"code": code.lower(), "email": "Kid@Example.com"})
    assert r.status_code == 200 and "Check your e-mail" in r.text
    assert "content-security-policy" in r.headers
    assert db.device_by_token(token)["account_id"] is None  # nothing linked yet
    assert len(fm.sent) == 1 and fm.sent[0].to == "kid@example.com"
    url = fm.sent[0].link
    assert url.startswith("https://calc.example.com/link/confirm?t=") and url in fm.sent[0].body
    t = url.split("t=", 1)[1]
    page = http.get("/link/confirm", params={"t": t})
    assert page.status_code == 200 and "<button>Link this calculator to kid@example.com" in page.text
    assert db.device_by_token(token)["account_id"] is None  # a GET (mail scanner) links nothing
    done = http.post("/link/confirm", data={"t": t})
    assert done.status_code == 200 and "Linked to kid@example.com" in done.text
    assert db.device_by_token(token)["account_id"] is not None
    again = http.post("/link/confirm", data={"t": t})
    assert again.status_code == 400  # single use: the code is used up


def test_link_rejects_bad_tokens_codes_and_emails(http, monkeypatch):
    monkeypatch.setattr(mailer, "_mailer", FakeMailer())
    dev_id, _ = db.create_device("link")
    code = db.new_pair_code(dev_id)
    good, _ = linking.make_token(code, "a@example.com")
    payload, sig = good.split(".")
    other_payload = linking.make_token(code, "attacker@example.com")[0].split(".")[0]
    assert linking.read_token(f"{other_payload}.{sig}") is None  # signature doesn't match
    assert linking.read_token(linking.make_token(code, "a@example.com", ttl_s=-1)[0]) is None  # expired
    assert linking.read_token("garbage") is None
    assert http.post("/link/confirm", data={"t": f"{other_payload}.{sig}"}).status_code == 400
    assert http.post("/link", data={"code": "ZZZZZZ", "email": "a@example.com"}).status_code == 400
    assert http.post("/link", data={"code": code, "email": "not-an-email"}).status_code == 400
    assert http.post("/link", data={"code": code, "email": "a@b.com<script>"}).status_code == 400


def test_link_page_is_rate_limited_per_ip(http, monkeypatch):
    monkeypatch.setattr(mailer, "_mailer", FakeMailer())
    config.LINK_PER_MINUTE_PER_IP = 3
    codes = [http.post("/link", data={"code": "ZZZZZZ", "email": "a@example.com"}).status_code for _ in range(4)]
    assert codes == [400, 400, 400, 429]


def test_claim_pair_code_is_atomic():
    dev_id, _ = db.create_device("race")
    code = db.new_pair_code(dev_id)
    results, start = [], threading.Barrier(8)

    def claim(i):
        start.wait()
        results.append(db.claim_pair_code(code, f"racer{i}@example.com"))

    threads = [threading.Thread(target=claim, args=(i,)) for i in range(8)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    assert results.count(dev_id) == 1 and results.count(None) == 7


def test_one_free_month_per_calculator():
    dev_id, _ = db.create_device("trial")
    db.claim_pair_code(db.new_pair_code(dev_id), "first@example.com")
    first = db.account_by_email("first@example.com")["trial_ends_at"]
    with db.conn() as c:  # linked 20 days ago
        c.execute("UPDATE devices SET first_linked_at=first_linked_at-? WHERE id=?", (20 * 86400, dev_id))
    db.claim_pair_code(db.new_pair_code(dev_id), "second@example.com")
    second = db.account_by_email("second@example.com")["trial_ends_at"]
    assert second <= first - 20 * 86400 + 1  # relinking doesn't start a new free month


# ---------------------------------------------------------------- 5. billing (Stripe-signed webhook)
def stripe_post(http, event, secret="whsec_test", ts=None):
    payload = json.dumps(event).encode()
    ts = int(ts or time.time())
    sig = billing.StripeVerifier.sign(secret, payload, ts)
    return http.post("/v1/billing/webhook", content=payload,
                     headers={"stripe-signature": f"t={ts},v1={sig}", "content-type": "application/json"})


def sub_event(eid, etype, email, status, period_end, lookup_key="base_monthly", customer="cus_1"):
    return {"id": eid, "type": etype, "data": {"object": {
        "id": "sub_1", "customer": customer, "status": status, "metadata": {"email": email},
        "items": {"data": [{"price": {"lookup_key": lookup_key}, "current_period_end": period_end}]}}}}


def test_stripe_signature_checks():
    v = billing.StripeVerifier("whsec_test", tolerance_s=300, clock=lambda: 1_000_000)
    payload = b'{"id":"evt_1","type":"x"}'
    good = billing.StripeVerifier.sign("whsec_test", payload, 1_000_000)
    assert v.verify(payload, {"stripe-signature": f"t=1000000,v1=bad,v1={good}"})["id"] == "evt_1"
    for header in ("", f"t=1000000,v1={'0' * 64}", f"v1={good}", f"t=999000,v1={billing.StripeVerifier.sign('whsec_test', payload, 999000)}"):
        with pytest.raises(billing.SignatureError):
            v.verify(payload, {"stripe-signature": header})
    with pytest.raises(billing.SignatureError):  # body changed after signing
        v.verify(payload.replace(b"x", b"y"), {"stripe-signature": f"t=1000000,v1={good}"})
    with pytest.raises(billing.SignatureError):
        billing.StripeVerifier("").verify(payload, {"stripe-signature": f"t=1000000,v1={good}"})


def test_webhook_http_rejects_unsigned_and_old(http):
    _, _, email = linked_device()
    ev = sub_event("evt_x", "customer.subscription.created", email, "active", int(time.time()) + 86400)
    assert stripe_post(http, ev, secret="whsec_wrong").status_code == 400
    assert stripe_post(http, ev, ts=time.time() - 3600).status_code == 400
    assert http.post("/v1/billing/webhook", json=ev, headers={"x-billing-secret": "anything"}).status_code == 400
    assert db.subscription_state(db.account_by_email(email)) == "trial"


def test_subscription_lifecycle(http):
    _, h, email = linked_device()
    dev = lambda: db.device_by_token(h["authorization"][7:])  # noqa: E731
    with db.conn() as c:
        c.execute("UPDATE accounts SET trial_ends_at=0 WHERE email=?", (email,))  # free month over
    assert policy.decide(dev()).error_type == "subscription_inactive"
    end = int(time.time()) + 30 * 86400

    r = stripe_post(http, sub_event("evt_1", "customer.subscription.created", email, "active", end, "calc_pro_monthly"))
    assert r.status_code == 200 and r.json()["result"] == "subscription active"
    a = db.account_by_email(email)
    assert (db.subscription_state(a), a["paid_until"], db.account_tier(a), a["billing_customer"]) == ("active", end, "pro", "cus_1")
    assert policy.decide(dev()).ok
    assert stripe_post(http, sub_event("evt_1", "customer.subscription.created", email, "active", end)).json()["result"] == "duplicate"

    # card fails: past_due with a grace period, then blocked
    stripe_post(http, {"id": "evt_2", "type": "invoice.payment_failed", "data": {"object": {"customer": "cus_1"}}})
    a = db.account_by_email(email)
    assert db.subscription_state(a) == "past_due" and policy.decide(dev()).ok
    assert a["paid_until"] <= time.time() + config.PAST_DUE_GRACE_DAYS * 86400 + 5
    with db.conn() as c:
        c.execute("UPDATE accounts SET paid_until=1 WHERE email=?", (email,))
    d = policy.decide(dev())
    assert d.error_type == "subscription_inactive" and "Payment failed" in d.message

    # paid again (found by customer id, no e-mail in the event)
    ev = sub_event("evt_3", "customer.subscription.updated", "", "active", end + 86400)
    ev["data"]["object"]["metadata"] = {}
    stripe_post(http, ev)
    assert db.subscription_state(db.account_by_email(email)) == "active"

    # canceled now
    ev = sub_event("evt_4", "customer.subscription.deleted", "", "canceled", end)
    ev["data"]["object"]["ended_at"] = int(time.time()) - 1
    stripe_post(http, ev)
    a = db.account_by_email(email)
    assert db.subscription_state(a) == "canceled"
    assert "Subscription ended" in policy.decide(dev()).message


def test_first_month_is_free_then_needs_a_subscription():
    dev_id, h, email = linked_device()
    a = db.account_by_email(email)
    assert db.subscription_state(a) == "trial"
    assert a["trial_ends_at"] - a["created_at"] == config.TRIAL_DAYS * 86400
    assert db.subscription_state(a, ts=a["trial_ends_at"] + 1) == "inactive"


# ---------------------------------------------------------------- 6. /healthz
def test_healthz_reveals_nothing(http):
    assert http.get("/healthz").json() == {"ok": True, "version": config.VERSION}


# ---------------------------------------------------------------- 7. SSE keep-alive
def test_keepalive_pings_only_between_events(http, fake):
    config.KEEPALIVE_SECONDS = 0.05
    data = claude_events(ANSWER, "claude-sonnet-5-5")
    # long silences in the middle of events (chunks split events at odd places)
    fake({"claude-sonnet-5-5": [FakeUpstream(data, chunk=11, delay=0.2, delay_at={1, 5, 9})]})
    _, h, email = linked_device()
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    body = r.content
    assert body.count(solve.KEEPALIVE) >= 3
    for event in body.split(b"\n\n")[:-1]:
        assert event == b": ping" or event.startswith(b"event: "), event[:40]
    text, finished, _ = stream_reader(body)
    assert finished and text == ANSWER
    assert body.replace(solve.KEEPALIVE, b"") == data  # otherwise byte-for-byte Claude's stream
    assert used(email) == 1


def test_stream_cut_off_sends_error_event_and_is_not_counted(http, fake):
    fake({"claude-sonnet-5-5": [FakeUpstream(claude_events(ANSWER, "claude-sonnet-5-5"), chunk=50, fail_after=3)]})
    _, h, email = linked_device()
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    text, finished, error = stream_reader(r.content)
    assert not finished and error == "overloaded_error"  # calculator: "busy"
    assert used(email) == 0


def test_framer_notes_usage_and_model():
    f = solve.SseFramer()
    out = []
    data = claude_events("hi", "claude-sonnet-5-5", cache_read=1234)
    for i in range(0, len(data), 5):
        out += f.feed(data[i:i + 5])
    assert b"".join(out) == data and f.seen_stop and f.model == "claude-sonnet-5-5"
    assert f.usage["cache_read_input_tokens"] == 1234 and f.usage["output_tokens"] == 321


# ---------------------------------------------------------------- 8. logging and request ids
def test_logs_have_request_ids_and_no_secrets(http, fake, caplog):
    fake({"claude-sonnet-5-5": [FakeUpstream(claude_events(ANSWER, "claude-sonnet-5-5"))]})
    _, h, _ = linked_device()
    token = h["authorization"][7:]
    with caplog.at_level(logging.INFO, logger="calc-proxy"):
        r = http.post("/v1/solve", headers={**h, "x-request-id": "req_from_device1"}, json=calculator_body())
    assert r.headers["x-request-id"] == "req_from_device1"
    text = "\n".join(rec.getMessage() for rec in caplog.records)
    assert "cache_read=2000" in text and "prompt=solver_v1+" in text
    assert token not in text and IMAGE_B64[:200] not in text and ANSWER not in text
    assert any(getattr(rec, "request_id", "") == "req_from_device1" for rec in caplog.records)
    assert http.get("/healthz", headers={"x-request-id": "bad id!"}).headers["x-request-id"].startswith("req_")


def test_redaction():
    line = ("key sk-ant-api03-abcDEF_123 token dt_abcdefghijklmnop Bearer xyz.123 "
            "/link/confirm?t=eyJhbGc.sig whsec_abc123 " + "A" * 300)
    out = logs.redact(line)
    for secret in ("abcDEF_123", "abcdefghijklmnop", "xyz.123", "eyJhbGc", "whsec_abc123", "A" * 300):
        assert secret not in out


# ---------------------------------------------------------------- firmware board family on the image too
def test_firmware_image_board_family(http, tmp_path):
    p = tmp_path / "fw.bin"
    p.write_bytes(b"\xe9" + b"\x5a" * 149_999)
    firmware.publish(str(p), "v15lcd-2026.10.10")
    _, h, _ = linked_device()
    assert http.get("/v1/firmware/image", headers={**h, "x-firmware": "stage14-2026.10.06"}).status_code == 404
    assert http.get("/v1/firmware/image", headers={**h, "x-firmware": "v15lcd-2026.10.08"}).status_code == 200
    assert http.get("/v1/firmware", headers={**h, "x-firmware": "stage14-2026.10.06"}).json()["update"] is False


# ================================================================ follow-ups (2026-10-10, second pass)
import hashlib  # noqa: E402
NEW_FW = {"stage14": "stage14-2026.10.10", "v15lcd": "v15lcd-2026.10.10"}  # firmware that shows rate_limited


def fw_image(tmp_path, name, fill):
    p = tmp_path / name
    p.write_bytes(b"\xe9" + bytes([fill]) * 149_999)
    return str(p)


# ---------------------------------------------------------------- rate_limited, with old firmware kept working
def test_rate_limit_type_follows_the_firmware_version():
    t = policy.rate_limit_error_type
    assert t("stage14-2026.10.10") == t("v15lcd-2026.10.10") == t("v15lcd-2026.11.02b") == "rate_limited"
    assert t("stage14-2027.1.5") == "rate_limited"
    # older builds, unknown boards, missing or odd versions: the type every firmware displays
    for v in ("stage14-2026.10.06", "v15lcd-2026.10.08", "stage14-2026.10.09", "", None, "x", "stage14",
              "proto-2030.01.01", "stage14-junk", "STAGE14-2026.10.10"):
        assert t(v) == "fair_use_exceeded", v
    assert policy.LEGACY_RATE_LIMIT_ERROR_TYPE in ACCOUNT_ERRORS


def test_burst_limit_http_type_per_firmware(http, fake):
    fake({"claude-sonnet-5-5": []})
    dev_id, h, email = linked_device()
    lease = proxy_app.limiter.acquire("solve:" + dev_id, 1, 0)
    try:
        types = {}
        for fw in ("stage14-2026.10.06", NEW_FW["stage14"], NEW_FW["v15lcd"], ""):
            r = http.post("/v1/solve", headers={**h, "x-firmware": fw} if fw else h, json=calculator_body())
            assert r.status_code == 429 and r.headers["retry-after"] and len(r.json()["error"]["message"]) <= 110
            types[fw] = r.json()["error"]["type"]
    finally:
        lease.release()
    assert types == {"stage14-2026.10.06": "fair_use_exceeded", NEW_FW["stage14"]: "rate_limited",
                     NEW_FW["v15lcd"]: "rate_limited", "": "fair_use_exceeded"}
    assert used(email) == 0
    # the monthly cap keeps its own type (all firmware shows it)
    config.MONTHLY_CAP = 0
    r = http.post("/v1/solve", headers={**h, "x-firmware": NEW_FW["v15lcd"]}, json=calculator_body())
    assert r.json()["error"]["type"] == "fair_use_exceeded"


def test_small_endpoint_limit_type_per_firmware(http, monkeypatch):
    monkeypatch.setattr(config, "OTHER_PER_MINUTE", 1)
    _, h, _ = linked_device()
    assert http.get("/v1/device/status", headers=h).status_code == 200
    old = http.get("/v1/device/status", headers={**h, "x-firmware": "stage14-2026.10.06"})
    new = http.post("/v1/pair/start", headers={**h, "x-firmware": NEW_FW["stage14"]})
    assert (old.status_code, old.json()["error"]["type"]) == (429, "fair_use_exceeded")
    assert (new.status_code, new.json()["error"]["type"]) == (429, "rate_limited")


def test_request_without_system_prompt():
    b = calculator_body()
    del b["system"]
    system, _, fmt, _, _ = solve.clean_request(b)  # the server's prompt (PROMPT_SOURCE=server)
    assert system == solve.server_prompt()[0] and fmt["schema"]["required"][0] == "readable"
    config.PROMPT_SOURCE = "device"  # bench mode needs a firmware built with CALC_SEND_SYSTEM_PROMPT=1
    with pytest.raises(solve.BadRequest):
        solve.clean_request(b)
    b2 = calculator_body()
    del b2["output_config"]["format"]  # the schema is still required
    config.PROMPT_SOURCE = "server"
    with pytest.raises(solve.BadRequest):
        solve.clean_request(b2)


# ---------------------------------------------------------------- one firmware image per board family
def test_firmware_one_image_per_family(tmp_path):
    d = str(tmp_path / "fw")
    v14 = firmware.publish(fw_image(tmp_path, "a.bin", 0x11), "stage14-2026.10.10", d)
    v15 = firmware.publish(fw_image(tmp_path, "b.bin", 0x22), "v15lcd-2026.10.10", d)
    assert v14["sha256"] != v15["sha256"]
    assert set(firmware.published(d)) == {"stage14", "v15lcd"}
    a = firmware.decide("stage14-2026.10.06", d)
    b = firmware.decide("v15lcd-2026.10.08", d)
    assert (a["update"], a["version"], a["sha256"]) == (True, "stage14-2026.10.10", v14["sha256"])
    assert (b["update"], b["version"], b["sha256"]) == (True, "v15lcd-2026.10.10", v15["sha256"])
    assert firmware.decide("stage14-2026.10.10", d)["update"] is False
    assert firmware.decide("proto-1", d) == {"update": False, "version": ""}  # unknown board: nothing
    assert firmware.decide("", d) == {"update": False, "version": ""}
    assert firmware.image_for("", d) is None  # two families and no header: never guess
    # publishing a new v15 build leaves the v14 image alone
    v15b = firmware.publish(fw_image(tmp_path, "c.bin", 0x33), "v15lcd-2026.10.11", d)
    assert firmware.current("stage14", d)["sha256"] == v14["sha256"]
    assert firmware.current("v15lcd", d)["sha256"] == v15b["sha256"]
    with pytest.raises(ValueError):
        firmware.publish(fw_image(tmp_path, "e.bin", 1), "../evil-1", d)
    with pytest.raises(ValueError):
        firmware.publish(fw_image(tmp_path, "f.bin", 1), "Stage14-1", d)


def test_firmware_legacy_single_image_still_served(tmp_path):
    """An image published by the older proxy (FIRMWARE_DIR/firmware.bin) keeps working for its family."""
    d = tmp_path / "fw"
    d.mkdir()
    src = fw_image(tmp_path, "old.bin", 0x44)
    (d / firmware.IMAGE).write_bytes(open(src, "rb").read())
    (d / firmware.MANIFEST).write_text(json.dumps({"version": "stage14-2026.10.07", "size": 150_000,
                                                   "sha256": firmware.sha256_of(src)}))
    assert firmware.decide("stage14-2026.10.06", str(d))["update"] is True
    assert firmware.decide("v15lcd-2026.10.08", str(d))["update"] is False
    assert firmware.image_for("", str(d))["version"] == "stage14-2026.10.07"  # the only image
    # once the family has its own folder, that one wins
    firmware.publish(fw_image(tmp_path, "new.bin", 0x55), "stage14-2026.10.10", str(d))
    assert firmware.current("stage14", str(d))["version"] == "stage14-2026.10.10"


def test_firmware_both_families_over_http(http, tmp_path, monkeypatch):
    monkeypatch.setattr(config, "FIRMWARE_DIR", str(tmp_path / "fw"))
    v14 = firmware.publish(fw_image(tmp_path, "a.bin", 0x66), "stage14-2026.10.10")
    v15 = firmware.publish(fw_image(tmp_path, "b.bin", 0x77), "v15lcd-2026.10.10")
    _, h, _ = linked_device()
    for old, new, m in (("stage14-2026.10.06", "stage14-2026.10.10", v14), ("v15lcd-2026.10.08", "v15lcd-2026.10.10", v15)):
        man = http.get("/v1/firmware", headers={**h, "x-firmware": old}).json()
        assert (man["update"], man["version"], man["sha256"]) == (True, new, m["sha256"])
        img = http.get(man["path"], headers={**h, "x-firmware": old})
        assert img.status_code == 200 and img.headers["x-firmware-version"] == new
        assert hashlib.sha256(img.content).hexdigest() == m["sha256"]
    # cross-family: a board whose family has no image gets nothing, never the other board's image
    r = http.get("/v1/firmware/image", headers={**h, "x-firmware": "proto-2026.10.10"})
    assert r.status_code == 404
    assert http.get("/v1/firmware/image", headers=h).status_code == 404  # no header, two images
    monkeypatch.setattr(config, "FIRMWARE_DIR", str(tmp_path / "fw2"))
    firmware.publish(fw_image(tmp_path, "c.bin", 0x78), "v15lcd-2026.10.10")
    assert http.get("/v1/firmware/image", headers={**h, "x-firmware": "stage14-2026.10.06"}).status_code == 404
    assert http.get("/v1/firmware", headers={**h, "x-firmware": "stage14-2026.10.06"}).json()["update"] is False


# ---------------------------------------------------------------- retry on the other model before the first content byte
def error_events(error_type, model, with_start=True):
    ev = []
    if with_start:
        ev.append(solve.sse("message_start", {"type": "message_start", "message": {
            "id": "msg_x", "type": "message", "role": "assistant", "model": model, "content": [], "usage": {}}}))
        ev.append(solve.sse("ping", {"type": "ping"}))
    ev.append(solve.sse("error", {"type": "error", "error": {"type": error_type, "message": "Overloaded"}}))
    return b"".join(ev)


@pytest.mark.parametrize("early", ["overloaded_event", "error_event_first", "dropped", "ended_silently"])
def test_early_stream_failure_retries_on_the_other_model(http, fake, early):
    first = {"overloaded_event": FakeUpstream(error_events("overloaded_error", "claude-sonnet-5-5")),
             "error_event_first": FakeUpstream(error_events("api_error", "claude-sonnet-5-5", with_start=False)),
             # message_start arrives, then the connection drops (fail_after counts 50-byte chunks)
             "dropped": FakeUpstream(claude_events(ANSWER, "claude-sonnet-5-5"), chunk=50, fail_after=2),
             "ended_silently": FakeUpstream(solve.sse(
                 "message_start", {"type": "message_start", "message": {"model": "claude-sonnet-5-5"}}))}[early]
    c = fake({"claude-sonnet-5-5": [first],
              "claude-opus-5-5": [FakeUpstream(claude_events(ANSWER, "claude-opus-5-5"))]})
    _, h, email = linked_device()
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    text, finished, error = stream_reader(r.content)
    assert finished and not error and json.loads(text)["answer"] == "4"
    assert [x["model"] for x in c.calls] == ["claude-sonnet-5-5", "claude-opus-5-5"]
    assert r.content.count(b"event: message_start") == 1  # the failed attempt's prelude was dropped
    assert used(email) == 1


def test_failure_after_content_is_not_retried(http, fake):
    data = claude_events(ANSWER, "claude-sonnet-5-5")
    c = fake({"claude-sonnet-5-5": [FakeUpstream(data, chunk=50, fail_after=len(data) // 50 - 2)],
              "claude-opus-5-5": [FakeUpstream(claude_events(ANSWER, "claude-opus-5-5"))]})
    _, h, email = linked_device()
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    text, finished, error = stream_reader(r.content)
    assert text and not finished and error == "overloaded_error"  # calculator: "busy", as before
    assert len(c.calls) == 1 and used(email) == 0
    # an error event after content: passed through unchanged, not retried
    mid = data[:data.index(b"event: content_block_stop")] + error_events("overloaded_error", "", with_start=False)
    c = fake({"claude-sonnet-5-5": [FakeUpstream(mid)], "claude-opus-5-5": [FakeUpstream(data)]})
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert stream_reader(r.content)[2] == "overloaded_error" and len(c.calls) == 1


def test_early_failure_without_a_fallback(http, fake):
    # non-retryable error event before content: passed on, no second model
    c = fake({"claude-sonnet-5-5": [FakeUpstream(error_events("invalid_request_error", "claude-sonnet-5-5"))],
              "claude-opus-5-5": []})
    dev_id, h, email = linked_device()
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert stream_reader(r.content)[2] == "invalid_request_error" and len(c.calls) == 1
    # fallback switched off: the overloaded error reaches the calculator ("busy")
    config.MODEL_FALLBACK = False
    c = fake({"claude-sonnet-5-5": [FakeUpstream(error_events("overloaded_error", "claude-sonnet-5-5"))]})
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert stream_reader(r.content)[2] == "overloaded_error" and len(c.calls) == 1
    # the fallback model is down too: the first failure is reported, once
    config.MODEL_FALLBACK = True
    c = fake({"claude-sonnet-5-5": [FakeUpstream(error_events("overloaded_error", "claude-sonnet-5-5"))],
              "claude-opus-5-5": [api_status_error(529)]})
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert stream_reader(r.content)[2] == "overloaded_error" and r.content.count(b"event: error") == 1
    assert [x["model"] for x in c.calls] == ["claude-sonnet-5-5", "claude-opus-5-5"]
    # the stream that already fell back at connect time has no third model to try
    c = fake({"claude-sonnet-5-5": [api_status_error(529)],
              "claude-opus-5-5": [FakeUpstream(error_events("overloaded_error", "claude-opus-5-5"))]})
    r = http.post("/v1/solve", headers=h, json=calculator_body())
    assert stream_reader(r.content)[2] == "overloaded_error" and len(c.calls) == 2
    assert used(email) == 0
    # the lease was released every time (another one can be taken at once)
    proxy_app.limiter.acquire("solve:" + dev_id, 1, 0).release()


def test_event_helpers():
    assert solve.event_type(solve.sse("message_start", {"type": "message_start"})) == "message_start"
    assert solve.event_type(b'data: {"type":"ping"}\n\n') == "ping"
    assert solve.event_type(solve.KEEPALIVE) == ""
    assert solve.event_error_type(error_events("overloaded_error", "", with_start=False)) == "overloaded_error"
    assert solve.event_error_type(solve.sse("message_stop", {"type": "message_stop"})) == ""
