"""Settings for the AI Calculator proxy, all from environment variables (or a .env file).

Nothing secret lives in this repository: ANTHROPIC_API_KEY is read from the server's
environment only. See README.md for every variable.
"""
import os


def _load_dotenv(path=os.path.join(os.path.dirname(__file__), ".env")):
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


def _bool(name, default):
    return os.environ.get(name, "1" if default else "0").strip().lower() in ("1", "true", "yes", "on")


# The Claude API key: ONLY here, on the server. Never in the calculator, never in git.
ANTHROPIC_API_KEY = os.environ.get("ANTHROPIC_API_KEY", "")

# Models. The calculator asks for the main model; "haiku-first" routing tries the
# cheaper model first and only falls back to the main model when Haiku is unsure.
MAIN_MODEL = os.environ.get("MAIN_MODEL", "claude-sonnet-5-5")
HAIKU_MODEL = os.environ.get("HAIKU_MODEL", "claude-haiku-4-5-20251001")
ROUTING = os.environ.get("ROUTING", "main")  # "main" or "haiku-first"
HAIKU_MIN_CONFIDENCE = float(os.environ.get("HAIKU_MIN_CONFIDENCE", "0.85"))
# Server-side refusal fallback on the main model (Claude API beta). Set 0 if your
# account or region rejects it.
REFUSAL_FALLBACK = _bool("REFUSAL_FALLBACK", True)
MAX_TOKENS = _int("MAX_TOKENS", 32000)

# Fair use and pricing (the price isn't final: these are switches, not code).
MONTHLY_CAP = _int("MONTHLY_CAP", 500)        # solves per account per calendar month
TRIAL_DAYS = _int("TRIAL_DAYS", 30)           # first month free, from the day the calculator is linked
FREE_DAILY = _int("FREE_DAILY", 0)            # free tier: N solves/day without a subscription (0 = off)
FREE_DAILY_UNLINKED = _bool("FREE_DAILY_UNLINKED", False)  # free tier also before linking

# Where people link a calculator (shown on the device with the pairing code).
PUBLIC_URL = os.environ.get("PUBLIC_URL", "https://calc-proxy.example.com").rstrip("/")
PAIR_CODE_MINUTES = _int("PAIR_CODE_MINUTES", 15)

# Shared secret for the billing webhook (Stripe or similar sets paid_until).
BILLING_SECRET = os.environ.get("BILLING_SECRET", "")
# Admin endpoints (create devices from a browser / script). Empty = admin API off;
# use admin.py on the server instead.
ADMIN_TOKEN = os.environ.get("ADMIN_TOKEN", "")

DB_PATH = os.environ.get("DB_PATH", os.path.join(os.path.dirname(__file__), "proxy.db"))
# Over-the-air firmware: firmware.bin + firmware.json written by `admin.py firmware` (see firmware.py).
FIRMWARE_DIR = os.environ.get("FIRMWARE_DIR") or os.path.join(os.path.dirname(__file__), "firmware")
MAX_BODY_BYTES = _int("MAX_BODY_BYTES", 8 * 1024 * 1024)
