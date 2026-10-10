"""AI Calculator proxy: holds the Claude API key so the calculators never do.

Run:  uvicorn app:app --host 0.0.0.0 --port 8000 --workers 1   (behind HTTPS; see README.md)

Calculator endpoints (header  authorization: Bearer <device token>):
  POST /v1/solve           the photo request -> Claude's streamed reply (server-sent events)
  POST /v1/pair/start      a pairing code to show on the calculator
  GET  /v1/device/status   linked account, subscription, tier, solves this month
  GET  /v1/firmware        is there a newer firmware for this calculator? (x-firmware header; size + sha256)
  GET  /v1/firmware/image  the firmware image itself (over-the-air update)
People:
  GET/POST /link           pairing code + e-mail -> a confirmation e-mail (LINK_MODE=email)
  GET/POST /link/confirm   the e-mailed link: links the calculator (free month starts)
Billing / admin:
  POST /v1/billing/webhook   Stripe-signed subscription events (billing.py)
  POST /v1/admin/devices     create a device token (header x-admin-token; off unless ADMIN_TOKEN is set)
  GET  /healthz              {"ok": true, "version": ...}
"""
import hmac
import html
import json
import logging
import queue
import threading
import time
from urllib.parse import parse_qs, quote

import anthropic
from fastapi import FastAPI, Header, Request
from fastapi.responses import FileResponse, HTMLResponse, JSONResponse, StreamingResponse
from starlette.concurrency import run_in_threadpool

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

logs.setup(config.LOG_LEVEL)
log = logging.getLogger("calc-proxy")
app = FastAPI(title="AI Calculator proxy", docs_url=None, redoc_url=None, openapi_url=None)
db.init()
limiter = limits.Limiter()

if not config.LINK_SECRET:
    log.warning("LINK_SECRET is not set: e-mail confirmation links stop working when the server restarts")
if config.LINK_MODE != "email":
    log.warning("LINK_MODE=%s: anyone with a pairing code can link it to any e-mail (bench testing only)",
                config.LINK_MODE)


@app.middleware("http")
async def request_context(request: Request, call_next):
    rid = logs.new_request_id(request.headers.get("x-request-id", ""))
    token = logs.request_id.set(rid)
    t0 = time.monotonic()
    try:
        response = await call_next(request)
        response.headers["x-request-id"] = rid
        response.headers["x-content-type-options"] = "nosniff"
        response.headers["referrer-policy"] = "no-referrer"
        if "cache-control" not in response.headers:
            response.headers["cache-control"] = "no-store"
        if response.headers.get("content-type", "").startswith("text/html"):
            response.headers["content-security-policy"] = (
                "default-src 'none'; style-src 'unsafe-inline'; form-action 'self'; frame-ancestors 'none'; "
                "base-uri 'none'")
        if request.url.path != "/healthz":
            log.info("%s %s -> %s (%d ms)", request.method, request.url.path, response.status_code,
                     (time.monotonic() - t0) * 1000)
        return response
    finally:
        logs.request_id.reset(token)


def err(status, error_type, message, headers=None):
    return JSONResponse(solve.error_body(error_type, message), status_code=status, headers=headers)


def device_from(authorization, hw_id):
    token = (authorization or "").removeprefix("Bearer ").strip()
    dev = db.device_by_token(token)
    if dev is not None and hw_id and not dev["hw_id"]:
        db.note_hw_id(dev["id"], hw_id)
    return dev


UNKNOWN = ("device_unknown", "This calculator isn't registered with the AI server. Check its token.")


def limited(e: limits.Limited, kind=None):
    log.info("rate limited: %s", e.reason)
    return err(429, policy.RATE_LIMIT_ERROR_TYPE, policy.RATE_MESSAGES[kind or e.reason],
               headers={"retry-after": str(e.retry_after)})


def small_request_limit(dev):
    try:
        limiter.hit("other:" + dev["id"], config.OTHER_PER_MINUTE)
    except limits.Limited as e:
        return limited(e, "other")
    return None


@app.get("/healthz")
def healthz():
    return {"ok": True, "version": config.VERSION}


