"""Checking the calculator's request and calling Claude.

The calculator sends the same Messages API body the simulators send (core/claude_api.cpp:
buildSolveRequest). The proxy never forwards it blindly: it keeps only the photos and the
short instruction text, and supplies everything else itself -- the model (by tier), the
limits, the system prompt and the JSON schema (versioned files in prompts/, PROMPT_SOURCE)
-- and adds the API key. A stolen device token therefore can't be used as general Claude
access, and prompt fixes don't need a firmware flash.

Model routing: the account's tier picks the first model (base = Sonnet 5.5, pro = Opus 5.5);
on 429 / 5xx / 529 overloaded / connection errors the other model is tried once
(MODEL_FALLBACK). The fixed system prompt carries a cache_control breakpoint
(PROMPT_CACHE), so repeat solves read it from the prompt cache.
"""
import hashlib
import json
import logging
import os

import config

log = logging.getLogger("calc-proxy")

ALLOWED_EFFORT = {"high", "xhigh", "max"}
MAX_IMAGES = 2
MAX_TEXT_CHARS = 1000
MAX_SYSTEM_CHARS = 20000     # PROMPT_SOURCE=device only
MAX_SCHEMA_CHARS = 8000      # PROMPT_SOURCE=device only
RETRYABLE_STATUS = {408, 409, 429, 500, 502, 503, 504, 529}
REFUSAL_FALLBACK_BETA = "server-side-fallback-2026-07-01"

TUTOR_NOTE = (
    "Tutor mode: the student wants to learn the method. Write the steps as small hints that "
    "build on each other, each one revealing a little more, so the student could finish alone "
    "after any step. The answer field still holds the final answer (shown last)."
)


class BadRequest(Exception):
    pass


# ---- the server's prompt (versioned files)
_prompt_cache = {}


def _read(path):
    st = os.stat(path)
    key = (path, st.st_mtime_ns, st.st_size)
    if key not in _prompt_cache:
        with open(path, encoding="utf-8") as f:
            _prompt_cache[key] = f.read()
    return _prompt_cache[key]


def server_prompt():
    """(system_text, schema_dict, version). version = file name + short hash, logged per solve."""
    system = _read(config.SYSTEM_PROMPT_FILE)
    schema_text = _read(config.SCHEMA_FILE)
    schema = json.loads(schema_text)
    digest = hashlib.sha256((system + "\0" + schema_text).encode()).hexdigest()[:8]
    name = os.path.splitext(os.path.basename(config.SYSTEM_PROMPT_FILE))[0]
    return system, schema, f"{name}+{digest}"


def clean_request(body):
    """The calculator's JSON -> (system, user_content, output_format, effort, tutor). Raises BadRequest.

    The request schema is unchanged (the firmware still sends model, system, output_config
    and so on), but by default only the photos, the instruction text, effort and tutor are
    taken from it: system prompt and JSON schema are the server's (PROMPT_SOURCE=server)."""
    if not isinstance(body, dict):
        raise BadRequest("body must be a JSON object")
    tutor = bool(body.get("tutor", False))
    device_system = body.get("system", "")
    if not isinstance(device_system, str) or len(device_system) > MAX_SYSTEM_CHARS:
        raise BadRequest("system prompt too long")
    messages = body.get("messages")
    if (not isinstance(messages, list) or len(messages) != 1 or not isinstance(messages[0], dict)
            or messages[0].get("role") != "user"):
        raise BadRequest("exactly one user message expected")
    content = messages[0].get("content")
    if not isinstance(content, list) or not content:
        raise BadRequest("message content must be a list")
    images, texts, clean = 0, 0, []
    for block in content:
        t = block.get("type") if isinstance(block, dict) else None
        if t == "image":
            src = block.get("source", {})
            if (not isinstance(src, dict) or src.get("type") != "base64"
                    or src.get("media_type") not in ("image/jpeg", "image/png")
                    or not isinstance(src.get("data"), str) or not src.get("data")):
                raise BadRequest("images must be base64 JPEG or PNG")
            images += 1
            clean.append({"type": "image", "source": {"type": "base64", "media_type": src["media_type"],
                                                      "data": src["data"]}})
        elif t == "text":
            text = block.get("text", "")
            texts += 1
            if not isinstance(text, str) or len(text) > MAX_TEXT_CHARS or texts > 1:
                raise BadRequest("one short text block expected")
            clean.append({"type": "text", "text": text})
        else:
            raise BadRequest(f"content block type {str(t)[:20]!r} not allowed")
    if images == 0 or images > MAX_IMAGES:
        raise BadRequest("1 or 2 photos expected")
    if tutor:
        clean.append({"type": "text", "text": TUTOR_NOTE})
    oc = body.get("output_config") or {}
    if not isinstance(oc, dict):
        raise BadRequest("output_config must be an object")
    fmt = oc.get("format")
    if not isinstance(fmt, dict) or fmt.get("type") != "json_schema" or not isinstance(fmt.get("schema"), dict):
        raise BadRequest("output_config.format json_schema expected")
    effort = oc.get("effort", "high")
    if effort not in ALLOWED_EFFORT:
        effort = "high"
    if config.PROMPT_SOURCE == "device":
        if not device_system:
            raise BadRequest("system prompt missing")
        if len(json.dumps(fmt["schema"])) > MAX_SCHEMA_CHARS:
            raise BadRequest("schema too long")
        system = device_system
        fmt = {"type": "json_schema", "schema": fmt["schema"]}
    else:
        system, schema, _ = server_prompt()
        fmt = {"type": "json_schema", "schema": schema}
    return system, clean, fmt, effort, tutor


