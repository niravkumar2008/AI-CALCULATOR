"""Billing: a signed webhook from the payment provider sets each account's subscription.

Two parts, so another provider can be added later without touching the rest:
  WebhookVerifier   checks the signature and returns the event (StripeVerifier: Stripe's
                    "Stripe-Signature: t=<unix>,v1=<hex HMAC-SHA256 of '<t>.<raw body>'>"
                    scheme, with a timestamp tolerance against replays)
  apply_event()     turns a verified event into account changes (state, paid_until, tier)

Subscription states (db.subscription_state): trial (first month free from linking, or
Stripe "trialing"), active, past_due (card failed: PAST_DUE_GRACE_DAYS more days), canceled.
The first month is free on the proxy's side; create the Stripe subscription with
trial_end = the account's trial_ends_at (GET /v1/device/status shows it) so the first
charge comes after the free month.

No network calls here: the webhook carries everything needed.
"""
import hashlib
import hmac
import json
import logging
import time

import config
import db

log = logging.getLogger("calc-proxy")


class SignatureError(Exception):
    pass


class WebhookVerifier:
    """Interface: verify(raw_body: bytes, headers: Mapping) -> event dict, or raise SignatureError."""

    def verify(self, payload, headers):
        raise NotImplementedError


class StripeVerifier(WebhookVerifier):
    header = "stripe-signature"

    def __init__(self, secret, tolerance_s=300, clock=time.time):
        self.secret, self.tolerance, self.clock = secret, tolerance_s, clock

    @staticmethod
    def sign(secret, payload, timestamp):
        signed = f"{int(timestamp)}.".encode() + payload
        return hmac.new(secret.encode(), signed, hashlib.sha256).hexdigest()

    def verify(self, payload, headers):
        if not self.secret:
            raise SignatureError("webhook secret not configured")
        sig_header = headers.get(self.header, "") or ""
        timestamp, sigs = None, []
        for part in sig_header.split(","):
            k, _, v = part.strip().partition("=")
            if k == "t":
                try:
                    timestamp = int(v)
                except ValueError:
                    raise SignatureError("bad timestamp") from None
            elif k == "v1":
                sigs.append(v)
        if timestamp is None or not sigs:
            raise SignatureError("missing signature")
        if abs(self.clock() - timestamp) > self.tolerance:
            raise SignatureError("timestamp outside tolerance")
        want = self.sign(self.secret, payload, timestamp)
        if not any(hmac.compare_digest(want, s) for s in sigs):
            raise SignatureError("signature mismatch")
        try:
            event = json.loads(payload)
        except ValueError:
            raise SignatureError("body is not JSON") from None
        if not isinstance(event, dict) or not event.get("id") or not event.get("type"):
            raise SignatureError("not an event")
        return event


def verifier():
    return StripeVerifier(config.STRIPE_WEBHOOK_SECRET, config.WEBHOOK_TOLERANCE_S)


# ---- events -> accounts
STATUS_MAP = {
    "trialing": "trialing",
    "active": "active",
    "past_due": "past_due",
    "unpaid": "past_due",
    "canceled": "canceled",
    "incomplete_expired": "canceled",
    "paused": "canceled",
}


def _email_of(obj):
    md = obj.get("metadata") or {}
    details = obj.get("customer_details") or {}
    return db.normalize_email(md.get("email") or obj.get("customer_email") or details.get("email") or "")


def _find_account(obj):
    a = db.account_by_customer(obj.get("customer"))
    if a is None:
        email = _email_of(obj)
        a = db.account_by_email(email) if email else None
        if a is not None and obj.get("customer"):
            db.update_billing(a["id"], billing_customer=obj["customer"])
    return a


def _tier_of(sub):
    md = sub.get("metadata") or {}
    if md.get("tier") in db.TIERS:
        return md["tier"]
    for item in ((sub.get("items") or {}).get("data") or []):
        price = item.get("price") or {}
        if price.get("lookup_key") in config.PRO_PRICE_KEYS or (price.get("metadata") or {}).get("tier") == "pro":
            return "pro"
    return "base"


def _period_end(sub):
    if sub.get("current_period_end"):
        return int(sub["current_period_end"])
    ends = [int(i["current_period_end"]) for i in ((sub.get("items") or {}).get("data") or [])
            if i.get("current_period_end")]  # newer Stripe API versions keep it per item
    return max(ends) if ends else None


def apply_event(event, now=None):
    """Applies one verified event. Returns a short result string (also logged)."""
    if not db.first_time_event(str(event["id"]), str(event["type"])):
        return "duplicate"
    try:
        return _apply(event, now or db.now())
    except Exception:
        db.forget_event(str(event["id"]))  # let the provider's retry try again
        raise


def _apply(event, t):
    etype = event["type"]
    obj = ((event.get("data") or {}).get("object")) or {}

    if etype == "checkout.session.completed":
        a = _find_account(obj)
        return "linked customer" if a else "no account"

    if etype in ("customer.subscription.created", "customer.subscription.updated", "customer.subscription.deleted"):
        a = _find_account(obj)
        if a is None:
            return "no account"
        status = "canceled" if etype.endswith("deleted") else STATUS_MAP.get(obj.get("status"), "")
        if not status:  # "incomplete": first payment not done yet, nothing changes
            return "ignored status"
        fields = {"sub_status": status, "tier": _tier_of(obj)}
        end = _period_end(obj)
        if status in ("active", "trialing") and end:
            fields["paid_until"] = end
        elif status == "past_due":
            if a["sub_status"] != "past_due":  # grace starts once, not on every retry
                fields["paid_until"] = t + config.PAST_DUE_GRACE_DAYS * 86400
        elif status == "canceled":
            ended = obj.get("ended_at") or t
            fields["paid_until"] = min(a["paid_until"] or 0, int(ended))
        db.update_billing(a["id"], **fields)
        return f"subscription {status}"

    if etype == "invoice.payment_failed":
        a = _find_account(obj)
        if a is None:
            return "no account"
        if a["sub_status"] != "past_due":
            db.update_billing(a["id"], sub_status="past_due", paid_until=t + config.PAST_DUE_GRACE_DAYS * 86400)
        return "past_due"

    if etype == "invoice.paid":
        a = _find_account(obj)
        if a is None:
            return "no account"
        ends = [int((line.get("period") or {}).get("end", 0)) for line in ((obj.get("lines") or {}).get("data") or [])]
        end = max(ends, default=0)
        if end > (a["paid_until"] or 0):
            db.update_billing(a["id"], sub_status="active", paid_until=end)
        return "paid"

    return "ignored"
