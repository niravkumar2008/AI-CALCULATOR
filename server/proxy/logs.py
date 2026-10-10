"""Logging: one request id per HTTP request (x-request-id header, in every log line) and a
redaction filter so secrets never reach the log, even by accident.

What is logged: request id, device id, model, prompt version, status, token usage, timing.
Never logged: request or response bodies (photos, answers), device tokens, API keys,
e-mail confirmation links, webhook payloads.
"""
import contextvars
import logging
import re
import secrets

request_id = contextvars.ContextVar("request_id", default="-")

_RID_OK = re.compile(r"^[A-Za-z0-9._-]{8,64}$")

# Anything that looks like a secret is replaced before the line is written.
_REDACT = [
    (re.compile(r"sk-ant-[A-Za-z0-9_\-]+"), "sk-ant-[redacted]"),
    (re.compile(r"\bdt_[A-Za-z0-9_\-]{8,}"), "dt_[redacted]"),
    (re.compile(r"\bwhsec_[A-Za-z0-9]+"), "whsec_[redacted]"),
    (re.compile(r"(?i)(bearer\s+)[A-Za-z0-9._\-]+"), r"\1[redacted]"),
    (re.compile(r"([?&]t=)[^&\s\"]+"), r"\1[redacted]"),  # /link/confirm?t=<token>
    (re.compile(r"[A-Za-z0-9+/]{200,}={0,2}"), "[base64 redacted]"),  # never an image
]


def redact(text):
    for pattern, repl in _REDACT:
        text = pattern.sub(repl, text)
    return text


def new_request_id(incoming=""):
    """The caller's x-request-id if it is a sane id, else a fresh one."""
    return incoming if incoming and _RID_OK.match(incoming) else "req_" + secrets.token_hex(8)


class ContextFilter(logging.Filter):
    def filter(self, record):
        record.request_id = request_id.get()
        try:
            msg = record.getMessage()
        except Exception:  # noqa: BLE001
            return True
        clean = redact(msg)
        if clean != msg:
            record.msg, record.args = clean, None
        return True


_installed = False


def setup(level="INFO"):
    """Installs the filter on our logger and uvicorn's (idempotent)."""
    global _installed
    if _installed:
        return
    _installed = True
    flt = ContextFilter()
    root = logging.getLogger()
    if not root.handlers:
        h = logging.StreamHandler()
        h.setFormatter(logging.Formatter("%(asctime)s %(levelname)s [%(request_id)s] %(name)s: %(message)s"))
        h.addFilter(flt)
        root.addHandler(h)
    for name in ("calc-proxy", "uvicorn.access", "uvicorn.error"):
        logging.getLogger(name).addFilter(flt)
    logging.getLogger("calc-proxy").setLevel(level)
