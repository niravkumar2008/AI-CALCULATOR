"""Who may solve right now: device link, subscription state, monthly fair-use cap per tier, free tier.

Pure decisions on database rows, so they are easy to test. Error types and messages
are what the calculator shows (core/claude_api.cpp: classifyFailure), so keep messages
short (<= 110 characters). The calculator shows the message for exactly these types:
device_unknown, device_not_linked, subscription_inactive, fair_use_exceeded,
free_tier_exhausted. Anything else with 429/5xx only shows "Claude is busy".
"""
from dataclasses import dataclass, field

import config
import db

# The burst limiter (limits.py) answers with this type so the calculator shows the message
# ("wait a minute") instead of a generic "busy". A dedicated "rate_limited" type needs a
# firmware change first (classifyFailure's list); see FINAL_REVIEW_server.md.
RATE_LIMIT_ERROR_TYPE = "fair_use_exceeded"

RATE_MESSAGES = {
    "concurrency": "This calculator is already solving a photo. Wait for that answer, then try again.",
    "minute": "Too many AI solves in a minute. Wait a moment, then try again.",
    "hour": "Too many AI solves this hour. Take a short break, then try again.",
    "other": "Too many requests from this calculator. Wait a minute, then try again.",
}


@dataclass
class Decision:
    ok: bool
    status: int = 200
    error_type: str = ""
    message: str = ""
    tier: str = "base"
    # usage counters to bump after a successful solve: [(subject, period), ...]
    count: list = field(default_factory=list)


def _deny(status, error_type, message):
    return Decision(False, status, error_type, message[:110])


def cap_for(tier):
    return config.PRO_MONTHLY_CAP if tier == "pro" else config.MONTHLY_CAP


def model_for(tier):
    return config.PRO_MODEL if tier == "pro" else config.BASE_MODEL


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
    tier = db.account_tier(account)
    state = db.subscription_state(account, ts)
    allowed = state in ("trial", "active") or (state == "past_due" and account["paid_until"] > (ts or db.now()))
    if allowed:
        cap = cap_for(tier)
        if db.used(subject, month) >= cap:
            return _deny(429, "fair_use_exceeded", f"Fair use: {cap} AI solves used this month. More on the 1st.")
        return Decision(True, tier=tier, count=[(subject, month)])
    if config.FREE_DAILY > 0:
        if db.used(subject, day) >= config.FREE_DAILY:
            return _deny(429, "free_tier_exhausted",
                         f"{config.FREE_DAILY} free solves used today. Subscribe at {short_url()} for more.")
        return Decision(True, count=[(subject, day), (subject, month)])
    if state == "past_due":
        return _deny(402, "subscription_inactive", f"Payment failed. Update your card at {short_url()} to keep AI solving.")
    if state == "canceled":
        return _deny(402, "subscription_inactive", f"Subscription ended. Subscribe at {short_url()} to keep AI solving.")
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
    tier = db.account_tier(a)
    out.update({
        "account": a["email"],
        "subscription": db.subscription_state(a, ts),
        "tier": tier,
        "monthly_cap": cap_for(tier),
        "trial_ends_at": a["trial_ends_at"],
        "paid_until": a["paid_until"],
        "solves_this_month": db.used("acct:" + str(a["id"]), db.month(ts)),
    })
    return out
