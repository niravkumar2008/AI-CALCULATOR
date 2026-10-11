"""Settings for the AI Calculator proxy, all from environment variables (or a .env file).

Nothing secret lives in this repository: ANTHROPIC_API_KEY, LINK_SECRET, STRIPE_WEBHOOK_SECRET,
ADMIN_TOKEN and SMTP_PASSWORD are read from the server's environment only. See README.md.
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))


def _load_dotenv(path=os.path.join(HERE, ".env")):
    """Tiny .env reader (KEY=value lines) so no extra package is needed."""
    if not os.path.exists(path):
        return
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        k, v = line.split("=", 1)
        os.environ.setdefault(k.strip(), v.strip().strip('"').strip("'"))


_load_dotenv()


def _int(name, default):
    try:
        return int(os.environ.get(name, default))
    except ValueError:
        return default


def _float(name, default):
    try:
        return float(os.environ.get(name, default))
    except ValueError:
        return default


def _bool(name, default):
    return os.environ.get(name, "1" if default else "0").strip().lower() in ("1", "true", "yes", "on")


VERSION = os.environ.get("APP_VERSION", "proxy-2026.10.10")

# The Claude API key: ONLY here, on the server. Never in the calculator, never in git.
ANTHROPIC_API_KEY = os.environ.get("ANTHROPIC_API_KEY", "")

# ---- Models and tiers (hardware/production/UNIT_ECONOMICS.md)
# base tier ($15/month): Sonnet 5.5 (~$3.77 typical, $12.40 at the 500 cap: covers the cap).
# pro tier (optional):   Opus 5.5   (~$7.52 typical, $24.72 at 500: loses money at $15 above ~280 solves).
# MAIN_MODEL is the old name of BASE_MODEL and still works.
BASE_MODEL = os.environ.get("BASE_MODEL") or os.environ.get("MAIN_MODEL") or "claude-sonnet-5-5"
PRO_MODEL = os.environ.get("PRO_MODEL", "claude-opus-5-5")
MAIN_MODEL = BASE_MODEL  # kept for older scripts
# On 429 / 5xx / 529 overloaded / connection errors, try the other tier's model once before
# telling the calculator "busy". 0 = off.
MODEL_FALLBACK = _bool("MODEL_FALLBACK", True)
# SDK retries per model before falling back (the SDK backs off between tries).
CLAUDE_MAX_RETRIES = _int("CLAUDE_MAX_RETRIES", 1)
CLAUDE_TIMEOUT_S = _float("CLAUDE_TIMEOUT_S", 600.0)
HAIKU_MODEL = os.environ.get("HAIKU_MODEL", "claude-haiku-4-5")
ROUTING = os.environ.get("ROUTING", "main")  # "main" or "haiku-first"
HAIKU_MIN_CONFIDENCE = _float("HAIKU_MIN_CONFIDENCE", 0.85)
# Server-side refusal fallback (Claude API beta, "default" form). Set 0 if your account rejects it.
REFUSAL_FALLBACK = _bool("REFUSAL_FALLBACK", True)
# Prompt caching of the fixed system prompt (cache reads cost 0.1x input on Sonnet/Opus 5.5).
PROMPT_CACHE = _bool("PROMPT_CACHE", True)
MAX_TOKENS = _int("MAX_TOKENS", 32000)

# ---- The system prompt is the server's (versioned file), not the calculator's.
SYSTEM_PROMPT_FILE = os.environ.get("SYSTEM_PROMPT_FILE") or os.path.join(HERE, "prompts", "solver_v1.txt")
SCHEMA_FILE = os.environ.get("SCHEMA_FILE") or os.path.join(HERE, "prompts", "solver_v1.schema.json")
# "server" (default): ignore the calculator's system prompt and JSON schema.
# "device": use what the calculator sends (bench testing a prompt change before publishing it).
PROMPT_SOURCE = os.environ.get("PROMPT_SOURCE", "server")

# ---- Fair use, tiers and pricing (the price isn't final: these are switches, not code)
MONTHLY_CAP = _int("MONTHLY_CAP", 500)          # base tier: solves per account per calendar month
PRO_MONTHLY_CAP = _int("PRO_MONTHLY_CAP", 280)  # pro tier on Opus 5.5 (break-even at $15 is about 280)
TRIAL_DAYS = _int("TRIAL_DAYS", 30)             # first month free, from the day the calculator is linked
PAST_DUE_GRACE_DAYS = _int("PAST_DUE_GRACE_DAYS", 3)  # a failed card keeps working this long
FREE_DAILY = _int("FREE_DAILY", 0)              # free tier: N solves/day without a subscription (0 = off)
FREE_DAILY_UNLINKED = _bool("FREE_DAILY_UNLINKED", False)

# ---- Per-device burst limits (in this process: run ONE worker, see README)
SOLVE_CONCURRENCY = _int("SOLVE_CONCURRENCY", 1)     # solves in flight per calculator
SOLVES_PER_MINUTE = _int("SOLVES_PER_MINUTE", 4)
SOLVES_PER_HOUR = _int("SOLVES_PER_HOUR", 40)
OTHER_PER_MINUTE = _int("OTHER_PER_MINUTE", 30)     # pairing / status / firmware checks
LINK_PER_MINUTE_PER_IP = _int("LINK_PER_MINUTE_PER_IP", 10)

# SSE comment sent while Claude is quiet, so the calculator's 120 s idle timeout never fires.
KEEPALIVE_SECONDS = _float("KEEPALIVE_SECONDS", 15.0)

# ---- Linking a calculator to an e-mail
PUBLIC_URL = os.environ.get("PUBLIC_URL", "https://calc-proxy.example.com").rstrip("/")
PAIR_CODE_MINUTES = _int("PAIR_CODE_MINUTES", 15)
LINK_CONFIRM_MINUTES = _int("LINK_CONFIRM_MINUTES", 30)  # life of the e-mailed confirmation link
# HMAC key for the confirmation links. Empty = a random key per start (links die on restart).
LINK_SECRET = os.environ.get("LINK_SECRET", "")
# "email" (default): the link page e-mails a confirmation link; nothing is linked until it is used.
# "open": code + e-mail links at once (bench testing only; anyone with a code picks the e-mail).
LINK_MODE = os.environ.get("LINK_MODE", "email")
# Mail: "console" (logs that a mail was sent, prints the link only if MAIL_LOG_LINKS=1) or "smtp".
MAIL_BACKEND = os.environ.get("MAIL_BACKEND", "console")
MAIL_LOG_LINKS = _bool("MAIL_LOG_LINKS", False)
MAIL_FROM = os.environ.get("MAIL_FROM", "AI Calculator <no-reply@example.com>")
SMTP_HOST = os.environ.get("SMTP_HOST", "")
SMTP_PORT = _int("SMTP_PORT", 587)
SMTP_USER = os.environ.get("SMTP_USER", "")
SMTP_PASSWORD = os.environ.get("SMTP_PASSWORD", "")

# ---- Billing (Stripe-signed webhook, billing.py)
STRIPE_WEBHOOK_SECRET = os.environ.get("STRIPE_WEBHOOK_SECRET", "")  # whsec_...
WEBHOOK_TOLERANCE_S = _int("WEBHOOK_TOLERANCE_S", 300)
# Stripe price lookup_key (or subscription metadata "tier") that means the pro tier.
PRO_PRICE_KEYS = {k.strip() for k in os.environ.get("PRO_PRICE_KEYS", "pro,calc_pro_monthly").split(",") if k.strip()}

# Admin endpoints (create devices from a script). Empty = admin API off; use admin.py on the server.
ADMIN_TOKEN = os.environ.get("ADMIN_TOKEN", "")

DB_PATH = os.environ.get("DB_PATH", os.path.join(HERE, "proxy.db"))
# Over-the-air firmware: firmware.bin + firmware.json written by `admin.py firmware` (see firmware.py).
FIRMWARE_DIR = os.environ.get("FIRMWARE_DIR") or os.path.join(HERE, "firmware")
MAX_BODY_BYTES = _int("MAX_BODY_BYTES", 8 * 1024 * 1024)
LOG_LEVEL = os.environ.get("LOG_LEVEL", "INFO")
