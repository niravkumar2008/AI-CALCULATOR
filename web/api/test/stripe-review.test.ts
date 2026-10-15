// Fixes from the Stripe integration review (docs/web/STRIPE_REVIEW_2026-10-14.md), approved
// items 1-7: SCA confirmation on renewal, customer.deleted, idempotency retries for the
// customer create, refunds and disputes on store orders, the portal configuration and a
// required API version, store/plan Checkout settings, and pruning stripe_events.
// Stripe is faked in memory (test/stripe-fake.ts); nothing leaves the test.

import { env } from "cloudflare:workers";
import { createExecutionContext, createScheduledController, waitOnExecutionContext } from "cloudflare:test";
import { describe, expect, it } from "vitest";
import worker from "../src/index";
import { browserHeaders, call, expectError, signIn } from "./helpers";
import { event, fakeStripe, seedFromEvent, signed, subscription } from "./stripe-fake";

const now = () => Math.floor(Date.now() / 1000);

async function webhook(ev: object) {
  seedFromEvent(ev as { data?: { object?: Record<string, any> } });
  const s = await signed(ev);
  return call("/api/stripe/webhook", {
    method: "POST",
    body: s.body,
    headers: { "content-type": "application/json", "stripe-signature": s.signature },
  });
}

async function customerAccount() {
  const s = await signIn();
  const r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) });
  expect(r.res.status, r.text).toBe(200);
  const row = await env.DB.prepare(`SELECT id, stripe_customer_id FROM users WHERE email = ?1`)
    .bind(s.email)
    .first<{ id: string; stripe_customer_id: string }>();
  return { ...s, userId: row!.id, customer: row!.stripe_customer_id };
}

async function me(cookie: string) {
  const r = await call("/api/me", { headers: { cookie } });
  expect(r.res.status, r.text).toBe(200);
  return r.json;
}

const userRow = (id: string) =>
  env.DB.prepare(`SELECT stripe_customer_id, stripe_customer_attempt FROM users WHERE id = ?1`)
    .bind(id)
    .first<{ stripe_customer_id: string | null; stripe_customer_attempt: number }>();

// ------------------------------------------------------------ fix 1
describe("invoice.payment_action_required (review G5, fix 1)", () => {
  const HOSTED = "https://invoice.stripe.com/i/acct_test123/test_confirmMe";

  it("shows 'confirm your payment' with Stripe's invoice page, until Stripe reports it paid", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    expect((await me(a.cookie)).payment_action_required).toBe(false);
    const sub = stripe.seed(subscription(a.customer, "past_due"));
    const inv = { id: "in_sca1", object: "invoice", subscription: sub.id, hosted_invoice_url: HOSTED };
    expect((await webhook(event("invoice.payment_action_required", inv))).res.status).toBe(200);
    let m = await me(a.cookie);
    expect(m.plan).toBe("past_due"); // the 3-day grace still applies, but it isn't silent
    expect(m.payment_action_required).toBe(true);
    expect(m.payment_action_url).toBe(HOSTED);

    // A later payment_failed for the same renewal keeps the confirmation link.
    expect((await webhook(event("invoice.payment_failed", { id: "in_sca1", object: "invoice", subscription: sub.id }))).res.status).toBe(200);
    expect((await me(a.cookie)).payment_action_url).toBe(HOSTED);

    // Confirmed: Stripe has the subscription active again -> cleared.
    stripe.subscriptions.get(sub.id)!.status = "active";
    expect((await webhook(event("invoice.paid", { id: "in_sca1", object: "invoice", parent: { subscription_details: { subscription: sub.id } } }))).res.status).toBe(200);
    m = await me(a.cookie);
    expect(m.plan).toBe("active");
    expect(m.payment_action_required).toBe(false);
    expect(m.payment_action_url).toBeUndefined();
  });

  it("drops a URL that isn't Stripe's invoice page, but still flags the account", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = stripe.seed(subscription(a.customer, "past_due"));
    for (const bad of ["https://evil.example/pay", "http://invoice.stripe.com/i/x", "javascript:alert(1)", 42]) {
      const inv = { id: "in_sca2", object: "invoice", subscription: sub.id, hosted_invoice_url: bad };
      expect((await webhook(event("invoice.payment_action_required", inv))).res.status).toBe(200);
      const m = await me(a.cookie);
      expect(m.payment_action_required).toBe(true);
      expect(m.payment_action_url).toBeUndefined();
    }
  });

  it("nothing to confirm when Stripe already has the subscription active", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = stripe.seed(subscription(a.customer, "active"));
    const inv = { id: "in_sca3", object: "invoice", subscription: sub.id, hosted_invoice_url: HOSTED };
    expect((await webhook(event("invoice.payment_action_required", inv))).res.status).toBe(200);
    expect((await me(a.cookie)).payment_action_required).toBe(false);
  });
});

