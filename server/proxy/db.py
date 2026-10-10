"""SQLite storage: devices (hashed tokens), accounts, pairing codes, usage counters, billing events.

Small on purpose: one file, no ORM. Every function opens its own connection, so it is
safe to call from FastAPI's worker threads. Multi-step changes run in one
`BEGIN IMMEDIATE` transaction (tx()), so two requests can't interleave them.
"""
import hashlib
import secrets
import sqlite3
import time
from contextlib import contextmanager
from datetime import datetime, timezone

import config

SCHEMA = """
CREATE TABLE IF NOT EXISTS accounts (
  id INTEGER PRIMARY KEY,
  email TEXT UNIQUE NOT NULL,
  created_at INTEGER NOT NULL,
  trial_ends_at INTEGER NOT NULL,       -- first month free (from linking)
  paid_until INTEGER NOT NULL DEFAULT 0 -- end of the paid period (billing webhook / admin)
);
CREATE TABLE IF NOT EXISTS devices (
  id TEXT PRIMARY KEY,                 -- short public id, e.g. dev_8f3a...
  token_hash TEXT UNIQUE NOT NULL,     -- sha256 of the device token (the token itself is never stored)
  label TEXT,
  hw_id TEXT,                          -- calc-<mac>, reported by the firmware
  account_id INTEGER REFERENCES accounts(id),
  created_at INTEGER NOT NULL,
  revoked INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS pair_codes (
  code TEXT PRIMARY KEY,
  device_id TEXT NOT NULL REFERENCES devices(id),
  expires_at INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS usage (
  subject TEXT NOT NULL,               -- "acct:<id>" or "dev:<id>"
  period TEXT NOT NULL,                -- "2026-10" (month) or "2026-10-04" (day)
  count INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY (subject, period)
);
CREATE TABLE IF NOT EXISTS billing_events (
  id TEXT PRIMARY KEY,                 -- the provider's event id (replays are ignored)
  type TEXT NOT NULL,
  received_at INTEGER NOT NULL
);
"""

# Columns added after the first release: (table, column, definition). Applied by init().
MIGRATIONS = [
    ("accounts", "tier", "TEXT NOT NULL DEFAULT 'base'"),        # 'base' (Sonnet) or 'pro' (Opus)
    ("accounts", "sub_status", "TEXT NOT NULL DEFAULT ''"),      # '', trialing, active, past_due, canceled
    ("accounts", "billing_customer", "TEXT"),                    # Stripe customer id (cus_...)
    ("devices", "first_linked_at", "INTEGER"),                   # one free month per calculator, not per e-mail
]

TIERS = ("base", "pro")


@contextmanager
def conn():
    c = sqlite3.connect(config.DB_PATH, timeout=10)
    c.row_factory = sqlite3.Row
    try:
        yield c
        c.commit()
    finally:
        c.close()


@contextmanager
def tx():
    """One atomic read-modify-write: BEGIN IMMEDIATE takes the write lock up front."""
    c = sqlite3.connect(config.DB_PATH, timeout=10, isolation_level=None)
    c.row_factory = sqlite3.Row
    try:
        c.execute("BEGIN IMMEDIATE")
        try:
            yield c
        except BaseException:
            c.execute("ROLLBACK")
            raise
        c.execute("COMMIT")
    finally:
        c.close()


def init():
    with conn() as c:
        c.execute("PRAGMA journal_mode=WAL")
        c.executescript(SCHEMA)
        for table, column, definition in MIGRATIONS:
            have = {r["name"] for r in c.execute(f"PRAGMA table_info({table})")}
            if column not in have:
                c.execute(f"ALTER TABLE {table} ADD COLUMN {column} {definition}")
        c.execute("CREATE INDEX IF NOT EXISTS accounts_customer ON accounts(billing_customer)")


def now():
    return int(time.time())


def month(ts=None):
    return datetime.fromtimestamp(ts or now(), timezone.utc).strftime("%Y-%m")


def day(ts=None):
    return datetime.fromtimestamp(ts or now(), timezone.utc).strftime("%Y-%m-%d")


