"""AI Calculator proxy: holds the Claude API key so the calculators never do.

Run:  uvicorn app:app --host 0.0.0.0 --port 8000   (behind HTTPS; see README.md)

Calculator endpoints (header  authorization: Bearer <device token>):
  POST /v1/solve           the photo request -> Claude's streamed reply (server-sent events)
  POST /v1/pair/start      a pairing code to show on the calculator
  GET  /v1/device/status   linked account, subscription, solves this month
  GET  /v1/firmware        is there a newer firmware for this calculator? (x-firmware header; size + sha256)
  GET  /v1/firmware/image  the firmware image itself (over-the-air update)
People:
  GET/POST /link           enter the pairing code + e-mail: links the calculator (free month starts)
Billing / admin:
  POST /v1/billing/webhook   {"email", "paid_until"} with header x-billing-secret
  POST /v1/admin/devices     create a device token (header x-admin-token; off unless ADMIN_TOKEN is set)
"""
import html
import json
import logging
from urllib.parse import unquote_plus

import anthropic
from fastapi import FastAPI, Header, Request
from fastapi.responses import FileResponse, HTMLResponse, JSONResponse, StreamingResponse
from starlette.concurrency import run_in_threadpool

import config
import db
import firmware
import policy
import solve

log = logging.getLogger("calc-proxy")
app = FastAPI(title="AI Calculator proxy", docs_url=None, redoc_url=None)
db.init()


def err(status, error_type, message):
    return JSONResponse(solve.error_body(error_type, message), status_code=status)


def device_from(authorization, hw_id):
    token = (authorization or "").removeprefix("Bearer ").strip()
    dev = db.device_by_token(token)
    if dev is not None:
        db.note_hw_id(dev["id"], hw_id)
    return dev


UNKNOWN = ("device_unknown", "This calculator isn't registered with the AI server. Check its token.")


@app.get("/healthz")
def healthz():
    return {"ok": True, "routing": config.ROUTING, "main_model": config.MAIN_MODEL}


