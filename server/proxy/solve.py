"""Checking the calculator's request and calling Claude.

The calculator sends the same Messages API body the simulators send (core/claude_api.cpp:
buildSolveRequest). The proxy never forwards it blindly: it keeps only the parts a photo
solve needs, sets the model and limits itself, and adds the API key. A stolen device token
therefore can't be used as general Claude access, and is capped per month anyway.
"""
import json

import config

ALLOWED_EFFORT = {"high", "xhigh", "max"}
MAX_IMAGES = 2
MAX_TEXT_CHARS = 2000
MAX_SYSTEM_CHARS = 20000

TUTOR_NOTE = (
    "Tutor mode: the student wants to learn the method. Write the steps as small hints that "
    "build on each other, each one revealing a little more, so the student could finish alone "
    "after any step. The answer field still holds the final answer (shown last)."
)


class BadRequest(Exception):
    pass


def clean_request(body):
    """The calculator's JSON -> (system, user_content, output_format, effort, tutor). Raises BadRequest."""
    if not isinstance(body, dict):
        raise BadRequest("body must be a JSON object")
    tutor = bool(body.get("tutor", False))
    system = body.get("system", "")
    if not isinstance(system, str) or not system or len(system) > MAX_SYSTEM_CHARS:
        raise BadRequest("system prompt missing or too long")
    messages = body.get("messages")
    if not isinstance(messages, list) or len(messages) != 1 or messages[0].get("role") != "user":
        raise BadRequest("exactly one user message expected")
    content = messages[0].get("content")
    if not isinstance(content, list) or not content:
        raise BadRequest("message content must be a list")
    images, clean = 0, []
    for block in content:
        t = block.get("type") if isinstance(block, dict) else None
        if t == "image":
            src = block.get("source", {})
            if src.get("type") != "base64" or src.get("media_type") not in ("image/jpeg", "image/png"):
                raise BadRequest("images must be base64 JPEG or PNG")
            images += 1
            clean.append({"type": "image", "source": {"type": "base64", "media_type": src["media_type"],
                                                      "data": src.get("data", "")}})
        elif t == "text":
            text = block.get("text", "")
            if not isinstance(text, str) or len(text) > MAX_TEXT_CHARS:
                raise BadRequest("text block too long")
            clean.append({"type": "text", "text": text})
        else:
            raise BadRequest(f"content block type {t!r} not allowed")
    if images == 0 or images > MAX_IMAGES:
        raise BadRequest("1 or 2 photos expected")
    if tutor:
        clean.append({"type": "text", "text": TUTOR_NOTE})
    oc = body.get("output_config") or {}
    fmt = oc.get("format")
    if not isinstance(fmt, dict) or fmt.get("type") != "json_schema" or not isinstance(fmt.get("schema"), dict):
        raise BadRequest("output_config.format json_schema expected")
    effort = oc.get("effort", "high")
    if effort not in ALLOWED_EFFORT:
        effort = "high"
    return system, clean, fmt, effort, tutor


def main_params(system, content, fmt, effort):
    """Request for the main model (streamed straight back to the calculator)."""
    return {
        "model": config.MAIN_MODEL,
        "max_tokens": config.MAX_TOKENS,
        "thinking": {"type": "adaptive"},
        "system": system,
        "output_config": {"effort": effort, "format": fmt},
        "messages": [{"role": "user", "content": content}],
    }


def haiku_params(system, content, fmt):
    """Request for the cheap first try. Haiku 4.5 takes no `effort` and no adaptive thinking;
    structured outputs work. Not streamed: the proxy reads the answer before deciding."""
    return {
        "model": config.HAIKU_MODEL,
        "max_tokens": 16000,
        "system": system,
        "output_config": {"format": fmt},
        "messages": [{"role": "user", "content": content}],
    }


def client():
    import anthropic  # imported here so the request checks can be tested without the SDK

    if not config.ANTHROPIC_API_KEY:
        raise RuntimeError("ANTHROPIC_API_KEY is not set on the server")
    return anthropic.Anthropic(api_key=config.ANTHROPIC_API_KEY, max_retries=2)


def open_main_stream(params):
    """Opens the streamed main-model request and returns (context_manager, response).
    The caller iterates response.iter_bytes() and must call cm.__exit__(None, None, None).
    Raises anthropic.APIStatusError / APIConnectionError before any byte is sent."""
    c = client()
    if config.REFUSAL_FALLBACK:
        cm = c.beta.messages.with_streaming_response.create(
            **params, stream=True, betas=["server-side-fallback-2026-07-01"], extra_body={"fallbacks": "default"})
    else:
        cm = c.messages.with_streaming_response.create(**params, stream=True)
    resp = cm.__enter__()
    return cm, resp


def try_haiku(params):
    """Haiku's reply text if it is confident enough to stand on its own, else None."""
    import anthropic

    try:
        msg = client().messages.create(**params)
    except anthropic.APIError:
        return None
    if msg.stop_reason != "end_turn":
        return None
    text = "".join(b.text for b in msg.content if b.type == "text")
    try:
        reply = json.loads(text)
    except ValueError:
        return None
    if not reply.get("readable") or float(reply.get("confidence", 0)) < config.HAIKU_MIN_CONFIDENCE:
        return None
    if reply.get("unclear"):
        return None  # anything Haiku flags goes to the main model
    return text


def sse(event, data):
    return f"event: {event}\ndata: {json.dumps(data, separators=(',', ':'))}\n\n".encode()


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


def error_body(error_type, message):
    return {"type": "error", "error": {"type": error_type, "message": message}}