def hash_token(token):
    return hashlib.sha256(token.encode()).hexdigest()


def normalize_email(email):
    return (email or "").strip().lower()


# ---- devices
def create_device(label=""):
    """Returns (device_id, token). The token is shown once: type it into the calculator."""
    device_id = "dev_" + secrets.token_hex(6)
    token = "dt_" + secrets.token_urlsafe(30)
    with conn() as c:
        c.execute("INSERT INTO devices (id, token_hash, label, created_at) VALUES (?,?,?,?)",
                  (device_id, hash_token(token), label, now()))
    return device_id, token


def device_by_token(token):
    if not token:
        return None
    with conn() as c:
        return c.execute("SELECT * FROM devices WHERE token_hash=? AND revoked=0", (hash_token(token),)).fetchone()


def note_hw_id(device_id, hw_id):
    if hw_id:
        with conn() as c:
            c.execute("UPDATE devices SET hw_id=? WHERE id=? AND (hw_id IS NULL OR hw_id='')", (hw_id[:40], device_id))


def revoke_device(device_id):
    with conn() as c:
        return c.execute("UPDATE devices SET revoked=1 WHERE id=?", (device_id,)).rowcount


def list_devices():
    with conn() as c:
        return c.execute("SELECT d.*, a.email FROM devices d LEFT JOIN accounts a ON a.id=d.account_id "
                         "ORDER BY d.created_at").fetchall()


# ---- accounts and subscription
def account(account_id):
    with conn() as c:
        return c.execute("SELECT * FROM accounts WHERE id=?", (account_id,)).fetchone()


def account_by_email(email):
    with conn() as c:
        return c.execute("SELECT * FROM accounts WHERE email=?", (normalize_email(email),)).fetchone()


def account_by_customer(customer_id):
    if not customer_id:
        return None
    with conn() as c:
        return c.execute("SELECT * FROM accounts WHERE billing_customer=?", (customer_id,)).fetchone()


def _get_or_create_account(c, email, trial_ends_at):
    c.execute("INSERT OR IGNORE INTO accounts (email, created_at, trial_ends_at) VALUES (?,?,?)",
              (email, now(), trial_ends_at))
    return c.execute("SELECT * FROM accounts WHERE email=?", (email,)).fetchone()


def get_or_create_account(email):
    with tx() as c:
        return _get_or_create_account(c, normalize_email(email), now() + config.TRIAL_DAYS * 86400)


def set_paid_until(email, paid_until):
    with conn() as c:
        return c.execute("UPDATE accounts SET paid_until=? WHERE email=?",
                         (int(paid_until), normalize_email(email))).rowcount


def set_tier(email, tier):
    if tier not in TIERS:
        raise ValueError(f"tier must be one of {', '.join(TIERS)}")
    with conn() as c:
        return c.execute("UPDATE accounts SET tier=? WHERE email=?", (tier, normalize_email(email))).rowcount


def update_billing(account_id, **fields):
    """Sets billing columns (sub_status, paid_until, tier, billing_customer) on one account."""
    allowed = {"sub_status", "paid_until", "tier", "billing_customer"}
    fields = {k: v for k, v in fields.items() if k in allowed and v is not None}
    if not fields:
        return 0
    sets = ", ".join(f"{k}=?" for k in fields)
    with conn() as c:
        return c.execute(f"UPDATE accounts SET {sets} WHERE id=?", (*fields.values(), account_id)).rowcount


def subscription_state(a, ts=None):
    """'trial', 'active', 'past_due', 'canceled' or 'inactive' for an account row.

    - active:   paid period running (Stripe active, or `admin.py paid`). A subscription
                canceled at period end stays active until that end.
    - past_due: the card failed; still allowed for PAST_DUE_GRACE_DAYS after the period end.
    - trial:    the first free month (from linking), or Stripe's own trial.
    - canceled: had a subscription, it ended.  inactive: free month over, never paid.
    """
    t = ts or now()
    keys = a.keys()
    status = a["sub_status"] if "sub_status" in keys else ""
    paid_until = a["paid_until"] or 0
    if status == "past_due":
        return "past_due"
    if paid_until > t:
        return "trial" if status == "trialing" else "active"
    if a["trial_ends_at"] > t:
        return "trial"
    return "canceled" if status in ("canceled", "past_due") else "inactive"


