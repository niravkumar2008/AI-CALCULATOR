"""Short-lived signed confirmation tokens for linking a calculator to an e-mail.

Flow (LINK_MODE=email, the default):
  1. /link: the person types the calculator's pairing code and their e-mail. Nothing is
     linked yet; the proxy e-mails a confirmation link to that address.
  2. /link/confirm?t=<token>: shows a button (mail scanners that open links don't link
     anything); POST links the calculator. Only someone who can read that inbox can finish.

The token is "<payload>.<signature>", payload = base64url(JSON {c: code, e: email, x: expiry}),
signature = HMAC-SHA256(LINK_SECRET, payload). It is single-use because the pairing code it
names is used up atomically by db.claim_pair_code.
"""
import base64
import hashlib
import hmac
import json
import logging
import secrets
import time

import config

log = logging.getLogger("calc-proxy")
_fallback_key = secrets.token_bytes(32)


def _key():
    if config.LINK_SECRET:
        return config.LINK_SECRET.encode()
    return _fallback_key  # random per start: links stop working after a restart


def _b64(b):
    return base64.urlsafe_b64encode(b).rstrip(b"=").decode()


def _unb64(s):
    return base64.urlsafe_b64decode(s + "=" * (-len(s) % 4))


def make_token(code, email, ttl_s=None, now=None):
    exp = int((now or time.time()) + (ttl_s if ttl_s is not None else config.LINK_CONFIRM_MINUTES * 60))
    payload = _b64(json.dumps({"c": code, "e": email, "x": exp}, separators=(",", ":")).encode())
    sig = _b64(hmac.new(_key(), payload.encode(), hashlib.sha256).digest())
    return f"{payload}.{sig}", exp


def read_token(token, now=None):
    """(code, email) for a valid, unexpired token, else None."""
    try:
        payload, sig = (token or "").split(".", 1)
        want = _b64(hmac.new(_key(), payload.encode(), hashlib.sha256).digest())
        if not hmac.compare_digest(sig, want):
            return None
        d = json.loads(_unb64(payload))
        if int(d["x"]) < (now or time.time()):
            return None
        return str(d["c"]), str(d["e"])
    except (ValueError, KeyError, TypeError):
        return None
