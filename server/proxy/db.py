"""SQLite storage: devices (hashed tokens), accounts, pairing codes, usage counters.

Small on purpose: one file, no ORM. Every function opens its own connection, so it is
safe to call from FastAPI's worker threads.
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
  trial_ends_at INTEGER NOT NULL,      -- first month free (from linking)
  paid_until INTEGER NOT NULL DEFAULT 0 -- set by the billing webhook / admin
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
"""


@contextmanager
def conn():
    c = sqlite3.connect(config.DB_PATH, timeout=10)
    c.row_factory = sqlite3.Row
    try:
        yield c
        c.commit()
    finally:
        c.close()


def init():
    with conn() as c:
        c.executescript(SCHEMA)


def now():
    return int(time.time())


def month(ts=None):
    return datetime.fromtimestamp(ts or now(), timezone.utc).strftime("%Y-%m")


def day(ts=None):
    return datetime.fromtimestamp(ts or now(), timezone.utc).strftime("%Y-%m-%d")


def hash_token(token):
    return hashlib.sha256(token.encode()).hexdigest()


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
        return c.execute("SELECT * FROM accounts WHERE email=?", (email.strip().lower(),)).fetchone()


def get_or_create_account(email):
    email = email.strip().lower()
    a = account_by_email(email)
    if a:
        return a
    with conn() as c:
        c.execute("INSERT INTO accounts (email, created_at, trial_ends_at) VALUES (?,?,?)",
                  (email, now(), now() + config.TRIAL_DAYS * 86400))
    return account_by_email(email)


def set_paid_until(email, paid_until):
    with conn() as c:
        return c.execute("UPDATE accounts SET paid_until=? WHERE email=?", (int(paid_until), email.strip().lower())).rowcount


def subscription_state(a):
    """'trial', 'active' or 'inactive' for an account row."""
    t = now()
    if a["paid_until"] > t:
        return "active"
    if a["trial_ends_at"] > t:
        return "trial"
    return "inactive"


# ---- pairing
_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"  # no 0/O, 1/I: easy to read off an e-paper screen


def new_pair_code(device_id):
    code = "".join(secrets.choice(_ALPHABET) for _ in range(6))
    with conn() as c:
        c.execute("DELETE FROM pair_codes WHERE device_id=? OR expires_at<?", (device_id, now()))
        c.execute("INSERT INTO pair_codes (code, device_id, expires_at) VALUES (?,?,?)",
                  (code, device_id, now() + config.PAIR_CODE_MINUTES * 60))
    return code


def current_pair_code(device_id):
    with conn() as c:
        r = c.execute("SELECT code FROM pair_codes WHERE device_id=? AND expires_at>?", (device_id, now())).fetchone()
    return r["code"] if r else new_pair_code(device_id)


def claim_pair_code(code, email):
    """Links the code's device to the account for `email` (created on first use: its free month starts).
    Returns the device id, or None for an unknown / expired code."""
    code = code.strip().upper().replace("-", "").replace(" ", "")
    with conn() as c:
        r = c.execute("SELECT device_id FROM pair_codes WHERE code=? AND expires_at>?", (code, now())).fetchone()
    if not r:
        return None
    a = get_or_create_account(email)
    with conn() as c:
        c.execute("UPDATE devices SET account_id=? WHERE id=?", (a["id"], r["device_id"]))
        c.execute("DELETE FROM pair_codes WHERE code=?", (code,))
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