def account_tier(a):
    return a["tier"] if "tier" in a.keys() and a["tier"] in TIERS else "base"


# ---- pairing
_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"  # no 0/O, 1/I: easy to read off a small screen


def clean_code(code):
    return (code or "").strip().upper().replace("-", "").replace(" ", "")[:12]


def new_pair_code(device_id):
    code = "".join(secrets.choice(_ALPHABET) for _ in range(6))
    with tx() as c:
        c.execute("DELETE FROM pair_codes WHERE device_id=? OR expires_at<?", (device_id, now()))
        c.execute("INSERT INTO pair_codes (code, device_id, expires_at) VALUES (?,?,?)",
                  (code, device_id, now() + config.PAIR_CODE_MINUTES * 60))
    return code


def current_pair_code(device_id):
    with conn() as c:
        r = c.execute("SELECT code FROM pair_codes WHERE device_id=? AND expires_at>?", (device_id, now())).fetchone()
    return r["code"] if r else new_pair_code(device_id)


def pair_code_device(code):
    """The device id of a valid (unexpired) code, else None. Does not use the code up."""
    with conn() as c:
        r = c.execute("SELECT device_id FROM pair_codes WHERE code=? AND expires_at>?", (clean_code(code), now())).fetchone()
    return r["device_id"] if r else None


def extend_pair_code(code, until):
    """Keeps a code alive while its e-mail confirmation is pending (never shortens it)."""
    with conn() as c:
        c.execute("UPDATE pair_codes SET expires_at=MAX(expires_at, ?) WHERE code=? AND expires_at>?",
                  (int(until), clean_code(code), now()))


def claim_pair_code(code, email):
    """Links the code's device to the account for `email` (created on first use) and uses the code up.
    Atomic: of two simultaneous claims of one code, exactly one wins. The free month is per
    calculator: relinking a calculator to a new e-mail does not start another free month.
    Returns the device id, or None for an unknown / expired / already used code."""
    code, email, t = clean_code(code), normalize_email(email), now()
    with tx() as c:
        r = c.execute("SELECT device_id FROM pair_codes WHERE code=? AND expires_at>?", (code, t)).fetchone()
        if not r:
            return None
        c.execute("DELETE FROM pair_codes WHERE code=?", (code,))
        dev = c.execute("SELECT first_linked_at FROM devices WHERE id=?", (r["device_id"],)).fetchone()
        first = dev["first_linked_at"] if dev and dev["first_linked_at"] else t
        a = _get_or_create_account(c, email, min(t, first) + config.TRIAL_DAYS * 86400)
        c.execute("UPDATE devices SET account_id=?, first_linked_at=COALESCE(first_linked_at, ?) WHERE id=?",
                  (a["id"], t, r["device_id"]))
    return r["device_id"]


# ---- usage
def used(subject, period):
    with conn() as c:
        r = c.execute("SELECT count FROM usage WHERE subject=? AND period=?", (subject, period)).fetchone()
    return r["count"] if r else 0


def count_solve(subject, period):
    with conn() as c:
        c.execute("INSERT INTO usage (subject, period, count) VALUES (?,?,1) "
                  "ON CONFLICT(subject, period) DO UPDATE SET count=count+1", (subject, period))


# ---- billing events (idempotency)
def first_time_event(event_id, event_type):
    """True the first time this provider event id is seen; replays return False."""
    with conn() as c:
        return c.execute("INSERT OR IGNORE INTO billing_events (id, type, received_at) VALUES (?,?,?)",
                         (event_id, event_type, now())).rowcount == 1


def forget_event(event_id):
    with conn() as c:
        c.execute("DELETE FROM billing_events WHERE id=?", (event_id,))