// ------------------------------------------------------------ fix 2
describe("customer.deleted (review G5, fix 2)", () => {
  it("unlinks the customer and cancels its subscriptions; the next checkout makes a new customer", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    await webhook(event("customer.subscription.created", subscription(a.customer, "active")));
    expect((await me(a.cookie)).plan).toBe("active");

    stripe.deletedCustomers.add(a.customer); // deleted in the Dashboard
    expect((await webhook(event("customer.deleted", { id: a.customer, object: "customer", deleted: true }))).res.status).toBe(200);
    const m = await me(a.cookie);
    expect(m.has_billing).toBe(false);
    expect(m.plan).not.toBe("active");
    expect(await userRow(a.userId)).toEqual({ stripe_customer_id: null, stripe_customer_attempt: 1 });
    const subs = await env.DB.prepare(`SELECT status FROM subscriptions WHERE user_id = ?1`).bind(a.userId).all();
    expect(subs.results.map((r: any) => r.status)).toEqual(["canceled"]);

    // The portal says "subscribe first" (409), not a 503 forever.
    expectError(await call("/api/billing/portal", { body: {}, headers: browserHeaders(a.cookie) }), 409, "no_customer");
    // A new checkout gets a new customer under a fresh idempotency key.
    const before = stripe.calls.length;
    const r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(a.cookie) });
    expect(r.res.status, r.text).toBe(200);
    const create = stripe.calls.slice(before).find((c) => c.method === "POST" && c.path === "/v1/customers")!;
    expect(create.headers["idempotency-key"]).toBe(`customer-${a.userId}-r1`);
    const now2 = (await userRow(a.userId))!.stripe_customer_id;
    expect(now2).toMatch(/^cus_/);
    expect(now2).not.toBe(a.customer);
  });

  it("a customer we don't link (or an old one) changes nothing", async () => {
    fakeStripe();
    const a = await customerAccount();
    expect((await webhook(event("customer.deleted", { id: "cus_someoneElse1", object: "customer", deleted: true }))).res.status).toBe(200);
    expect((await userRow(a.userId))!.stripe_customer_id).toBe(a.customer);
    expect((await webhook(event("customer.deleted", { id: "not-a-customer", object: "customer" }))).res.status).toBe(200);
  });
});

// ------------------------------------------------------------ fix 3
describe("customer create retries (review G9, fix 3)", () => {
  it("after a 5xx the next attempt uses a fresh idempotency key (no 24-hour replay)", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    stripe.failNext("/v1/customers", 500, "POST");
    expectError(await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) }), 503, "unavailable");
    const id = (await env.DB.prepare(`SELECT id FROM users WHERE email = ?1`).bind(s.email).first<{ id: string }>())!.id;
    expect(await userRow(id)).toEqual({ stripe_customer_id: null, stripe_customer_attempt: 1 });

    const before = stripe.calls.length;
    const r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) });
    expect(r.res.status, r.text).toBe(200);
    const next = stripe.calls.slice(before);
    // First it looks for a customer the failed attempt may have made anyway, then creates one.
    expect(next[0]).toMatchObject({ method: "GET", path: "/v1/customers" });
    expect(next[0]!.params.email).toBe(s.email);
    expect(next[1]).toMatchObject({ method: "POST", path: "/v1/customers" });
    expect(next[1]!.headers["idempotency-key"]).toBe(`customer-${id}-r1`);
    expect((await userRow(id))!.stripe_customer_id).toMatch(/^cus_/);
  });

  it("a customer the failed attempt did create is reused, not duplicated", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    const id = (await env.DB.prepare(`SELECT id FROM users WHERE email = ?1`).bind(s.email).first<{ id: string }>())!.id;
    stripe.failNext("/v1/customers", 502, "POST");
    expectError(await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) }), 503, "unavailable");
    // Stripe did run the create before the 502 reached us.
    stripe.customers.set("cus_madeAnyway1", { id: "cus_madeAnyway1", object: "customer", email: s.email, metadata: { user_id: id } });
    // Someone else's customer with the same e-mail is never taken.
    stripe.customers.set("cus_notOurs1", { id: "cus_notOurs1", object: "customer", email: s.email, metadata: { user_id: "usr_other" } });

    const before = stripe.calls.length;
    const r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) });
    expect(r.res.status, r.text).toBe(200);
    expect(stripe.calls.slice(before).some((c) => c.method === "POST" && c.path === "/v1/customers")).toBe(false);
    expect((await userRow(id))!.stripe_customer_id).toBe("cus_madeAnyway1");
    // A recovered customer counts as existing: Stripe is asked for live subscriptions (S3-3).
    expect(stripe.calls.slice(before).some((c) => c.method === "GET" && c.path === "/v1/subscriptions")).toBe(true);
  });

  it("a 4xx (Stripe refused, nothing stored) keeps the same key; success keeps 'create once'", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    stripe.failNext("/v1/customers", 400, "POST");
    expectError(await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) }), 503, "unavailable");
    const id = (await env.DB.prepare(`SELECT id FROM users WHERE email = ?1`).bind(s.email).first<{ id: string }>())!.id;
    expect((await userRow(id))!.stripe_customer_attempt).toBe(0);
    expect((await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) })).res.status).toBe(200);
    expect((await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) })).res.status).toBe(200);
    const creates = stripe.calls.filter((c) => c.method === "POST" && c.path === "/v1/customers");
    expect(creates.map((c) => c.headers["idempotency-key"])).toEqual([`customer-${id}`, `customer-${id}`]);
    expect(stripe.calls.some((c) => c.method === "GET" && c.path === "/v1/customers")).toBe(false);
  });
});

