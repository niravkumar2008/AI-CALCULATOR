// Stage 3 (Stripe + waitlist) black-box tests over HTTP, against wrangler dev and the Stripe
// mock (mock-stripe.mjs). Every response is checked against openapi.yaml by lib.mjs.
// Needs: STRIPE_MOCK_URL and STRIPE_WEBHOOK_SECRET (run-all.mjs sets both).
import { test, describe, before } from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import { ORIGIN, api, assertError, browser, devUser, fakeIp, linkedDevice, login, now } from './lib.mjs';
import { MOCK_STRIPE_KEY } from './mock-stripe.mjs';

const STRIPE = (process.env.STRIPE_MOCK_URL || 'http://127.0.0.1:8789').replace(/\/$/, '');
const SECRET = process.env.STRIPE_WEBHOOK_SECRET || '';
const PASS = process.env.STAGE3_PASS || 'main';

const stripeCalls = async () => (await fetch(STRIPE + '/__calls')).json();
const seedSub = (s) => fetch(STRIPE + '/__subscriptions', { method: 'POST', body: JSON.stringify(s) });
const failNext = (prefix, status = 500, method) =>
  fetch(STRIPE + '/__fail', { method: 'POST', body: JSON.stringify({ prefix, status, method }) });
const customerOf = async (email) => (await (await fetch(`${STRIPE}/__customer?email=${encodeURIComponent(email)}`)).json()).id;

let seq = 0;
function sub(customer, status, over = {}) {
  return {
    id: `sub_bb${crypto.randomBytes(6).toString('hex')}`,
    object: 'subscription',
    customer,
    status,
    cancel_at_period_end: false,
    items: { data: [{ price: { id: 'price_mockMonthly15' }, current_period_end: now() + 30 * 86400 }] },
    metadata: {},
    ...over,
  };
}
const event = (type, object, created = now(), extra = {}) => ({
  id: `evt_bb${crypto.randomBytes(6).toString('hex')}${++seq}`,
  object: 'event',
  type,
  created,
  livemode: false,
  ...extra,
  data: { object },
});

function sign(body, t = now(), secret = SECRET) {
  const v1 = crypto.createHmac('sha256', secret).update(`${t}.${body}`).digest('hex');
  return `t=${t},v1=${v1}`;
}
/**
 * Sends a signed event. The Worker re-reads every subscription from Stripe (S3-4), so a
 * subscription in the event is first stored in the mock as Stripe's current state, unless
 * `seed: false` (a stale event).
 */
async function webhook(ev, { t, secret, signature, seed = true } = {}) {
  if (seed && ev.data?.object?.object === 'subscription') await seedSub(ev.data.object);
  const body = JSON.stringify(ev);
  return api('POST', '/api/stripe/webhook', {
    rawBody: body,
    headers: { 'stripe-signature': signature ?? sign(body, t, secret) },
  });
}

/** Signed-in account whose free month is over, with a Stripe customer (from a checkout). */
async function subscriberToBe({ withDevice = false } = {}) {
  const base = withDevice ? await linkedDevice({ trial: true }) : { user: await login() };
  await devUser({ email: base.user.email, trial_ends_at: now() - 60 });
  const r = await browser('POST', '/api/billing/checkout', base.user.cookie);
  assert.equal(r.status, 200, r.text);
  return { ...base, customer: await customerOf(base.user.email) };
}

const status = (token) => api('GET', '/v1/device/status', { token });
const me = async (cookie) => (await api('GET', '/api/me', { cookie })).json;

