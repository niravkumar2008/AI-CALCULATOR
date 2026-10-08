// Stage 3 (Stripe) placeholders. Stage 1 excludes /api/billing/*, /api/store/checkout and
// /api/stripe/webhook. Each test below is skipped with a TODO so stage 3 fills it in against
// openapi.yaml (Stripe test mode; webhook payloads signed with STRIPE_WEBHOOK_SECRET).
import { test, describe } from 'node:test';
import assert from 'node:assert/strict';
import { BASE_URL, ORIGIN } from './lib.mjs';

const STAGE = Number(process.env.STAGE || 1);
const TODO = 'TODO stage 3 (Stripe)';

describe('stage 1: Stripe routes are not wired yet', { skip: STAGE >= 3 ? 'stage 3 implements these' : false }, () => {
  for (const [m, p] of [['POST', '/api/billing/checkout'], ['POST', '/api/billing/portal'], ['POST', '/api/store/checkout'], ['POST', '/api/stripe/webhook']]) {
    test(`${m} ${p} -> 404 in stage 1`, async () => {
      // Not validated against the contract on purpose: 404 is not documented for these paths.
      const r = await fetch(BASE_URL + p, { method: m, headers: { 'content-type': 'application/json', origin: ORIGIN }, body: '{}' });
      assert.equal(r.status, 404);
    });
  }
});

describe('billing (stage 3)', () => {
  test('checkout: 401 without session, 403 bad Origin', { skip: TODO });
  test('checkout: 200 RedirectUrl on https://checkout.stripe.com, creates Stripe customer once', { skip: TODO });
  test('checkout: 409 already_subscribed when trialing/active/past_due', { skip: TODO });
  test('portal: 409 no_customer when never subscribed; 200 RedirectUrl on billing.stripe.com', { skip: TODO });
});

describe('store (stage 3)', () => {
  test('store checkout: quantity 1..3 (0 and 4 -> 400), no login needed', { skip: TODO });
  test('store checkout: 503 sold_out when STORE_OPEN=0', { skip: TODO });
  test('store checkout: 10 per IP per hour -> 429 rate_limited with Retry-After', { skip: TODO });
});

describe('stripe webhook (stage 3)', () => {
  test('missing or bad Stripe-Signature -> 400 bad_signature, nothing written', { skip: TODO });
  test('timestamp outside the 5 min tolerance -> 400 bad_signature', { skip: TODO });
  test('same event id delivered twice -> 200 both times, applied once (idempotency)', { skip: TODO });
  test('unknown event type -> 200 and ignored', { skip: TODO });
  test('checkout.session.completed (payment) -> order row, stripe_customer_id linked', { skip: TODO });
  test('subscription created/updated -> /api/me plan active, device can_solve', { skip: TODO });
  test('subscription past_due -> still solves within 3 days of current_period_end, then 402', { skip: TODO });
  test('subscription deleted -> plan inactive, 402 "Your subscription has ended..." message', { skip: TODO });
  test('state re-read from the event object, not metadata alone (forged metadata ignored)', { skip: TODO });
  test('DB error -> 503 so Stripe retries', { skip: TODO });
});

describe('limits that need settled spend (stage 3 follow-up)', () => {
  test('monthly solves at MONTHLY_SOLVES -> 429 quota_exceeded "Monthly AI limit reached..."', { skip: 'TODO: needs a /__dev/user field to preset usage, or a low MONTHLY_SOLVES run' });
  test('global daily cap reached -> 503 spend_cap, no upstream call', { skip: 'TODO: needs a run with GLOBAL_DAILY_CAP_USD tiny' });
  test('settlement: usage row settled to real cost from message_delta.usage', { skip: 'TODO: needs a dev read of usage rows' });
});