// ------------------------------------------------------------ fix 4
describe("refunds and disputes on store orders (review G6, fix 4)", () => {
  const order = (over: Record<string, any> = {}) => ({
    id: "cs_test_refund1",
    object: "checkout.session",
    mode: "payment",
    metadata: { kind: "device" },
    payment_status: "paid",
    payment_intent: "pi_testOrder1",
    amount_total: 24300,
    currency: "usd",
    customer_details: { email: "parent@example.com" },
    ...over,
  });
  const charge = (amountRefunded: number, over: Record<string, any> = {}) => ({
    id: "ch_test1",
    object: "charge",
    payment_intent: "pi_testOrder1",
    amount: 24300,
    amount_refunded: amountRefunded,
    refunded: amountRefunded >= 24300,
    ...over,
  });
  const dispute = (status: string, over: Record<string, any> = {}) => ({
    id: "dp_test1",
    object: "dispute",
    charge: "ch_test1",
    payment_intent: "pi_testOrder1",
    status,
    ...over,
  });
  const row = () => env.DB.prepare(`SELECT status, payment_intent, amount_refunded FROM orders WHERE id = 'cs_test_refund1'`).first<any>();

  it("stores the payment intent; partial then full refund; never back to partial or paid", async () => {
    fakeStripe();
    await webhook(event("checkout.session.completed", order()));
    expect(await row()).toEqual({ status: "paid", payment_intent: "pi_testOrder1", amount_refunded: null });

    await webhook(event("charge.refunded", charge(5000)));
    expect(await row()).toMatchObject({ status: "partially_refunded", amount_refunded: 5000 });
    await webhook(event("charge.refunded", charge(2000))); // an older copy arriving late
    expect(await row()).toMatchObject({ status: "partially_refunded", amount_refunded: 5000 });
    await webhook(event("charge.refunded", charge(24300)));
    expect(await row()).toMatchObject({ status: "refunded", amount_refunded: 24300 });
    await webhook(event("charge.refunded", charge(5000)));
    await webhook(event("checkout.session.completed", order())); // a late copy of the sale
    await webhook(event("checkout.session.async_payment_failed", order({ payment_status: "unpaid" })));
    expect(await row()).toMatchObject({ status: "refunded", amount_refunded: 24300 });
  });

  it("a full refund straight from paid; charges that aren't store orders match nothing", async () => {
    fakeStripe();
    await webhook(event("checkout.session.completed", order()));
    expect((await webhook(event("charge.refunded", charge(24300, { payment_intent: "pi_planInvoice9" })))).res.status).toBe(200);
    expect((await webhook(event("charge.refunded", charge(24300, { payment_intent: null })))).res.status).toBe(200);
    expect((await row()).status).toBe("paid");
    await webhook(event("charge.refunded", charge(24300)));
    expect((await row()).status).toBe("refunded");
  });

  it("an unpaid or failed order is never marked refunded", async () => {
    fakeStripe();
    await webhook(event("checkout.session.completed", order({ payment_status: "unpaid" })));
    await webhook(event("charge.refunded", charge(24300)));
    expect((await row()).status).toBe("unpaid");
    await webhook(event("checkout.session.async_payment_failed", order({ payment_status: "unpaid" })));
    await webhook(event("charge.refunded", charge(24300)));
    expect((await row()).status).toBe("failed");
  });

  it("a payment intent arriving on a later copy is filled in", async () => {
    fakeStripe();
    await webhook(event("checkout.session.completed", order({ payment_intent: null })));
    expect((await row()).payment_intent).toBeNull();
    await webhook(event("checkout.session.completed", order({ payment_intent: { id: "pi_testOrder1" } })));
    expect(await row()).toMatchObject({ status: "paid", payment_intent: "pi_testOrder1" });
  });

  it("disputed -> dispute_won; a late 'created' can't reopen it", async () => {
    fakeStripe();
    await webhook(event("checkout.session.completed", order()));
    await webhook(event("charge.dispute.created", dispute("needs_response")));
    expect((await row()).status).toBe("disputed");
    await webhook(event("charge.refunded", charge(24300))); // can't refund a disputed charge
    expect((await row()).status).toBe("disputed");
    await webhook(event("charge.dispute.closed", dispute("won")));
    expect((await row()).status).toBe("dispute_won");
    await webhook(event("charge.dispute.created", dispute("needs_response")));
    await webhook(event("charge.dispute.closed", dispute("lost")));
    expect((await row()).status).toBe("dispute_won");
  });

  it("disputed -> dispute_lost; an inquiry closed without a chargeback counts as won", async () => {
    fakeStripe();
    await webhook(event("checkout.session.completed", order()));
    await webhook(event("charge.dispute.created", dispute("needs_response")));
    await webhook(event("charge.dispute.closed", dispute("lost")));
    expect((await row()).status).toBe("dispute_lost");

    await env.DB.prepare(`DELETE FROM orders`).run();
    await webhook(event("checkout.session.completed", order()));
    await webhook(event("charge.dispute.closed", dispute("warning_closed"))); // closed arrived first
    expect((await row()).status).toBe("dispute_won");
    await webhook(event("charge.dispute.closed", dispute("under_review"))); // not an outcome
    expect((await row()).status).toBe("dispute_won");
  });

  it("a dispute after a partial refund", async () => {
    fakeStripe();
    await webhook(event("checkout.session.completed", order()));
    await webhook(event("charge.refunded", charge(1000)));
    await webhook(event("charge.dispute.created", dispute("warning_needs_response")));
    expect(await row()).toMatchObject({ status: "disputed", amount_refunded: 1000 });
  });
});