def system_blocks(system):
    """The system prompt as one text block with a cache breakpoint: it is the same for every
    solve, so after the first request it is read from the cache (the photos come after it)."""
    block = {"type": "text", "text": system}
    if config.PROMPT_CACHE:
        block["cache_control"] = {"type": "ephemeral"}
    return [block]


def main_params(system, content, fmt, effort, model=None):
    """Request for the main model (streamed straight back to the calculator). Opus 5.5's default
    effort is medium, so effort is always sent explicitly."""
    return {
        "model": model or config.BASE_MODEL,
        "max_tokens": config.MAX_TOKENS,
        "thinking": {"type": "adaptive"},
        "system": system_blocks(system),
        "output_config": {"effort": effort, "format": fmt},
        "messages": [{"role": "user", "content": content}],
    }


def haiku_params(system, content, fmt):
    """Request for the cheap first try. Haiku 4.5 takes no `effort` and no adaptive thinking;
    structured outputs work. Not streamed: the proxy reads the answer before deciding."""
    return {
        "model": config.HAIKU_MODEL,
        "max_tokens": 16000,
        "system": system_blocks(system),
        "output_config": {"format": fmt},
        "messages": [{"role": "user", "content": content}],
    }


def model_chain(first):
    """Models to try in order: the tier's model, then the other one (MODEL_FALLBACK)."""
    chain = [first]
    if config.MODEL_FALLBACK:
        other = config.PRO_MODEL if first == config.BASE_MODEL else config.BASE_MODEL
        if other and other != first:
            chain.append(other)
    return chain


def client():
    """The Anthropic client (tests replace this function with a fake)."""
    import anthropic  # imported here so the request checks can be tested without the SDK

    if not config.ANTHROPIC_API_KEY:
        raise RuntimeError("ANTHROPIC_API_KEY is not set on the server")
    # Short connect timeout: an unreachable model falls back well inside the calculator's 120 s idle limit.
    return anthropic.Anthropic(api_key=config.ANTHROPIC_API_KEY, max_retries=config.CLAUDE_MAX_RETRIES,
                               timeout=anthropic.Timeout(config.CLAUDE_TIMEOUT_S, connect=10.0))


def _open(c, params):
    if config.REFUSAL_FALLBACK:
        cm = c.beta.messages.with_streaming_response.create(
            **params, stream=True, betas=[REFUSAL_FALLBACK_BETA], extra_body={"fallbacks": "default"})
    else:
        cm = c.messages.with_streaming_response.create(**params, stream=True)
    resp = cm.__enter__()
    return cm, resp


def is_retryable(exc):
    import anthropic

    if isinstance(exc, anthropic.APIConnectionError):  # includes timeouts
        return True
    return isinstance(exc, anthropic.APIStatusError) and exc.status_code in RETRYABLE_STATUS


def open_main_stream(params, models=None):
    """Opens the streamed request on the first model that answers. Returns (cm, response, model).
    The caller iterates response.iter_bytes() and must call cm.__exit__(None, None, None).
    Raises the last anthropic error (APIStatusError / APIConnectionError) when every model
    failed, before any byte is sent; non-retryable errors (400, 401, ...) are raised at once."""
    c = client()
    models = models or [params["model"]]
    last = None
    for i, model in enumerate(models):
        try:
            cm, resp = _open(c, {**params, "model": model})
            if i:
                log.warning("model fallback: %s -> %s (%s)", models[0], model, _describe(last))
            return cm, resp, model
        except Exception as e:  # noqa: BLE001 (re-raised below unless retryable)
            if not is_retryable(e):
                raise
            last = e
            log.warning("model %s unavailable: %s", model, _describe(e))
    raise last


def _describe(e):
    return f"HTTP {e.status_code}" if hasattr(e, "status_code") else type(e).__name__


def try_haiku(params):
    """Haiku's reply text if it is confident enough to stand on its own, else None."""
    import anthropic

    try:
        msg = client().messages.create(**params)
    except (anthropic.APIError, RuntimeError):
        return None
    if msg.stop_reason != "end_turn":
        return None
    text = "".join(b.text for b in msg.content if b.type == "text")
    try:
        reply = json.loads(text)
    except ValueError:
        return None
    if not isinstance(reply, dict) or not reply.get("readable"):
        return None
    try:
        if float(reply.get("confidence", 0)) < config.HAIKU_MIN_CONFIDENCE:
            return None
    except (TypeError, ValueError):
        return None
    if reply.get("unclear"):
        return None  # anything Haiku flags goes to the main model
    return text