@app.post("/v1/solve")
async def solve_endpoint(request: Request, authorization: str = Header(""), x_device_id: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    decision = policy.decide(dev)
    if not decision.ok:
        log.info("solve %s: denied %s", dev["id"], decision.error_type)
        return err(decision.status, decision.error_type, decision.message)

    try:
        declared = int(request.headers.get("content-length") or 0)
    except ValueError:
        declared = 0
    if declared > config.MAX_BODY_BYTES:
        return err(413, "invalid_request_error", "Photo too large.")
    raw = await request.body()
    if len(raw) > config.MAX_BODY_BYTES:
        return err(413, "invalid_request_error", "Photo too large.")
    try:
        system, content, fmt, effort, tutor = solve.clean_request(json.loads(raw))
    except (ValueError, solve.BadRequest) as e:
        return err(400, "invalid_request_error", f"Bad request: {e}"[:110])
    del raw
    prompt_version = solve.server_prompt()[2] if config.PROMPT_SOURCE != "device" else "device"

    try:
        lease = limiter.acquire("solve:" + dev["id"], config.SOLVE_CONCURRENCY,
                                config.SOLVES_PER_MINUTE, config.SOLVES_PER_HOUR)
    except limits.Limited as e:
        return limited(e)

    def counted():
        for subject, period in decision.count:
            db.count_solve(subject, period)

    # Haiku first (optional): a confident, clean Haiku reply is sent as a stream.
    if config.ROUTING == "haiku-first":
        text = await run_in_threadpool(solve.try_haiku, solve.haiku_params(system, content, fmt))
        if text is not None:
            counted()
            lease.release()
            log.info("solve %s: model=%s tier=%s prompt=%s", dev["id"], config.HAIKU_MODEL, decision.tier, prompt_version)
            return StreamingResponse(solve.synth_stream(text, config.HAIKU_MODEL), media_type="text/event-stream")

    first = policy.model_for(decision.tier)
    params = solve.main_params(system, content, fmt, effort, model=first)
    try:
        cm, upstream, model = await run_in_threadpool(solve.open_main_stream, params, solve.model_chain(first))
    except anthropic.APIStatusError as e:
        lease.release()
        status = 503 if e.status_code in solve.RETRYABLE_STATUS else 502
        log.warning("solve %s: upstream HTTP %s", dev["id"], e.status_code)
        return err(status, "overloaded_error" if status == 503 else "api_error", "Claude is busy or refused: try again.")
    except (anthropic.APIConnectionError, RuntimeError) as e:
        lease.release()
        log.error("solve %s: %s", dev["id"], type(e).__name__ if isinstance(e, anthropic.APIError) else e)
        return err(503, "overloaded_error", "The AI server can't reach Claude right now.")

    rid = logs.request_id.get()
    started = time.monotonic()
    return StreamingResponse(
        relay(upstream, cm, lease, counted, dev["id"], model, decision.tier, prompt_version, rid, started),
        media_type="text/event-stream", headers={"cache-control": "no-cache", "x-accel-buffering": "no"})


_END = object()


def relay(upstream, cm, lease, counted, device_id, model, tier, prompt_version, rid, started):
    """Passes Claude's events through, whole events at a time, with a ": ping" comment every
    KEEPALIVE_SECONDS while Claude is quiet (long thinking), so the calculator's 120 s idle
    timeout never fires. Only finished answers (message_stop) count towards the cap."""
    q = queue.Queue()
    stop = threading.Event()

    def pump():
        try:
            for chunk in upstream.iter_bytes():
                if stop.is_set():
                    break
                q.put(chunk)
        except Exception as e:  # noqa: BLE001 (handed to the consumer)
            q.put(e)
        finally:
            q.put(_END)

    threading.Thread(target=pump, name="sse-pump", daemon=True).start()
    framer = solve.SseFramer()
    pings = 0
    try:
        while True:
            try:
                item = q.get(timeout=config.KEEPALIVE_SECONDS)
            except queue.Empty:
                pings += 1
                yield solve.KEEPALIVE
                continue
            if item is _END:
                tail = framer.flush()
                if tail:
                    yield tail
                break
            if isinstance(item, Exception):
                if not framer.seen_stop:  # the half-received event is dropped
                    yield solve.error_event("overloaded_error", "The connection to Claude dropped: try again.")
                break
            yield from framer.feed(item)
    finally:
        stop.set()
        try:
            cm.__exit__(None, None, None)
        except Exception:  # noqa: BLE001
            pass
        lease.release()
        if framer.seen_stop:
            counted()
        token = logs.request_id.set(rid)
        try:
            u = framer.usage
            log.info("solve %s: model=%s%s tier=%s prompt=%s %s in=%s cache_read=%s cache_write=%s out=%s pings=%d %.1fs",
                     device_id, model, f" (answered by {framer.model})" if framer.model and framer.model != model else "",
                     tier, prompt_version, "done" if framer.seen_stop else "UNFINISHED",
                     u.get("input_tokens"), u.get("cache_read_input_tokens"), u.get("cache_creation_input_tokens"),
                     u.get("output_tokens"), pings, time.monotonic() - started)
        finally:
            logs.request_id.reset(token)


@app.post("/v1/pair/start")
def pair_start(authorization: str = Header(""), x_device_id: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    if (r := small_request_limit(dev)) is not None:
        return r
    code = db.new_pair_code(dev["id"])
    return {"code": code, "link_url": config.PUBLIC_URL + "/link", "expires_in": config.PAIR_CODE_MINUTES * 60,
            "already_linked": dev["account_id"] is not None}


@app.get("/v1/device/status")
def device_status(authorization: str = Header(""), x_device_id: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    if (r := small_request_limit(dev)) is not None:
        return r
    return policy.status_json(dev)


# ---- over-the-air firmware (firmware.py; the calculator checks the SHA-256 before installing)
@app.get("/v1/firmware")
def firmware_manifest(authorization: str = Header(""), x_device_id: str = Header(""), x_firmware: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    if (r := small_request_limit(dev)) is not None:
        return r
    answer = firmware.decide(x_firmware)
    if answer["update"]:
        log.info("firmware %s: %s -> %s", dev["id"], (x_firmware or "?")[:40], answer["version"])
    return answer


@app.get("/v1/firmware/image")
def firmware_image(authorization: str = Header(""), x_device_id: str = Header(""), x_firmware: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    if (r := small_request_limit(dev)) is not None:
        return r
    m = firmware.current()
    if m is None:
        return err(404, "not_found", "No firmware is published.")
    # Same board-family check as the manifest (a calculator that sends x-firmware must match).
    if x_firmware and not firmware.same_family(x_firmware, m["version"]):
        return err(404, "not_found", "No firmware for this calculator's board.")
    return FileResponse(m["file"], media_type="application/octet-stream", filename=firmware.IMAGE,
                        headers={"x-firmware-version": m["version"], "x-firmware-sha256": m["sha256"]})


# ---- linking a calculator to an e-mail (see linking.py)
PAGE = """<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="referrer" content="no-referrer">
<title>Link your AI Calculator</title><style>body{{font:16px system-ui;max-width:420px;margin:2em auto;padding:0 16px}}
input,button{{font:inherit;padding:.5em;width:100%;margin:.3em 0;box-sizing:border-box}}</style></head><body>
<h1>Link your calculator</h1><p>{msg}</p>{form}
<p><small>First month free, then $15/month. Fair use: {cap} AI solves a month. The calculator's maths always works without a subscription.</small></p>
</body></html>"""

LINK_FORM = """<form method="post" action="/link"><label>Code shown on the calculator<input name="code" required maxlength="8" autocapitalize="characters" autocomplete="off"></label>
<label>Your e-mail<input name="email" type="email" required maxlength="200" autocomplete="email"></label><button>Link</button></form>"""

CONFIRM_FORM = """<form method="post" action="/link/confirm"><input type="hidden" name="t" value="{t}">
<button>Link this calculator to {email}</button></form>"""


def page(msg, form="", status=200):
    return HTMLResponse(PAGE.format(msg=msg, form=form, cap=config.MONTHLY_CAP), status_code=status)


async def form_fields(request):
    raw = await request.body()
    if len(raw) > 4096:
        return {}
    return {k: v[0] for k, v in parse_qs(raw.decode("utf-8", "replace"), max_num_fields=10).items()}


def client_ip(request):
    return request.client.host if request.client else "?"


def valid_email(email):
    return 3 < len(email) <= 200 and email.count("@") == 1 and "." in email.split("@")[1] and not any(
        c in email for c in " <>\"'\r\n,;")


@app.get("/link", response_class=HTMLResponse)
def link_page():
    return page("Type the 6-letter code from the calculator (SETUP, or the AI SOLVE message).", LINK_FORM)


@app.post("/link", response_class=HTMLResponse)
async def link_submit(request: Request):
    try:
        limiter.hit("link:" + client_ip(request), config.LINK_PER_MINUTE_PER_IP)
    except limits.Limited:
        return page("Too many tries. Wait a minute, then try again.", LINK_FORM, status=429)
    form = await form_fields(request)
    code = db.clean_code(form.get("code", ""))
    email = db.normalize_email(form.get("email", ""))
    if not valid_email(email):
        return page("Please enter a valid e-mail.", LINK_FORM, status=400)
    if db.pair_code_device(code) is None:
        return page("That code is unknown or expired: get a new one on the calculator.", LINK_FORM, status=400)

    if config.LINK_MODE != "email":  # bench testing only (see config.py)
        return finish_link(code, email)

    token, exp = linking.make_token(code, email)
    db.extend_pair_code(code, exp)
    url = f"{config.PUBLIC_URL}/link/confirm?t={quote(token)}"
    body = (f"Someone asked to link an AI Calculator to this e-mail address.\n\n"
            f"To link it (your free month starts), open this link within {config.LINK_CONFIRM_MINUTES} minutes:\n{url}\n\n"
            f"If this wasn't you, ignore this e-mail: nothing is linked.\n")
    try:
        await run_in_threadpool(mailer.get_mailer().send, email, "Link your AI Calculator", body, url)
    except Exception as e:  # noqa: BLE001
        log.error("link mail failed: %s", type(e).__name__)
        return page("We couldn't send the e-mail. Try again in a few minutes.", LINK_FORM, status=503)
    return page(f"Check your e-mail ({html.escape(email)}): open the link in it to finish linking. "
                f"It works for {config.LINK_CONFIRM_MINUTES} minutes.")


@app.get("/link/confirm", response_class=HTMLResponse)
def link_confirm_page(t: str = ""):
    data = linking.read_token(t)
    if data is None:
        return page("This link is invalid or has expired. Start again on the calculator.", LINK_FORM, status=400)
    return page("One more step:", CONFIRM_FORM.format(t=html.escape(t, quote=True), email=html.escape(data[1])))


@app.post("/link/confirm", response_class=HTMLResponse)
async def link_confirm(request: Request):
    form = await form_fields(request)
    data = linking.read_token(form.get("t", ""))
    if data is None:
        return page("This link is invalid or has expired. Start again on the calculator.", LINK_FORM, status=400)
    return finish_link(*data)


def finish_link(code, email):
    device_id = db.claim_pair_code(code, email)
    if device_id is None:
        return page("That code was already used or has expired: get a new one on the calculator.", LINK_FORM, status=400)
    a = db.account_by_email(email)
    log.info("linked %s to account %s", device_id, a["id"])
    return page(f"Linked to {html.escape(email)}. Subscription: {db.subscription_state(a)}. Press = on the calculator.")


# ---- billing and admin
@app.post("/v1/billing/webhook")
async def billing_webhook(request: Request):
    payload = await request.body()
    if len(payload) > 512 * 1024:
        return err(413, "invalid_request_error", "too large")
    try:
        event = billing.verifier().verify(payload, request.headers)
    except billing.SignatureError as e:
        log.warning("billing webhook rejected: %s", e)
        return err(400, "invalid_signature", "bad signature")
    result = await run_in_threadpool(billing.apply_event, event)
    log.info("billing %s %s: %s", event.get("type"), str(event.get("id"))[:40], result)
    return {"received": True, "result": result}


@app.post("/v1/admin/devices")
def admin_new_device(label: str = "", x_admin_token: str = Header("")):
    if not config.ADMIN_TOKEN or not hmac.compare_digest(x_admin_token.encode(), config.ADMIN_TOKEN.encode()):
        return err(403, "forbidden", "admin API off or bad token")
    device_id, token = db.create_device(label[:60])
    return {"device_id": device_id, "token": token}