// ------------------------------------------------------------ fix 5
describe("portal configuration and a pinned API version (review G3/G4, fix 5)", () => {
  it("passes STRIPE_PORTAL_CONFIG as `configuration`; none when unset; a malformed one fails closed", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    let r = await call("/api/billing/portal", { body: {}, headers: browserHeaders(a.cookie), env: { STRIPE_PORTAL_CONFIG: "bpc_1TestPortalCfg" } });
    expect(r.res.status, r.text).toBe(200);
    expect(stripe.calls.at(-1)!.params.configuration).toBe("bpc_1TestPortalCfg");
    r = await call("/api/billing/portal", { body: {}, headers: browserHeaders(a.cookie) });
    expect(r.res.status, r.text).toBe(200);
    expect("configuration" in stripe.calls.at(-1)!.params).toBe(false);
    for (const bad of ["bpc_", "pc_123456789", "bpc_12 3456"]) {
      expectError(await call("/healthz", { env: { STRIPE_PORTAL_CONFIG: bad } }), 503, "unavailable");
    }
  });

  it("production refuses a Stripe key without STRIPE_API_VERSION; with it every call sends Stripe-Version", async () => {
    const stripe = fakeStripe();
    const prod = { DEV_MODE: "", PUBLIC_URL: "https://aicalc.example" };
    expectError(await call("/healthz", { env: { ...prod, STRIPE_API_VERSION: "" } }), 503, "unavailable");
    // Without a key nothing calls Stripe, so the version isn't needed (stages 1-2 still run).
    expect((await call("/healthz", { env: { ...prod, STRIPE_API_VERSION: "", STRIPE_SECRET_KEY: "" } })).res.status).toBe(200);
    // Local development (DEV_MODE=1) may leave it empty.
    expect((await call("/healthz", { env: { STRIPE_API_VERSION: "" } })).res.status).toBe(200);
    expectError(await call("/healthz", { env: { STRIPE_API_VERSION: "latest" } }), 503, "unavailable");

    const h = { ...browserHeaders(), origin: "https://aicalc.example" };
    const r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: h, env: prod, base: "https://aicalc.example" });
    expect(r.res.status, r.text).toBe(200);
    expect(stripe.calls.length).toBeGreaterThan(0);
    for (const c of stripe.calls) expect(c.headers["stripe-version"]).toBe("2026-09-30.clover");
  });
});