# ---- server-sent events
KEEPALIVE = b": ping\n\n"


def sse(event, data):
    return f"event: {event}\ndata: {json.dumps(data, separators=(',', ':'))}\n\n".encode()


class SseFramer:
    """Re-frames the upstream byte stream into whole events, so keep-alive comments can only
    ever go *between* events (the calculator's StreamReader splits on blank lines). Also
    notes message_stop, the model that answered and token usage, for the log."""

    def __init__(self):
        self.buf = b""
        self.seen_stop = False
        self.model = ""
        self.usage = {}
        self.error_type = ""

    def feed(self, chunk):
        self.buf += chunk.replace(b"\r\n", b"\n")
        out = []
        while True:
            end = self.buf.find(b"\n\n")
            if end < 0:
                return out
            event, self.buf = self.buf[:end + 2], self.buf[end + 2:]
            self._note(event)
            out.append(event)

    def flush(self):
        rest, self.buf = self.buf, b""
        if rest.strip():
            self._note(rest)
            return rest if rest.endswith(b"\n\n") else rest + b"\n\n"
        return b""

    def _note(self, event):
        if b"content_block_delta" in event[:80]:
            return  # the bulk of the stream: nothing to note
        for line in event.split(b"\n"):
            if not line.startswith(b"data:"):
                continue
            try:
                d = json.loads(line[5:])
            except ValueError:
                continue
            t = d.get("type") if isinstance(d, dict) else None
            if t == "message_start":
                m = d.get("message") or {}
                self.model = m.get("model", "") or self.model
                u = m.get("usage") or {}
                for k in ("input_tokens", "cache_creation_input_tokens", "cache_read_input_tokens"):
                    if isinstance(u.get(k), int):
                        self.usage[k] = u[k]
            elif t == "message_delta":
                u = d.get("usage") or {}
                if isinstance(u.get("output_tokens"), int):
                    self.usage["output_tokens"] = u["output_tokens"]
            elif t == "message_stop":
                self.seen_stop = True
            elif t == "error":
                self.error_type = (d.get("error") or {}).get("type", "error")


# Events that come before any content: held back so an early failure can be retried on the
# other model (app.relay). Claude's own "ping" events can come at any time.
PRELUDE_EVENTS = {"message_start", "ping"}
# Error events worth one retry on the other model when they arrive before any content.
RETRYABLE_EVENT_ERRORS = {"overloaded_error", "api_error", "rate_limit_error", "timeout_error"}


def _event_data(event):
    for line in event.split(b"\n"):
        if line.startswith(b"data:"):
            try:
                d = json.loads(line[5:])
            except ValueError:
                return None
            return d if isinstance(d, dict) else None
    return None


def event_type(event):
    """The type of one whole SSE event ("event:" line, else the data's "type"); "" for a comment."""
    for line in event.split(b"\n"):
        if line.startswith(b"event:"):
            return line[6:].strip().decode("utf-8", "replace")
    d = _event_data(event)
    return str(d.get("type", "")) if d else ""


def event_error_type(event):
    """error.type of an SSE error event ("" if it isn't one)."""
    d = _event_data(event or b"")
    return str((d.get("error") or {}).get("type", "")) if d and d.get("type") == "error" else ""


def synth_stream(text, model):
    """A complete Messages API event stream carrying `text`, so the calculator reads a Haiku
    answer exactly like a streamed main-model answer."""
    yield sse("message_start", {"type": "message_start", "message": {
        "id": "msg_proxy", "type": "message", "role": "assistant", "model": model, "content": [],
        "stop_reason": None, "stop_sequence": None, "usage": {"input_tokens": 0, "output_tokens": 0}}})
    yield sse("content_block_start", {"type": "content_block_start", "index": 0,
                                      "content_block": {"type": "text", "text": ""}})
    for i in range(0, len(text), 400):
        yield sse("content_block_delta", {"type": "content_block_delta", "index": 0,
                                          "delta": {"type": "text_delta", "text": text[i:i + 400]}})
    yield sse("content_block_stop", {"type": "content_block_stop", "index": 0})
    yield sse("message_delta", {"type": "message_delta", "delta": {"stop_reason": "end_turn", "stop_sequence": None},
                                "usage": {"output_tokens": 0}})
    yield sse("message_stop", {"type": "message_stop"})


def error_event(error_type, message):
    """An SSE error event (the calculator maps overloaded_error to "busy")."""
    return sse("error", error_body(error_type, message))


def error_body(error_type, message):
    return {"type": "error", "error": {"type": error_type, "message": message}}