@app.post("/v1/solve")
async def solve_endpoint(request: Request, authorization: str = Header(""), x_device_id: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    decision = policy.decide(dev)
    if not decision.ok:
        return err(decision.status, decision.error_type, decision.message)

    raw = await request.body()
    if len(raw) > config.MAX_BODY_BYTES:
        return err(413, "invalid_request_error", "Photo too large.")
    try:
        system, content, fmt, effort, tutor = solve.clean_request(json.loads(raw))
    except (ValueError, solve.BadRequest) as e:
        return err(400, "invalid_request_error", f"Bad request: {e}"[:110])

    def counted():
        for subject, period in decision.count:
            db.count_solve(subject, period)

    # Haiku first (optional): a confident, clean Haiku reply is sent as a stream.
    if config.ROUTING == "haiku-first":
        text = await run_in_threadpool(solve.try_haiku, solve.haiku_params(system, content, fmt))
        if text is not None:
            counted()
            log.info("solve %s: haiku", dev["id"])
            return StreamingResponse(solve.synth_stream(text, config.HAIKU_MODEL), media_type="text/event-stream")

    try:
        cm, upstream = await run_in_threadpool(solve.open_main_stream, solve.main_params(system, content, fmt, effort))
    except anthropic.APIStatusError as e:
        status = 503 if e.status_code in (429, 500, 502, 503, 529) else 502
        log.warning("solve %s: upstream %s", dev["id"], e.status_code)
        return err(status, "overloaded_error" if status == 503 else "api_error", "Claude is busy or refused: try again.")
    except (anthropic.APIConnectionError, RuntimeError) as e:
        log.error("solve %s: %s", dev["id"], e)
        return err(503, "overloaded_error", "The AI server can't reach Claude right now.")

    def relay():
        seen_stop = False
        tail = b""
        try:
            for chunk in upstream.iter_bytes():
                tail = (tail + chunk)[-64:]
                seen_stop = seen_stop or b"message_stop" in tail
                yield chunk
        finally:
            cm.__exit__(None, None, None)
            if seen_stop:  # only finished answers count towards the cap
                counted()
            log.info("solve %s: main%s", dev["id"], "" if seen_stop else " (unfinished)")

    return StreamingResponse(relay(), media_type="text/event-stream")


@app.post("/v1/pair/start")
def pair_start(authorization: str = Header(""), x_device_id: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    code = db.new_pair_code(dev["id"])
    return {"code": code, "link_url": config.PUBLIC_URL + "/link", "expires_in": config.PAIR_CODE_MINUTES * 60,
            "already_linked": dev["account_id"] is not None}


@app.get("/v1/device/status")
def device_status(authorization: str = Header(""), x_device_id: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    return policy.status_json(dev)


# ---- over-the-air firmware (firmware.py; the calculator checks the SHA-256 before installing)
@app.get("/v1/firmware")
def firmware_manifest(authorization: str = Header(""), x_device_id: str = Header(""), x_firmware: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    answer = firmware.decide(x_firmware)
    if answer["update"]:
        log.info("firmware %s: %s -> %s", dev["id"], x_firmware or "?", answer["version"])
    return answer


@app.get("/v1/firmware/image")
def firmware_image(authorization: str = Header(""), x_device_id: str = Header("")):
    dev = device_from(authorization, x_device_id)
    if dev is None:
        return err(401, *UNKNOWN)
    m = firmware.current()
    if m is None:
        return err(404, "not_found", "No firmware is published.")
    return FileResponse(m["file"], media_type="application/octet-stream", filename=firmware.IMAGE,
                        headers={"x-firmware-version": m["version"], "x-firmware-sha256": m["sha256"]})


# ---- the link page (minimal: replace with your real sign-in when there is a website)
PAGE = """<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Link your AI Calculator</title><style>body{{font:16px system-ui;max-width:420px;margin:2em auto;padding:0 16px}}
input,button{{font:inherit;padding:.5em;width:100%;margin:.3em 0;box-sizing:border-box}}</style></head><body>
<h1>Link your calculator</h1><p>{msg}</p>
<form method="post"><label>Code shown on the calculator<input name="code" required maxlength="8" autocapitalize="characters"></label>
<label>Your e-mail<input name="email" type="email" required></label><button>Link</button></form>
<p><small>First month free, then $15/month. Fair use: {cap} AI solves a month. The calculator's maths always works without a subscription.</small></p>
</body></html>"""


@app.get("/link", response_class=HTMLResponse)
def link_page():
    return PAGE.format(msg="Type the 6-letter code from the calculator (SETUP, or the AI SOLVE message).",
                       cap=config.MONTHLY_CAP)


@app.post("/link", response_class=HTMLResponse)
async def link_submit(request: Request):
    form = dict(x.split("=", 1) for x in (await request.body()).decode().split("&") if "=" in x)
    code = unquote_plus(form.get("code", ""))
    email = unquote_plus(form.get("email", ""))
    if "@" not in email or len(email) > 200:
        msg = "Please enter a valid e-mail."
    elif db.claim_pair_code(code, email):
        a = db.account_by_email(email)
        msg = f"Linked to {html.escape(email)}. Subscription: {db.subscription_state(a)}. Press = on the calculator."
    else:
        msg = "That code is unknown or expired: get a new one on the calculator."
    return PAGE.format(msg=msg, cap=config.MONTHLY_CAP)


# ---- billing and admin
@app.post("/v1/billing/webhook")
async def billing_webhook(request: Request, x_billing_secret: str = Header("")):
    if not config.BILLING_SECRET or x_billing_secret != config.BILLING_SECRET:
        return err(403, "forbidden", "bad secret")
    body = await request.json()
    n = db.set_paid_until(str(body.get("email", "")), int(body.get("paid_until", 0)))
    return {"updated": n}


@app.post("/v1/admin/devices")
def admin_new_device(label: str = "", x_admin_token: str = Header("")):
    if not config.ADMIN_TOKEN or x_admin_token != config.ADMIN_TOKEN:
        return err(403, "forbidden", "admin API off or bad token")
    device_id, token = db.create_device(label)
    return {"device_id": device_id, "token": token}