describe('stage 3: Stripe and waitlist', { skip: !SECRET ? 'STRIPE_WEBHOOK_SECRET not set (run via npm run test:full)' : false }, () => {
  before(async () => {
    const r = await fetch(STRIPE + '/healthz').catch(() => null);
    assert.ok(r?.ok, `Stripe mock not reachable at ${STRIPE}`);
  });

  describe('closed store pass', { skip: PASS !== 'closed' }, () => {
    test('store checkout: 503 sold_out when STORE_OPEN=0', async () => {
      assertError(await browser('POST', '/api/store/checkout', undefined, { quantity: 1 }), 503, 'sold_out');
    });

    test('store status: open false when STORE_OPEN=0', async () => {
      const r = await api('GET', '/api/store/status');
      assert.equal(r.status, 200, r.text);
      assert.deepEqual(r.json, { open: false, max_quantity: 3 });
    });
  });

  describe('billing', { skip: PASS !== 'main' }, () => {
    test('checkout: 401 without session, 403 bad Origin', async () => {
      assertError(await browser('POST', '/api/billing/checkout', undefined), 401, 'unauthorized');
      const u = await login();
      assertError(await api('POST', '/api/billing/checkout', { cookie: u.cookie, body: {}, origin: 'https://evil.example' }), 403, 'forbidden');
    });

    test('checkout: 200 RedirectUrl on https://checkout.stripe.com, creates Stripe customer once', async () => {
      const u = await login();
      const a = await browser('POST', '/api/billing/checkout', u.cookie);
      const b = await browser('POST', '/api/billing/checkout', u.cookie);
      assert.equal(a.status, 200, a.text);
      assert.equal(b.status, 200, b.text);
      assert.match(a.json.url, /^https:\/\/checkout\.stripe\.com\//);
      const created = (await stripeCalls()).filter((c) => c.path === '/v1/customers' && c.params.email === u.email);
      assert.equal(created.length, 1);
      assert.match(created[0].idempotencyKey, /^customer-usr_/);
    });

    test('checkout: 409 already_subscribed when trialing/active/past_due', async () => {
      const s = await subscriberToBe();
      assert.equal((await webhook(event('customer.subscription.created', sub(s.customer, 'active')))).status, 200);
      assertError(await browser('POST', '/api/billing/checkout', s.user.cookie), 409, 'already_subscribed');
    });

    test('checkout: 409 already_subscribed when Stripe has a subscription we have not heard of yet', async () => {
      const s = await subscriberToBe();
      await seedSub(sub(s.customer, 'active')); // paid in another tab; no webhook delivered
      const before = (await stripeCalls()).length;
      assertError(await browser('POST', '/api/billing/checkout', s.user.cookie), 409, 'already_subscribed');
      const made = (await stripeCalls()).slice(before);
      const list = made.find((c) => c.method === 'GET' && c.path === '/v1/subscriptions');
      assert.ok(list, 'asked Stripe for the customer\'s subscriptions');
      assert.equal(list.params.customer, s.customer);
      assert.equal(list.params.status, 'all');
      assert.ok(!made.some((c) => c.path === '/v1/checkout/sessions'), 'no Checkout session created');
    });

    test('checkout: sessions expire after 31 minutes and return to /account/', async () => {
      const u = await login();
      const r = await browser('POST', '/api/billing/checkout', u.cookie);
      assert.equal(r.status, 200, r.text);
      const sess = (await stripeCalls()).filter((c) => c.path === '/v1/checkout/sessions').at(-1).params;
      assert.ok(Math.abs(Number(sess.expires_at) - (now() + 31 * 60)) <= 5, sess.expires_at);
      assert.equal(sess.success_url, `${ORIGIN}/account/?checkout=done`);
      assert.equal(sess.cancel_url, `${ORIGIN}/account/?checkout=cancelled`);
    });

    test('checkout: interval month (default) or year picks the matching price; anything else -> 400', async () => {
      const u = await login();
      const priceOfLast = async () =>
        (await stripeCalls()).filter((c) => c.path === '/v1/checkout/sessions').at(-1).params['line_items[0][price]'];
      assert.equal((await browser('POST', '/api/billing/checkout', u.cookie)).status, 200);
      assert.equal(await priceOfLast(), 'price_mockMonthly15');
      assert.equal((await browser('POST', '/api/billing/checkout', u.cookie, { interval: 'year' })).status, 200);
      assert.equal(await priceOfLast(), 'price_mockYearly100');
      assertError(await browser('POST', '/api/billing/checkout', u.cookie, { interval: 'week' }), 400, 'bad_request');
    });

    test('a yearly subscription unlocks AI and /api/me says plan_interval year', async () => {
      const s = await subscriberToBe({ withDevice: true });
      const yearly = sub(s.customer, 'active', {
        items: { data: [{ price: { id: 'price_mockYearly100' }, current_period_end: now() + 365 * 86400 }] },
      });
      assert.equal((await webhook(event('customer.subscription.created', yearly))).status, 200);
      assert.equal((await status(s.dev.token)).json.can_solve, true);
      const m = await me(s.user.cookie);
      assert.equal(m.plan, 'active');
      assert.equal(m.plan_interval, 'year');
    });

    test('portal: 409 no_customer when never subscribed; 200 RedirectUrl on billing.stripe.com', async () => {
      const u = await login();
      assertError(await browser('POST', '/api/billing/portal', u.cookie), 409, 'no_customer');
      await browser('POST', '/api/billing/checkout', u.cookie);
      const r = await browser('POST', '/api/billing/portal', u.cookie);
      assert.equal(r.status, 200, r.text);
      assert.match(r.json.url, /^https:\/\/billing\.stripe\.com\//);
    });
  });

  describe('store', { skip: PASS !== 'main' }, () => {
    test('store status: 200 {open: true, max_quantity: 3}, no login needed; GET only', async () => {
      const r = await api('GET', '/api/store/status');
      assert.equal(r.status, 200, r.text);
      assert.deepEqual(r.json, { open: true, max_quantity: 3 });
      // 405 isn't in the contract for this path (no POST operation): skip the schema check.
      const p = await api('POST', '/api/store/status', { body: {}, origin: ORIGIN, check: false });
      assertError(p, 405, 'bad_request');
      assert.equal(p.headers.get('allow'), 'GET');
    });

    test('store checkout: quantity 1..3 (0 and 4 -> 400), no login needed', async () => {
      assertError(await browser('POST', '/api/store/checkout', undefined, { quantity: 0 }), 400, 'bad_request');
      assertError(await browser('POST', '/api/store/checkout', undefined, { quantity: 4 }), 400, 'bad_request');
      const r = await browser('POST', '/api/store/checkout', undefined, { quantity: 3 });
      assert.equal(r.status, 200, r.text);
      assert.match(r.json.url, /^https:\/\/checkout\.stripe\.com\//);
      // The contract's "phone number collected" (carriers need it).
      const s = (await stripeCalls()).filter((c) => c.path === '/v1/checkout/sessions').at(-1);
      assert.equal(s.params.mode, 'payment');
      assert.equal(s.params['phone_number_collection[enabled]'], 'true');
    });

    test('store checkout: 10 per IP per hour -> 429 rate_limited with Retry-After', async () => {
      const ip = fakeIp();
      for (let i = 0; i < 10; i++) {
        assert.equal((await browser('POST', '/api/store/checkout', undefined, { quantity: 1 }, { ip })).status, 200);
      }
      const r = await browser('POST', '/api/store/checkout', undefined, { quantity: 1 }, { ip });
      assertError(r, 429, 'rate_limited');
      assert.ok(Number(r.headers.get('retry-after')) > 0);
    });
  });

  describe('stripe webhook', { skip: PASS !== 'main' }, () => {
    test('missing or bad Stripe-Signature -> 400 bad_signature, nothing written', async () => {
      const s = await subscriberToBe();
      const ev = event('customer.subscription.created', sub(s.customer, 'active'));
      assertError(await api('POST', '/api/stripe/webhook', { rawBody: JSON.stringify(ev) }), 400, 'bad_signature');
      assertError(await webhook(ev, { secret: 'whsec_notTheRealSecret0123456789' }), 400, 'bad_signature');
      assert.equal((await me(s.user.cookie)).plan, 'inactive'); // nothing applied
      assert.equal((await webhook(ev)).status, 200); // the same event, correctly signed, still applies
      assert.equal((await me(s.user.cookie)).plan, 'active');
    });

    test('timestamp outside the 5 min tolerance -> 400 bad_signature', async () => {
      const ev = event('customer.subscription.created', sub('cus_whoever', 'active'));
      assertError(await webhook(ev, { t: now() - 400 }), 400, 'bad_signature');
      assertError(await webhook(ev, { t: now() + 400 }), 400, 'bad_signature');
    });

    test('same event id delivered twice -> 200 both times, applied once (idempotency)', async () => {
      const s = await subscriberToBe();
      const ev = event('customer.subscription.created', sub(s.customer, 'active'));
      const a = await webhook(ev);
      const b = await webhook(ev);
      assert.equal(a.status, 200);
      assert.equal(b.status, 200);
      assert.equal(b.json.duplicate, true);
    });

    test('unknown event type -> 200 and ignored', async () => {
      assert.equal((await webhook(event('radar.early_fraud_warning.created', { id: 'issfr_x' }))).status, 200);
    });

    test('checkout.session.completed (payment) -> 200 (order recorded; checked in unit tests)', async () => {
      const r = await webhook(event('checkout.session.completed', {
        id: 'cs_test_bbOrder', object: 'checkout.session', mode: 'payment', payment_status: 'paid',
        metadata: { kind: 'device' },
        amount_total: 22500, currency: 'usd', customer_details: { email: 'parent@example.test' },
      }));
      assert.equal(r.status, 200);
    });

    test('refund and dispute events on a store order -> 200 (status machine checked in unit tests)', async () => {
      const order = {
        id: 'cs_test_bbRefund', object: 'checkout.session', mode: 'payment', payment_status: 'paid',
        metadata: { kind: 'device' }, payment_intent: 'pi_bbRefund1',
        amount_total: 22500, currency: 'usd', customer_details: { email: 'parent@example.test' },
      };
      assert.equal((await webhook(event('checkout.session.completed', order))).status, 200);
      const ch = { id: 'ch_bb1', object: 'charge', payment_intent: 'pi_bbRefund1', amount: 22500, amount_refunded: 1000, refunded: false };
      assert.equal((await webhook(event('charge.refunded', ch))).status, 200);
      const dp = { id: 'dp_bb1', object: 'dispute', charge: 'ch_bb1', payment_intent: 'pi_bbRefund1', status: 'needs_response' };
      assert.equal((await webhook(event('charge.dispute.created', dp))).status, 200);
      assert.equal((await webhook(event('charge.dispute.closed', { ...dp, status: 'won' }))).status, 200);
    });

    test('invoice.payment_action_required -> /api/me says confirm your payment, with the invoice page', async () => {
      const s = await subscriberToBe();
      const pd = sub(s.customer, 'past_due');
      await seedSub(pd);
      const url = 'https://invoice.stripe.com/i/acct_bb/test_confirm';
      const inv = { id: 'in_bbSca', object: 'invoice', subscription: pd.id, hosted_invoice_url: url };
      assert.equal((await webhook(event('invoice.payment_action_required', inv))).status, 200);
      let m = await me(s.user.cookie);
      assert.equal(m.plan, 'past_due');
      assert.equal(m.payment_action_required, true);
      assert.equal(m.payment_action_url, url);
      await seedSub({ ...pd, status: 'active' });
      assert.equal((await webhook(event('invoice.paid', { id: 'in_bbSca', object: 'invoice', subscription: pd.id }))).status, 200);
      m = await me(s.user.cookie);
      assert.equal(m.payment_action_required, false);
      assert.equal(m.payment_action_url, undefined);
    });

    test('customer.deleted -> billing unlinked (portal 409, not 503); the next checkout makes a new customer', async () => {
      const s = await subscriberToBe();
      await webhook(event('customer.subscription.created', sub(s.customer, 'active')));
      // Deleted in the Dashboard: gone at Stripe (its subscriptions cancelled), then the event.
      const del = await fetch(`${STRIPE}/v1/customers/${s.customer}`, { method: 'DELETE', headers: { authorization: `Bearer ${MOCK_STRIPE_KEY}` } });
      assert.equal(del.status, 200);
      assert.equal((await webhook(event('customer.deleted', { id: s.customer, object: 'customer', deleted: true }))).status, 200);
      const m = await me(s.user.cookie);
      assert.equal(m.has_billing, false);
      assert.equal(m.plan, 'inactive');
      assertError(await browser('POST', '/api/billing/portal', s.user.cookie), 409, 'no_customer');
      const r = await browser('POST', '/api/billing/checkout', s.user.cookie);
      assert.equal(r.status, 200, r.text);
      const fresh = await customerOf(s.user.email);
      assert.notEqual(fresh, s.customer);
      const create = (await stripeCalls()).filter((c) => c.method === 'POST' && c.path === '/v1/customers').at(-1);
      assert.match(create.idempotencyKey, /^customer-usr_[0-9a-f]+-r1$/);
    });

    test('a 5xx on the first customer create: 503, then the retry uses a fresh idempotency key', async () => {
      const u = await login();
      await failNext('/v1/customers', 500, 'POST');
      assertError(await browser('POST', '/api/billing/checkout', u.cookie), 503, 'unavailable');
      const before = (await stripeCalls()).length;
      const r = await browser('POST', '/api/billing/checkout', u.cookie);
      assert.equal(r.status, 200, r.text);
      const calls = (await stripeCalls()).slice(before);
      assert.equal(calls[0].method, 'GET');
      assert.equal(calls[0].path, '/v1/customers');
      assert.equal(calls[1].path, '/v1/customers');
      assert.match(calls[1].idempotencyKey, /-r1$/);
    });

    test('event whose livemode does not match the key -> 400 bad_signature', async () => {
      const s = await subscriberToBe();
      const live = sub(s.customer, 'active');
      assertError(await webhook(event('customer.subscription.created', live, now(), { livemode: true })), 400, 'bad_signature');
      assert.equal((await me(s.user.cookie)).plan, 'inactive');
      assert.equal((await webhook(event('customer.subscription.created', live))).status, 200); // livemode: false
      assert.equal((await me(s.user.cookie)).plan, 'active');
    });

    test('a stale event can\'t override Stripe\'s current state (subscriptions are re-read)', async () => {
      const s = await subscriberToBe();
      const live = sub(s.customer, 'active');
      await seedSub(live);
      await webhook(event('customer.subscription.created', { ...live, status: 'incomplete' }, now() - 60), { seed: false });
      assert.equal((await me(s.user.cookie)).plan, 'active');
    });

    test('subscription created/updated -> /api/me plan active, device can_solve', async () => {
      const s = await subscriberToBe({ withDevice: true });
      let st = await status(s.dev.token);
      assert.equal(st.json.can_solve, false);
      assert.match(st.json.reason, /^Your free month is over\./);
      const created = sub(s.customer, 'active');
      await webhook(event('customer.subscription.created', created));
      st = await status(s.dev.token);
      assert.equal(st.json.can_solve, true);
      assert.equal((await me(s.user.cookie)).plan, 'active');
      await webhook(event('customer.subscription.updated', { ...created, cancel_at_period_end: true }, now() + 1));
      assert.equal((await me(s.user.cookie)).cancel_at_period_end, true);
    });

    test('subscription past_due -> still solves (3-day grace counts from the failed payment, not the period end)', async () => {
      // The 3-day cut-off itself needs a clock: covered in the unit tests (billing.test.ts).
      const s = await subscriberToBe({ withDevice: true });
      const pd = sub(s.customer, 'past_due', {
        items: { data: [{ price: { id: 'price_mockMonthly15' }, current_period_end: now() - 4 * 86400 }] },
      });
      await webhook(event('customer.subscription.updated', pd));
      const st = await status(s.dev.token);
      assert.equal(st.json.can_solve, true);
      assert.equal((await me(s.user.cookie)).plan, 'past_due');
    });

    test('a subscription to another price does not unlock AI', async () => {
      const s = await subscriberToBe({ withDevice: true });
      const other = sub(s.customer, 'active', {
        items: { data: [{ price: { id: 'price_somethingElse9' }, current_period_end: now() + 30 * 86400 }] },
      });
      assert.equal((await webhook(event('customer.subscription.created', other))).status, 200);
      assert.equal((await status(s.dev.token)).json.can_solve, false);
      assert.equal((await me(s.user.cookie)).plan, 'inactive');
    });

    test('subscription deleted -> plan inactive, "Your subscription has ended..." message', async () => {
      const s = await subscriberToBe({ withDevice: true });
      const live = sub(s.customer, 'active');
      await webhook(event('customer.subscription.created', live));
      await webhook(event('customer.subscription.deleted', { ...live, status: 'canceled' }, now() + 1));
      assert.equal((await me(s.user.cookie)).plan, 'inactive');
      const st = await status(s.dev.token);
      assert.equal(st.json.can_solve, false);
      assert.match(st.json.reason, /^Your subscription has ended\. Renew at /);
    });

    test('state re-read from the event object, not metadata alone (forged metadata ignored)', async () => {
      const victim = await subscriberToBe();
      const userId = (await me(victim.user.cookie)).email; // not the id; the forged sub names a stranger's customer
      const forged = sub('cus_strangerCustomer01', 'active', { metadata: { user_id: 'usr_0000000000000000', email: userId } });
      assert.equal((await webhook(event('customer.subscription.created', forged))).status, 200);
      assert.equal((await me(victim.user.cookie)).plan, 'inactive');
    });

    test('Stripe failure while handling -> 503 so Stripe retries; the retry applies it', async () => {
      const s = await subscriberToBe();
      const live = sub(s.customer, 'active');
      await seedSub(live);
      const ev = event('invoice.paid', { id: 'in_bb1', object: 'invoice', subscription: live.id });
      await failNext(`/v1/subscriptions/${live.id}`);
      assertError(await webhook(ev), 503, 'unavailable');
      assert.equal((await webhook(ev)).status, 200);
      assert.equal((await me(s.user.cookie)).plan, 'active');
    });
  });

  describe('account deletion', { skip: PASS !== 'main' }, () => {
    test('DELETE /api/me deletes the Stripe customer first (cancelling its subscriptions)', async () => {
      const s = await subscriberToBe();
      const live = sub(s.customer, 'active');
      await webhook(event('customer.subscription.created', live));
      const r = await browser('DELETE', '/api/me', s.user.cookie, { confirm_email: s.user.email });
      assert.equal(r.status, 204, r.text);
      const del = (await stripeCalls()).filter((c) => c.method === 'DELETE' && c.path === `/v1/customers/${s.customer}`);
      assert.equal(del.length, 1);
    });

    test('DELETE /api/me: 503 and nothing erased if Stripe fails', async () => {
      const s = await subscriberToBe();
      await webhook(event('customer.subscription.created', sub(s.customer, 'active')));
      await failNext(`/v1/customers/${s.customer}`, 500, 'DELETE');
      assertError(await browser('DELETE', '/api/me', s.user.cookie, { confirm_email: s.user.email }), 503, 'unavailable');
      const m = await me(s.user.cookie);
      assert.equal(m.email, s.user.email);
      assert.equal(m.plan, 'active');
    });

    test('a live subscription arriving after the account was deleted is cancelled at Stripe', async () => {
      const s = await subscriberToBe();
      const r = await browser('DELETE', '/api/me', s.user.cookie, { confirm_email: s.user.email });
      assert.equal(r.status, 204, r.text);
      const late = sub(s.customer, 'active');
      assert.equal((await webhook(event('customer.subscription.created', late))).status, 200);
      const del = (await stripeCalls()).filter((c) => c.method === 'DELETE' && c.path === `/v1/subscriptions/${late.id}`);
      assert.equal(del.length, 1);
    });
  });

  describe('waitlist', { skip: PASS !== 'main' }, () => {
    test('201 for new and repeated addresses (same answer)', async () => {
      const addr = `wl-${crypto.randomBytes(4).toString('hex')}@example.test`;
      const a = await browser('POST', '/api/waitlist', undefined, {
        email: addr,
        age_ok: true,
        source: 'hero',
        utm_source: 'tiktok',
        utm_medium: 'video',
        utm_campaign: 'bb-test',
        ref: 'bb1',
      });
      const b = await browser('POST', '/api/waitlist', undefined, { email: addr.toUpperCase(), age_ok: true, source: 'footer' });
      assert.equal(a.status, 201, a.text);
      assert.equal(b.status, 201);
      assert.deepEqual(a.json, b.json);
    });

    test('bad email or no 13+ confirmation -> 400; other Origin -> 403', async () => {
      assertError(await browser('POST', '/api/waitlist', undefined, { email: 'nope', age_ok: true }), 400, 'bad_request');
      assertError(await browser('POST', '/api/waitlist', undefined, { email: 'kid@example.test' }), 400, 'bad_request');
      assertError(await browser('POST', '/api/waitlist', undefined, { email: 'kid@example.test', age_ok: false }), 400, 'bad_request');
      assertError(await api('POST', '/api/waitlist', { body: { email: 'a@example.test', age_ok: true }, origin: 'https://evil.example' }), 403, 'forbidden');
    });

    test('plain HTML form post: 400 without age_ok=on', async () => {
      const r = await api('POST', '/api/waitlist', {
        rawBody: 'email=form-kid%40example.test&company=',
        contentType: 'application/x-www-form-urlencoded',
        origin: ORIGIN,
      });
      assert.equal(r.status, 400);
    });

    test('plain HTML form post -> 303 back to the site', async () => {
      const r = await api('POST', '/api/waitlist', {
        rawBody: 'email=form-bb%40example.test&company=&age_ok=on&source=footer&utm_source=yt&utm_medium=&utm_campaign=&ref=',
        contentType: 'application/x-www-form-urlencoded',
        origin: ORIGIN,
      });
      assert.equal(r.status, 303);
      assert.equal(r.headers.get('location'), `${ORIGIN}/?waitlist=joined#waitlist`);
    });
  });
});