// ------------------------------------------------------------ fix 6
describe("Checkout settings (review G7, fix 6)", () => {
  it("the store collects a phone number; no shipping option unless STRIPE_SHIPPING_RATE is set", async () => {
    const stripe = fakeStripe();
    let r = await call("/api/store/checkout", { body: { quantity: 2 }, headers: browserHeaders() });
    expect(r.res.status, r.text).toBe(200);
    let p = stripe.calls.at(-1)!.params;
    expect(p["phone_number_collection[enabled]"]).toBe("true");
    expect(Object.keys(p).some((k) => k.startsWith("shipping_options"))).toBe(false);

    r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: browserHeaders(), env: { STRIPE_SHIPPING_RATE: "shr_1FreeUsShip" } });
    expect(r.res.status, r.text).toBe(200);
    p = stripe.calls.at(-1)!.params;
    expect(p["shipping_options[0][shipping_rate]"]).toBe("shr_1FreeUsShip");
    expect(p["shipping_address_collection[allowed_countries][0]"]).toBe("US");
    expectError(await call("/healthz", { env: { STRIPE_SHIPPING_RATE: "free" } }), 503, "unavailable");
  });

  it("STRIPE_TAX=1: the plan checkout requires the billing address and saves it; the store relies on shipping", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    let r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie), env: { STRIPE_TAX: "1" } });
    expect(r.res.status, r.text).toBe(200);
    let p = stripe.calls.find((c) => c.path === "/v1/checkout/sessions")!.params;
    expect(p["automatic_tax[enabled]"]).toBe("true");
    expect(p.billing_address_collection).toBe("required");
    expect(p["customer_update[address]"]).toBe("auto");
    expect(p["customer_update[name]"]).toBe("auto");

    r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: browserHeaders(), env: { STRIPE_TAX: "1" } });
    expect(r.res.status, r.text).toBe(200);
    p = stripe.calls.at(-1)!.params;
    expect(p["automatic_tax[enabled]"]).toBe("true");
    expect(Object.keys(p).some((k) => k.startsWith("customer_update"))).toBe(false); // no customer yet
  });

  it("without STRIPE_TAX the plan checkout leaves the billing address to Checkout", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    expect((await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) })).res.status).toBe(200);
    const p = stripe.calls.find((c) => c.path === "/v1/checkout/sessions")!.params;
    expect("billing_address_collection" in p).toBe(false);
  });
});

// ------------------------------------------------------------ fix 7
describe("stripe_events clean-up (review G9, fix 7)", () => {
  it("the hourly job deletes event ids older than 30 days and keeps newer ones", async () => {
    const t = now();
    await env.DB.batch([
      env.DB.prepare(`INSERT INTO stripe_events (id, type, received_at) VALUES ('evt_old1', 'invoice.paid', ?1)`).bind(t - 31 * 86400),
      env.DB.prepare(`INSERT INTO stripe_events (id, type, received_at) VALUES ('evt_new1', 'invoice.paid', ?1)`).bind(t - 29 * 86400),
      env.DB.prepare(`INSERT INTO stripe_events (id, type, received_at) VALUES ('evt_now1', 'invoice.paid', ?1)`).bind(t),
    ]);
    const ctx = createExecutionContext();
    await worker.scheduled!(createScheduledController(), env as any, ctx);
    await waitOnExecutionContext(ctx);
    const ids = (await env.DB.prepare(`SELECT id FROM stripe_events ORDER BY id`).all()).results.map((r: any) => r.id);
    expect(ids).toEqual(["evt_new1", "evt_now1"]);
  });
});
