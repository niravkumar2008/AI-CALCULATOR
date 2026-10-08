"""Who may solve right now: device link, subscription, monthly fair-use cap, free tier.

Pure decisions on database rows, so they are easy to test. Error types and messages
are what the calculator shows (core/claude_api.cpp: classifyFailure), so keep messages
short (the e-paper wraps them at 25 characters, about 110 characters in all).
"""
from dataclasses import dataclass, field

import config
import db


@dataclass
class Decision:
    ok: bool
    status: int = 200
    error_type: str = ""
    message: str = ""
    # usage counters to bump after a successful solve: [(subject, period), ...]
    count: list = field(default_factory=list)


def _deny(status, error_type, message):
    return Decision(False, status, error_type, message)


def decide(device, ts=None):
    """Decision for one solve by this device row."""
    month, day = db.month(ts), db.day(ts)
    if device["account_id"] is None:
        subject = "dev:" + device["id"]
        if config.FREE_DAILY > 0 and config.FREE_DAILY_UNLINKED:
            if db.used(subject, day) >= config.FREE_DAILY:
                return _deny(429, "free_tier_exhausted",
                             f"Free solves used for today. Link for more: {link_hint(device)}")
            return Decision(True, count=[(subject, day), (subject, month)])
        return _deny(402, "device_not_linked", f"Link this calculator: {link_hint(device)}")

    account = db.account(device["account_id"])
    subject = "acct:" + str(account["id"])
    state = db.subscription_state(account)
    if state in ("trial", "active"):
        if db.used(subject, month) >= config.MONTHLY_CAP:
            return _deny(429, "fair_use_exceeded",
                         f"Fair use: {config.MONTHLY_CAP} AI solves used this month. More on the 1st.")
        return Decision(True, count=[(subject, month)])
    if config.FREE_DAILY > 0:
        if db.used(subject, day) >= config.FREE_DAILY:
            return _deny(429, "free_tier_exhausted",
                         f"{config.FREE_DAILY} free solves used today. Subscribe at {short_url()} for more.")
        return Decision(True, count=[(subject, day), (subject, month)])
    return _deny(402, "subscription_inactive", f"Free month over. Subscribe at {short_url()} to keep AI solving.")


def short_url():
    return config.PUBLIC_URL.replace("https://", "").replace("http://", "")


def link_hint(device):
    return f"{short_url()}/link code {db.current_pair_code(device['id'])}"


def status_json(device, ts=None):
    """What `account` on the calculator (and the link page) shows."""
    out = {"device_id": device["id"], "linked": device["account_id"] is not None,
           "monthly_cap": config.MONTHLY_CAP, "free_daily": config.FREE_DAILY}
    if device["account_id"] is None:
        out["pair_code"] = db.current_pair_code(device["id"])
        out["link_url"] = config.PUBLIC_URL + "/link"
        return out
    a = db.account(device["account_id"])
    out.update({
        "account": a["email"],
        "subscription": db.subscription_state(a),
        "trial_ends_at": a["trial_ends_at"],
        "paid_until": a["paid_until"],
        "solves_this_month": db.used("acct:" + str(a["id"]), db.month(ts)),
    })
    return out
