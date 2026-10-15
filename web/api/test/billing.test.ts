// Stage 3: Stripe Checkout, Customer Portal, store, webhooks, account deletion and the waitlist.
// Stripe is faked in memory (test/stripe-fake.ts); nothing leaves the test.

import { env } from "cloudflare:workers";
import { describe, expect, it } from "vitest";
import {
  ORIGIN,
  browserHeaders,
  call,
  deviceHeaders,
  expectError,
  freshIp,
  linkedDevice,
  signIn,
} from "./helpers";
import { event, fakeStripe, seedFromEvent, signed, subscription } from "./stripe-fake";

const now = () => Math.floor(Date.now() / 1000);

/**
 * Delivers a signed event. By default a subscription in the event is also stored in the fake
 * Stripe first (Stripe's current state matches the event); `seed: false` sends a stale event
 * whose object no longer matches Stripe (S3-4: the Worker re-reads the subscription).
 */
async function webhook(ev: object, opts: { t?: number; secret?: string; signature?: string; seed?: boolean } = {}) {
  if (opts.seed !== false) seedFromEvent(ev as { data?: { object?: Record<string, any> } });
  const s = await signed(ev, opts.t, opts.secret);
  return call("/api/stripe/webhook", {
    method: "POST",
    body: s.body,
    headers: { "content-type": "application/json", "stripe-signature": opts.signature ?? s.signature },
  });
}

/** Signed-in account with a Stripe customer (created by a first checkout). */
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

async function deviceStatus(token: string) {
  const r = await call("/v1/device/status", { headers: { authorization: `Bearer ${token}`, "x-firmware": "stage13" } });
  expect(r.res.status, r.text).toBe(200);
  return r.json;
}

// ------------------------------------------------------------ checkout
describe("POST /api/billing/checkout", () => {
  it("needs a session and the site Origin", async () => {
    fakeStripe();
    expectError(await call("/api/billing/checkout", { body: {}, headers: browserHeaders() }), 401, "unauthorized");
    const s = await signIn();
    const h = { ...browserHeaders(s.cookie), origin: "https://evil.example" };
    expectError(await call("/api/billing/checkout", { body: {}, headers: h }), 403, "forbidden");
  });

  it("returns a checkout.stripe.com URL and creates the Stripe customer only once", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    const a = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) });
    const b = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) });
    expect(a.res.status, a.text).toBe(200);
    expect(a.json.url).toMatch(/^https:\/\/checkout\.stripe\.com\//);
    expect(b.res.status).toBe(200);

    const customers = stripe.calls.filter((c) => c.path === "/v1/customers");
    expect(customers).toHaveLength(1); // the second checkout reuses the stored customer
    expect(customers[0]!.headers["idempotency-key"]).toMatch(/^customer-usr_/);
    expect(customers[0]!.params.email).toBe(s.email);

    const session = stripe.calls.find((c) => c.path === "/v1/checkout/sessions")!.params;
    expect(session.mode).toBe("subscription");
    expect(session["line_items[0][price]"]).toBe("price_subMonthly15");
    expect(session["line_items[0][quantity]"]).toBe("1");
    expect(session.client_reference_id).toMatch(/^usr_/);
    expect(session.customer).toMatch(/^cus_/);
    expect(session["consent_collection[terms_of_service]"]).toBe("required");
    expect(session["custom_text[terms_of_service_acceptance][message]"]).toMatch(/18 or older/);
    expect(session.success_url).toBe(`${ORIGIN}/account/?checkout=done`);
    expect(session.cancel_url).toBe(`${ORIGIN}/account/?checkout=cancelled`);
    // S3-3: the session expires just over Stripe's 30-minute minimum.
    expect(Number(session.expires_at)).toBe(now() + 31 * 60);
    // Decision D2: our free month is the trial; Stripe never gets a trial.
    expect(Object.keys(session).some((k) => k.includes("trial"))).toBe(false);
    expect(Object.keys(session).some((k) => k.startsWith("automatic_tax"))).toBe(false); // STRIPE_TAX off
  });

  it("adds Stripe Tax when STRIPE_TAX=1", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    const r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie), env: { STRIPE_TAX: "1" } });
    expect(r.res.status, r.text).toBe(200);
    const p = stripe.calls.find((c) => c.path === "/v1/checkout/sessions")!.params;
    expect(p["automatic_tax[enabled]"]).toBe("true");
    expect(p["customer_update[address]"]).toBe("auto");
  });

  it("409 already_subscribed while a subscription is trialing, active, past_due, unpaid or incomplete", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    for (const status of ["active", "trialing", "past_due", "unpaid", "incomplete"]) {
      await env.DB.prepare(`DELETE FROM subscriptions`).run();
      stripe.subscriptions.clear();
      expect((await webhook(event("customer.subscription.updated", subscription(a.customer, status)))).res.status).toBe(200);
      stripe.subscriptions.clear(); // the local row alone must block
      const before = stripe.calls.length;
      expectError(await call("/api/billing/checkout", { body: {}, headers: browserHeaders(a.cookie) }), 409, "already_subscribed");
      expect(stripe.calls.length).toBe(before); // blocked before asking Stripe
    }
  });

  it("409 already_subscribed when Stripe has a subscription our database hasn't heard of yet (S3-3)", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    for (const status of ["active", "trialing", "past_due", "unpaid", "incomplete"]) {
      stripe.subscriptions.clear();
      stripe.seed(subscription(a.customer, status)); // paid in another tab; no webhook yet
      const before = stripe.calls.length;
      expectError(await call("/api/billing/checkout", { body: {}, headers: browserHeaders(a.cookie) }), 409, "already_subscribed");
      const made = stripe.calls.slice(before);
      expect(made.map((c) => `${c.method} ${c.path}`)).toEqual(["GET /v1/subscriptions"]); // no session created
      expect(made[0]!.params).toEqual({ customer: a.customer, status: "all", limit: "20" });
    }
    // Only ended subscriptions (or another customer's): a new checkout is fine.
    stripe.subscriptions.clear();
    stripe.seed(subscription(a.customer, "canceled"));
    stripe.seed(subscription(a.customer, "incomplete_expired"));
    stripe.seed(subscription("cus_someoneElse0002", "active"));
    const r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(a.cookie) });
    expect(r.res.status, r.text).toBe(200);
    // If the Stripe check fails, no session is made: 503.
    stripe.failNext("/v1/subscriptions", 500, "GET");
    expectError(await call("/api/billing/checkout", { body: {}, headers: browserHeaders(a.cookie) }), 503, "unavailable");
  });

  it("the first checkout (new customer) skips the Stripe subscription check; sessions carry expires_at", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    expect((await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) })).res.status).toBe(200);
    expect(stripe.calls.map((c) => `${c.method} ${c.path}`)).toEqual(["POST /v1/customers", "POST /v1/checkout/sessions"]);
    const exp = Number(stripe.calls[1]!.params.expires_at);
    expect(exp - now()).toBeGreaterThanOrEqual(30 * 60); // Stripe refuses less than 30 minutes
    expect(exp - now()).toBeLessThanOrEqual(31 * 60);
  });

  it("503 unavailable (no details) when Stripe isn't configured or fails", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    let r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie), env: { STRIPE_SECRET_KEY: "" } });
    expectError(r, 503, "unavailable");
    stripe.failNext("/v1/checkout/sessions", 500);
    r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) });
    const e = expectError(r, 503, "unavailable");
    expect(e.message).not.toMatch(/stripe|boom|500/i);
  });

  it("never redirects to a non-Stripe host, even if Stripe's answer says so", async () => {
    fakeStripe();
    // Re-mock: checkout answers with an attacker URL.
    const s = await signIn();
    const orig = globalThis.fetch;
    (globalThis.fetch as any) = async (input: RequestInfo | URL, init?: RequestInit) => {
      const u = String(input instanceof Request ? input.url : input);
      if (u.endsWith("/v1/checkout/sessions")) return Response.json({ id: "cs_x", url: "https://evil.example/pay" });
      return orig(input as RequestInfo, init);
    };
    try {
      expectError(await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) }), 503, "unavailable");
    } finally {
      globalThis.fetch = orig;
    }
  });
});

// ------------------------------------------------------------ portal
describe("POST /api/billing/portal", () => {
  it("409 no_customer before the first checkout, then a billing.stripe.com URL", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    expectError(await call("/api/billing/portal", { body: {}, headers: browserHeaders(s.cookie) }), 409, "no_customer");
    await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) });
    const r = await call("/api/billing/portal", { body: {}, headers: browserHeaders(s.cookie) });
    expect(r.res.status, r.text).toBe(200);
    expect(r.json.url).toMatch(/^https:\/\/billing\.stripe\.com\//);
    const p = stripe.calls.find((c) => c.path === "/v1/billing_portal/sessions")!.params;
    expect(p.customer).toMatch(/^cus_/);
    expect(p.return_url).toBe(`${ORIGIN}/account/`);
  });
});

// ------------------------------------------------------------ store
describe("POST /api/store/checkout", () => {
  it("validates quantity 1..3 and needs no login", async () => {
    const stripe = fakeStripe();
    for (const q of [0, 4, 1.5, "2", null]) {
      expectError(await call("/api/store/checkout", { body: { quantity: q }, headers: browserHeaders() }), 400, "bad_request");
    }
    expectError(await call("/api/store/checkout", { body: { quantity: 1, x: 1 }, headers: browserHeaders() }), 400, "bad_request");
    const r = await call("/api/store/checkout", { body: { quantity: 2 }, headers: browserHeaders() });
    expect(r.res.status, r.text).toBe(200);
    expect(r.json.url).toMatch(/^https:\/\/checkout\.stripe\.com\//);
    const p = stripe.calls.find((c) => c.path === "/v1/checkout/sessions")!.params;
    expect(p.mode).toBe("payment");
    expect(p["line_items[0][price]"]).toBe("price_device225");
    expect(p["line_items[0][quantity]"]).toBe("2");
    expect(p["shipping_address_collection[allowed_countries][0]"]).toBe("US");
    expect(p["consent_collection[terms_of_service]"]).toBe("required");
    expect(p["metadata[kind]"]).toBe("device"); // S3-6: how the webhook knows it's our order
    expect(Number(p.expires_at)).toBe(now() + 31 * 60);
    expect(p.success_url).toBe(`${ORIGIN}/order/complete/`);
    expect(p.cancel_url).toBe(`${ORIGIN}/cart/?checkout=cancelled`);
  });

  it("503 sold_out while STORE_OPEN isn't 1 (the default)", async () => {
    fakeStripe();
    const r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: browserHeaders(), env: { STORE_OPEN: "" } });
    expectError(r, 503, "sold_out");
  });

  it("10 per IP per hour, then 429 with Retry-After", async () => {
    fakeStripe();
    const h = { ...browserHeaders(), "x-dev-client-ip": freshIp() };
    for (let i = 0; i < 10; i++) {
      expect((await call("/api/store/checkout", { body: { quantity: 1 }, headers: h })).res.status).toBe(200);
    }
    const r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: h });
    expectError(r, 429, "rate_limited");
    expect(Number(r.res.headers.get("retry-after"))).toBeGreaterThan(0);
  });

  it("a global hourly cap (STORE_GLOBAL_PER_HOUR) applies across IPs, before the per-IP limit (S3-9)", async () => {
    const stripe = fakeStripe();
    const cap = { STORE_GLOBAL_PER_HOUR: "3" };
    for (let i = 0; i < 3; i++) {
      const r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: { ...browserHeaders(), "x-dev-client-ip": freshIp() }, env: cap });
      expect(r.res.status, r.text).toBe(200);
    }
    const ip = freshIp();
    const r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: { ...browserHeaders(), "x-dev-client-ip": ip }, env: cap });
    const e = expectError(r, 429, "rate_limited");
    expect(e.message).toMatch(/busy/);
    expect(Number(r.res.headers.get("retry-after"))).toBeGreaterThan(0);
    expect(stripe.calls.filter((c) => c.path === "/v1/checkout/sessions")).toHaveLength(3);
    // The refused request didn't use up that IP's own allowance.
    const ipRow = await env.DB.prepare(`SELECT COUNT(*) AS n FROM rate_limits WHERE key = ?1`).bind(`store:ip:${ip}`).first<{ n: number }>();
    expect(ipRow!.n).toBe(0);
    // Default (300/hour): the same traffic still goes through.
    expect((await call("/api/store/checkout", { body: { quantity: 1 }, headers: { ...browserHeaders(), "x-dev-client-ip": ip } })).res.status).toBe(200);
  });

  it("a closed store doesn't count towards the global cap", async () => {
    fakeStripe();
    const cap = { STORE_GLOBAL_PER_HOUR: "1" };
    for (let i = 0; i < 3; i++) {
      const r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: { ...browserHeaders(), "x-dev-client-ip": freshIp() }, env: { ...cap, STORE_OPEN: "" } });
      expectError(r, 503, "sold_out");
    }
    const r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: { ...browserHeaders(), "x-dev-client-ip": freshIp() }, env: cap });
    expect(r.res.status, r.text).toBe(200);
  });
});

// ------------------------------------------------------------ store status
describe("GET /api/store/status", () => {
  it("open with Stripe set up and STORE_OPEN=1; closed otherwise; never calls Stripe", async () => {
    const stripe = fakeStripe();
    let r = await call("/api/store/status");
    expect(r.res.status, r.text).toBe(200);
    expect(r.json).toEqual({ open: true, max_quantity: 3 });
    expect(r.res.headers.get("cache-control")).toMatch(/max-age=60/);
    for (const off of [{ STORE_OPEN: "" }, { STORE_OPEN: "0" }, { STRIPE_SECRET_KEY: "" }, { STRIPE_PRICE_DEVICE: "" }]) {
      r = await call("/api/store/status", { env: off });
      expect(r.res.status, r.text).toBe(200);
      expect(r.json).toEqual({ open: false, max_quantity: 3 });
    }
    expect(stripe.calls).toHaveLength(0);
  });

  it("is GET only and needs no login or Origin", async () => {
    const r = await call("/api/store/status", { method: "POST", body: {}, headers: browserHeaders() });
    expectError(r, 405, "bad_request");
    expect(r.res.headers.get("allow")).toBe("GET");
  });

});

// ------------------------------------------------------------ webhook
describe("POST /api/stripe/webhook", () => {
  it("400 bad_signature for a missing, wrong or stale signature, and writes nothing", async () => {
    fakeStripe();
    const a = await customerAccount();
    const ev = event("customer.subscription.created", subscription(a.customer, "active"));
    const body = JSON.stringify(ev);
    expectError(await call("/api/stripe/webhook", { body, headers: { "content-type": "application/json" } }), 400, "bad_signature");
    expectError(await webhook(ev, { secret: "whsec_someoneElsesSecret0123456789" }), 400, "bad_signature");
    expectError(await webhook(ev, { t: now() - 301 }), 400, "bad_signature");
    expectError(await webhook(ev, { t: now() + 301 }), 400, "bad_signature");
    expectError(await webhook(ev, { signature: "t=1,v1=zz" }), 400, "bad_signature");
    // A body changed after signing fails too.
    const s = await signed(ev);
    const tampered = s.body.replace('"active"', '"trialing"');
    expectError(
      await call("/api/stripe/webhook", { body: tampered, headers: { "content-type": "application/json", "stripe-signature": s.signature } }),
      400,
      "bad_signature",
    );
    const n = await env.DB.prepare(`SELECT (SELECT COUNT(*) FROM subscriptions) + (SELECT COUNT(*) FROM stripe_events) AS n`).first<{ n: number }>();
    expect(n!.n).toBe(0);
  });

  it("400 bad_signature for a correctly signed event whose livemode doesn't match the key (S3-11)", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = subscription(a.customer, "active");
    const live = event("customer.subscription.created", sub, now(), { livemode: true });
    expectError(await webhook(live), 400, "bad_signature");
    expect(await env.DB.prepare(`SELECT COUNT(*) AS n FROM stripe_events`).first()).toEqual({ n: 0 });
    expect(stripe.calls.some((c) => c.path.startsWith("/v1/subscriptions/"))).toBe(false);
    // The same event in test mode (the key is sk_test_) is accepted.
    expect((await webhook(event("customer.subscription.created", sub, now(), { livemode: false }))).res.status).toBe(200);
    expect((await me(a.cookie)).plan).toBe("active");
  });

  it("503 until STRIPE_WEBHOOK_SECRET is set (Stripe keeps retrying)", async () => {
    const ev = event("customer.subscription.created", subscription("cus_x", "active"));
    const s = await signed(ev);
    const r = await call("/api/stripe/webhook", {
      body: s.body,
      headers: { "content-type": "application/json", "stripe-signature": s.signature },
      env: { STRIPE_WEBHOOK_SECRET: "" },
    });
    expectError(r, 503, "unavailable");
  });

  it("an active subscription lets the account's calculator solve; deleted stops it with the right message", async () => {
    fakeStripe();
    const { device, cookie, email } = await linkedDevice();
    // End the free month, then subscribe.
    await env.DB.prepare(`UPDATE users SET trial_ends_at = ?1 WHERE email = ?2`).bind(now() - 10, email).run();
    let st = await deviceStatus(device.token);
    expect(st.can_solve).toBe(false);
    expect(st.reason).toMatch(/^Your free month is over\. Subscribe at /);

    await call("/api/billing/checkout", { body: {}, headers: browserHeaders(cookie) });
    const cus = (await env.DB.prepare(`SELECT stripe_customer_id AS c FROM users WHERE email = ?1`).bind(email).first<{ c: string }>())!.c;
    const sub = subscription(cus, "active");
    expect((await webhook(event("customer.subscription.created", sub))).res.status).toBe(200);
    st = await deviceStatus(device.token);
    expect(st.can_solve).toBe(true);
    expect(st.plan).toBe("active");
    expect(st.period_end).toBe(sub.items.data[0]!.current_period_end);
    expect((await me(cookie)).plan).toBe("active");

    expect((await webhook(event("customer.subscription.deleted", { ...sub, status: "canceled" }, now() + 1))).res.status).toBe(200);
    st = await deviceStatus(device.token);
    expect(st.can_solve).toBe(false);
    expect(st.reason).toMatch(/^Your subscription has ended\. Renew at /);
    expect(st.reason.length).toBeLessThanOrEqual(110);
    expect((await me(cookie)).plan).toBe("inactive");
  });

  it("reads the period end from older API versions too (on the subscription itself)", async () => {
    fakeStripe();
    const a = await customerAccount();
    const end = now() + 12345;
    const sub = { ...subscription(a.customer, "active"), current_period_end: end, items: { data: [{ price: "price_subMonthly15" }] } };
    expect((await webhook(event("customer.subscription.created", sub))).res.status).toBe(200);
    const row = await env.DB.prepare(`SELECT current_period_end AS e, price_id AS p FROM subscriptions WHERE id = ?1`).bind(sub.id).first<{ e: number; p: string }>();
    expect(row).toEqual({ e: end, p: "price_subMonthly15" });
  });

  it("past_due still solves within 3 days of the failed payment (past_due_since), then stops (S3-1)", async () => {
    fakeStripe();
    const { device, cookie, email } = await linkedDevice();
    await env.DB.prepare(`UPDATE users SET trial_ends_at = ?1 WHERE email = ?2`).bind(now() - 10, email).run();
    await call("/api/billing/checkout", { body: {}, headers: browserHeaders(cookie) });
    const cus = (await env.DB.prepare(`SELECT stripe_customer_id AS c FROM users WHERE email = ?1`).bind(email).first<{ c: string }>())!.c;

    const sub = subscription(cus, "past_due");
    await webhook(event("customer.subscription.updated", sub));
    const since = async () =>
      (await env.DB.prepare(`SELECT past_due_since AS s FROM subscriptions WHERE id = ?1`).bind(sub.id).first<{ s: number | null }>())!.s;
    expect(await since()).toBe(now()); // stamped when first stored as past_due
    let st = await deviceStatus(device.token);
    expect(st.can_solve).toBe(true);
    expect(st.plan).toBe("past_due");

    // Another past_due event later doesn't restart the grace period.
    await env.DB.prepare(`UPDATE subscriptions SET past_due_since = ?1 WHERE id = ?2`).bind(now() - 2 * 86400, sub.id).run();
    await webhook(event("invoice.payment_failed", { id: "in_pd1", object: "invoice", subscription: sub.id }));
    expect(await since()).toBe(now() - 2 * 86400);
    expect((await deviceStatus(device.token)).can_solve).toBe(true);

    await env.DB.prepare(`UPDATE subscriptions SET past_due_since = ?1 WHERE id = ?2`).bind(now() - 3 * 86400 - 1, sub.id).run();
    st = await deviceStatus(device.token);
    expect(st.can_solve).toBe(false);
    expect(st.reason).toMatch(/subscription has ended/);

    // Paid again: active, and the past_due mark is cleared.
    await webhook(event("customer.subscription.updated", { ...sub, status: "active" }));
    expect(await since()).toBeNull();
    expect((await deviceStatus(device.token)).can_solve).toBe(true);
  });

  it("past_due with the period end far in the future but past_due_since 4 days ago is NOT entitled (S3-1)", async () => {
    fakeStripe();
    const a = await customerAccount();
    await env.DB.prepare(`UPDATE users SET trial_ends_at = ?1 WHERE id = ?2`).bind(now() - 10, a.userId).run();
    // Stripe moves the period end forward at renewal even when the charge fails.
    const sub = subscription(a.customer, "past_due", { current_period_end: now() + 29 * 86400 });
    await webhook(event("customer.subscription.updated", sub));
    await env.DB.prepare(`UPDATE subscriptions SET past_due_since = ?1 WHERE id = ?2`).bind(now() - 4 * 86400, sub.id).run();
    const m = await me(a.cookie);
    expect(m.plan).toBe("inactive");
    expect(m.period_end).toBe(now() + 29 * 86400);
  });

  it("a past_due row without past_due_since falls back to updated_at (S3-1)", async () => {
    fakeStripe();
    const a = await customerAccount();
    await env.DB.prepare(`UPDATE users SET trial_ends_at = ?1 WHERE id = ?2`).bind(now() - 10, a.userId).run();
    const ins = (id: string, updated: number) =>
      env.DB.prepare(
        `INSERT INTO subscriptions (id, user_id, status, price_id, current_period_end, cancel_at_period_end, updated_at, past_due_since)
         VALUES (?1, ?2, 'past_due', 'price_subMonthly15', ?3, 0, ?4, NULL)`,
      )
        .bind(id, a.userId, now() + 20 * 86400, updated)
        .run();
    await ins("sub_legacyRecent", now() - 86400);
    expect((await me(a.cookie)).plan).toBe("past_due");
    await env.DB.prepare(`DELETE FROM subscriptions`).run();
    await ins("sub_legacyOld", now() - 4 * 86400);
    expect((await me(a.cookie)).plan).toBe("inactive");
  });

  it("the same event twice: 200 both times, applied once", async () => {
    fakeStripe();
    const a = await customerAccount();
    const ev = event("customer.subscription.created", subscription(a.customer, "active"));
    const first = await webhook(ev);
    const second = await webhook(ev);
    expect(first.res.status).toBe(200);
    expect(second.res.status).toBe(200);
    expect(second.json.duplicate).toBe(true);
    const n = await env.DB.prepare(`SELECT COUNT(*) AS n FROM stripe_events WHERE id = ?1`).bind(ev.id).first<{ n: number }>();
    expect(n!.n).toBe(1);
  });

  it("an older event arriving late can't roll the state back: Stripe's current state is stored (S3-4)", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = stripe.seed(subscription(a.customer, "active"));
    await webhook(event("customer.subscription.updated", sub, now()));
    // The 'created' event (status incomplete) arrives after the 'updated' one.
    await webhook(event("customer.subscription.created", { ...sub, status: "incomplete" }, now() - 60), { seed: false });
    const row = await env.DB.prepare(`SELECT status, updated_at FROM subscriptions WHERE id = ?1`)
      .bind(sub.id)
      .first<{ status: string; updated_at: number }>();
    expect(row).toEqual({ status: "active", updated_at: now() }); // stamped with the processing time
    expect(stripe.calls.filter((c) => c.method === "GET" && c.path === `/v1/subscriptions/${sub.id}`)).toHaveLength(2);
  });

  it("out-of-order delivery ends with Stripe's current state, whatever the payloads say (S3-4)", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = subscription(a.customer, "active");
    // Stripe now: cancelled. The events arrive deleted-first, then stale 'active' and 'incomplete'.
    stripe.seed({ ...sub, status: "canceled" });
    await webhook(event("customer.subscription.deleted", { ...sub, status: "canceled" }, now()), { seed: false });
    await webhook(event("customer.subscription.updated", sub, now() - 30), { seed: false });
    await webhook(event("customer.subscription.created", { ...sub, status: "incomplete" }, now() - 60), { seed: false });
    const row = await env.DB.prepare(`SELECT status FROM subscriptions WHERE id = ?1`).bind(sub.id).first<{ status: string }>();
    expect(row!.status).toBe("canceled");
    expect((await me(a.cookie)).plan).not.toBe("active");

    // And a stale 'past_due' payload while Stripe says active stores active.
    const other = stripe.seed(subscription(a.customer, "active"));
    await webhook(event("customer.subscription.updated", { ...other, status: "past_due" }), { seed: false });
    const row2 = await env.DB.prepare(`SELECT status FROM subscriptions WHERE id = ?1`).bind(other.id).first<{ status: string }>();
    expect(row2!.status).toBe("active");
  });

  it("a subscription event for a subscription Stripe doesn't know: 503 (retried), nothing recorded", async () => {
    fakeStripe();
    const a = await customerAccount();
    const ev = event("customer.subscription.created", subscription(a.customer, "active"));
    expectError(await webhook(ev, { seed: false }), 503, "unavailable");
    expect(await env.DB.prepare(`SELECT COUNT(*) AS n FROM stripe_events`).first<{ n: number }>()).toEqual({ n: 0 });
  });

  it("maps by Stripe customer id only: forged metadata pointing at another account is ignored", async () => {
    fakeStripe();
    const victim = await customerAccount();
    const forged = subscription("cus_someoneElse0001", "active", { metadata: { user_id: victim.userId } });
    expect((await webhook(event("customer.subscription.created", forged))).res.status).toBe(200);
    const n = await env.DB.prepare(`SELECT COUNT(*) AS n FROM subscriptions WHERE user_id = ?1`).bind(victim.userId).first<{ n: number }>();
    expect(n!.n).toBe(0);
    expect((await me(victim.cookie)).plan).not.toBe("active");
  });

  it("a subscription to another price never unlocks AI (S3-5)", async () => {
    fakeStripe();
    const { device, cookie, email } = await linkedDevice();
    await env.DB.prepare(`UPDATE users SET trial_ends_at = ?1 WHERE email = ?2`).bind(now() - 10, email).run();
    await call("/api/billing/checkout", { body: {}, headers: browserHeaders(cookie) });
    const cus = (await env.DB.prepare(`SELECT stripe_customer_id AS c FROM users WHERE email = ?1`).bind(email).first<{ c: string }>())!.c;
    const other = subscription(cus, "active", {
      items: { data: [{ id: "si_other1", price: { id: "price_somethingElse9" }, current_period_end: now() + 30 * 86400 }] },
    });
    expect((await webhook(event("customer.subscription.created", other))).res.status).toBe(200);
    const row = await env.DB.prepare(`SELECT status, price_id FROM subscriptions WHERE id = ?1`).bind(other.id).first();
    expect(row).toEqual({ status: "active", price_id: "price_somethingElse9" }); // stored, but not ours
    let st = await deviceStatus(device.token);
    expect(st.can_solve).toBe(false);
    expect(st.reason).toMatch(/^Your free month is over\./);
    expect((await me(cookie)).plan).toBe("inactive");

    // Our price unlocks it.
    expect((await webhook(event("customer.subscription.created", subscription(cus, "active")))).res.status).toBe(200);
    st = await deviceStatus(device.token);
    expect(st.can_solve).toBe(true);
  });

  it("a live subscription wins over an older cancelled one", async () => {
    fakeStripe();
    const a = await customerAccount();
    const old = subscription(a.customer, "canceled");
    const fresh = subscription(a.customer, "active");
    await webhook(event("customer.subscription.updated", fresh, now() - 100));
    await webhook(event("customer.subscription.deleted", old, now())); // newer event, older subscription
    expect((await me(a.cookie)).plan).toBe("active");
  });

  it("invoice events re-read the subscription from Stripe (old and new invoice shapes)", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = subscription(a.customer, "past_due");
    stripe.subscriptions.set(sub.id, sub);
    await webhook(event("invoice.payment_failed", { id: "in_1", object: "invoice", subscription: sub.id }));
    expect((await env.DB.prepare(`SELECT status FROM subscriptions WHERE id = ?1`).bind(sub.id).first<{ status: string }>())!.status).toBe("past_due");

    sub.status = "active";
    await webhook(event("invoice.paid", { id: "in_2", object: "invoice", parent: { subscription_details: { subscription: sub.id } } }, now() + 1));
    expect((await env.DB.prepare(`SELECT status FROM subscriptions WHERE id = ?1`).bind(sub.id).first<{ status: string }>())!.status).toBe("active");
    expect(stripe.calls.filter((c) => c.method === "GET" && c.path === `/v1/subscriptions/${sub.id}`)).toHaveLength(2);
  });

  it("checkout.session.completed (subscription) re-reads the new subscription", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = subscription(a.customer, "active");
    stripe.subscriptions.set(sub.id, sub);
    const session = { id: "cs_test_sub1", object: "checkout.session", mode: "subscription", customer: a.customer, subscription: sub.id, client_reference_id: a.userId };
    expect((await webhook(event("checkout.session.completed", session))).res.status).toBe(200);
    expect((await me(a.cookie)).plan).toBe("active");
  });

  const order = (over: Record<string, any> = {}) => ({
    id: "cs_test_order1",
    object: "checkout.session",
    mode: "payment",
    metadata: { kind: "device" },
    payment_status: "paid",
    amount_total: 24300,
    currency: "usd",
    customer_details: { email: "parent@example.com" },
    collected_information: { shipping_details: { name: "A Parent", address: { line1: "1 Main St", country: "US" } } },
    ...over,
  });
  const orderRow = () => env.DB.prepare(`SELECT * FROM orders WHERE id = 'cs_test_order1'`).first<any>();

  it("checkout.session.completed (calculator order) stores the order", async () => {
    fakeStripe();
    expect((await webhook(event("checkout.session.completed", order()))).res.status).toBe(200);
    const o = await orderRow();
    expect(o.status).toBe("paid");
    expect(o.amount_total).toBe(24300);
    expect(o.email).toBe("parent@example.com");
    expect(JSON.parse(o.shipping_json).address.line1).toBe("1 Main St");
  });

  it("payment sessions that aren't our device orders are ignored (S3-6)", async () => {
    fakeStripe();
    for (const over of [{ metadata: {} }, { metadata: { kind: "gift" } }, { metadata: undefined }, { mode: "setup" }]) {
      expect((await webhook(event("checkout.session.completed", order(over)))).res.status).toBe(200);
      expect((await webhook(event("checkout.session.async_payment_failed", order(over)))).res.status).toBe(200);
    }
    expect(await env.DB.prepare(`SELECT COUNT(*) AS n FROM orders`).first()).toEqual({ n: 0 });
  });

  it("a paid order can't regress to unpaid, and paid/failed are final (S3-6)", async () => {
    fakeStripe();
    await webhook(event("checkout.session.completed", order()));
    await webhook(event("checkout.session.completed", order({ payment_status: "unpaid" })));
    expect((await orderRow()).status).toBe("paid");
    await webhook(event("checkout.session.async_payment_failed", order({ payment_status: "unpaid" })));
    expect((await orderRow()).status).toBe("paid");

    // A delayed payment: unpaid first, then it fails, and a late success can't flip it.
    await env.DB.prepare(`DELETE FROM orders`).run();
    await webhook(event("checkout.session.completed", order({ payment_status: "unpaid" })));
    expect((await orderRow()).status).toBe("unpaid");
    await webhook(event("checkout.session.async_payment_failed", order({ payment_status: "unpaid" })));
    expect((await orderRow()).status).toBe("failed");
    await webhook(event("checkout.session.async_payment_succeeded", order()));
    expect((await orderRow()).status).toBe("failed");

    // unpaid -> paid by async_payment_succeeded.
    await env.DB.prepare(`DELETE FROM orders`).run();
    await webhook(event("checkout.session.completed", order({ payment_status: "unpaid" })));
    await webhook(event("checkout.session.async_payment_succeeded", order()));
    expect((await orderRow()).status).toBe("paid");
  });

  it("unknown event types: 200 and ignored", async () => {
    fakeStripe();
    const r = await webhook(event("radar.early_fraud_warning.created", { id: "issfr_1" }));
    expect(r.res.status).toBe(200);
    expect(await env.DB.prepare(`SELECT COUNT(*) AS n FROM subscriptions`).first<{ n: number }>()).toEqual({ n: 0 });
  });

  it("a Stripe failure while handling: 503 and nothing recorded, so the retry still applies it", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = subscription(a.customer, "active");
    stripe.subscriptions.set(sub.id, sub);
    const ev = event("invoice.paid", { id: "in_3", object: "invoice", subscription: sub.id });
    stripe.failNext(`/v1/subscriptions/${sub.id}`, 500);
    expectError(await webhook(ev), 503, "unavailable");
    expect(await env.DB.prepare(`SELECT COUNT(*) AS n FROM stripe_events WHERE id = ?1`).bind(ev.id).first<{ n: number }>()).toEqual({ n: 0 });
    expect((await webhook(ev)).res.status).toBe(200); // Stripe's retry
    expect((await me(a.cookie)).plan).toBe("active");
  });
});

// ------------------------------------------------------------ account deletion
describe("DELETE /api/me with a subscription", () => {
  const del = (a: { email: string; cookie: string }) =>
    call("/api/me", { method: "DELETE", body: { confirm_email: a.email }, headers: browserHeaders(a.cookie) });

  it("deletes the Stripe customer (cancelling its subscriptions), then erases the account (S3-2)", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = subscription(a.customer, "active");
    await webhook(event("customer.subscription.created", sub));
    const r = await del(a);
    expect(r.res.status, r.text).toBe(204);
    expect(stripe.calls.filter((c) => c.method === "DELETE").map((c) => c.path)).toEqual([`/v1/customers/${a.customer}`]);
    expect(stripe.subscriptions.get(sub.id)!.status).toBe("canceled");
    const row = await env.DB.prepare(`SELECT status, past_due_since FROM subscriptions WHERE id = ?1`).bind(sub.id).first();
    expect(row).toEqual({ status: "canceled", past_due_since: null });
    const u = await env.DB.prepare(`SELECT email, deleted_at FROM users WHERE id = ?1`).bind(a.userId).first<{ email: string; deleted_at: number }>();
    expect(u!.email).toBe(`deleted:${a.userId}`);
    expect(u!.deleted_at).toBe(now());
  });

  it("if Stripe can't delete the customer, nothing is erased (503) and the account still works", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    const sub = subscription(a.customer, "active");
    await webhook(event("customer.subscription.created", sub));
    stripe.failNext(`/v1/customers/${a.customer}`, 500);
    const e = expectError(await del(a), 503, "unavailable");
    expect(e.message).not.toMatch(/stripe|boom|500/i);
    expect((await me(a.cookie)).email).toBe(a.email);
    expect((await me(a.cookie)).plan).toBe("active");
    const row = await env.DB.prepare(`SELECT status FROM subscriptions WHERE id = ?1`).bind(sub.id).first<{ status: string }>();
    expect(row!.status).toBe("active");
  });

  it("a customer already gone at Stripe (404) doesn't block deletion", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    await webhook(event("customer.subscription.created", subscription(a.customer, "active")));
    stripe.deletedCustomers.add(a.customer); // the fake answers 404 for it now
    const r = await del(a);
    expect(r.res.status, r.text).toBe(204);
    expect((await env.DB.prepare(`SELECT status FROM subscriptions WHERE user_id = ?1`).bind(a.userId).first<{ status: string }>())!.status).toBe("canceled");
  });

  it("an account that never reached Checkout is erased without calling Stripe", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    const r = await call("/api/me", { method: "DELETE", body: { confirm_email: s.email }, headers: browserHeaders(s.cookie) });
    expect(r.res.status, r.text).toBe(204);
    expect(stripe.calls).toHaveLength(0);
  });

  it("a live subscription arriving for a deleted account is cancelled at Stripe and stored canceled (S3-2)", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    expect((await del(a)).res.status).toBe(204);
    // A Checkout page that was still open completes after the deletion.
    const sub = subscription(a.customer, "active");
    const r = await webhook(event("customer.subscription.created", sub));
    expect(r.res.status, r.text).toBe(200);
    const cancel = stripe.calls.find((c) => c.method === "DELETE" && c.path === `/v1/subscriptions/${sub.id}`);
    expect(cancel).toBeDefined();
    expect(cancel!.headers["idempotency-key"]).toBe(`cancel-${sub.id}`);
    expect(stripe.subscriptions.get(sub.id)!.status).toBe("canceled");
    const row = await env.DB.prepare(`SELECT status, user_id FROM subscriptions WHERE id = ?1`).bind(sub.id).first();
    expect(row).toEqual({ status: "canceled", user_id: a.userId });
  });

  it("for a deleted account: a failed cancel answers 503 (Stripe retries); 404 counts as cancelled", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    expect((await del(a)).res.status).toBe(204);
    const sub = subscription(a.customer, "active");
    const ev = event("customer.subscription.created", sub);
    stripe.failNext(`/v1/subscriptions/${sub.id}`, 500, "DELETE");
    expectError(await webhook(ev), 503, "unavailable");
    expect(await env.DB.prepare(`SELECT COUNT(*) AS n FROM stripe_events WHERE id = ?1`).bind(ev.id).first()).toEqual({ n: 0 });
    expect(await env.DB.prepare(`SELECT COUNT(*) AS n FROM subscriptions`).first()).toEqual({ n: 0 });
    expect((await webhook(ev)).res.status).toBe(200); // Stripe's retry cancels it
    expect(stripe.subscriptions.get(sub.id)!.status).toBe("canceled");

    // Already gone at Stripe (404 on the cancel): stored canceled anyway.
    const gone = subscription(a.customer, "active");
    stripe.failNext(`/v1/subscriptions/${gone.id}`, 404, "DELETE");
    expect((await webhook(event("customer.subscription.created", gone))).res.status).toBe(200);
    const row = await env.DB.prepare(`SELECT status FROM subscriptions WHERE id = ?1`).bind(gone.id).first<{ status: string }>();
    expect(row!.status).toBe("canceled");
  });

  it("an ended subscription for a deleted account is just stored, not cancelled again", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    expect((await del(a)).res.status).toBe(204);
    const sub = subscription(a.customer, "canceled");
    expect((await webhook(event("customer.subscription.deleted", sub))).res.status).toBe(200);
    expect(stripe.calls.some((c) => c.method === "DELETE" && c.path.startsWith("/v1/subscriptions/"))).toBe(false);
  });
});

// ------------------------------------------------------------ waitlist
describe("POST /api/waitlist", () => {
  // The 13+ box is ticked unless a test says otherwise (more in waitlist.test.ts).
  const join = (body: object, headers: Record<string, string> = browserHeaders(), e = {}) =>
    call("/api/waitlist", { body: { age_ok: true, ...body }, headers, env: e });

  it("201 for a new or repeated address (no way to tell which), one row", async () => {
    const a = await join({ email: "Kid.Parent@Example.com", source: "hero" });
    const b = await join({ email: "kid.parent@example.com", source: "footer" });
    expect(a.res.status, a.text).toBe(201);
    expect(b.res.status).toBe(201);
    expect(a.json).toEqual(b.json);
    const rows = await env.DB.prepare(`SELECT email, source FROM waitlist`).all();
    expect(rows.results).toEqual([{ email: "kid.parent@example.com", source: "hero" }]);
  });

  it("refuses a bad email, unknown fields, and other origins", async () => {
    expectError(await join({ email: "not-an-email" }), 400, "bad_request");
    expectError(await join({ email: "a@b.co", phone: "1" }), 400, "bad_request");
    expectError(await join({ email: "a@b.co" }, { ...browserHeaders(), origin: "https://evil.example" }), 403, "forbidden");
  });

  it("a plain HTML form post (no JavaScript) gets a 303 back to the site", async () => {
    const r = await call("/api/waitlist", {
      body: "email=form%40example.com&company=&age_ok=on",
      headers: { ...browserHeaders(), "content-type": "application/x-www-form-urlencoded" },
    });
    expect(r.res.status).toBe(303);
    expect(r.res.headers.get("location")).toBe(`${ORIGIN}/?waitlist=joined#waitlist`);
    expect(await env.DB.prepare(`SELECT source FROM waitlist WHERE email = 'form@example.com'`).first()).toEqual({ source: "site" });
  });

  it("the hidden honeypot field: looks successful, stores nothing", async () => {
    const r = await join({ email: "bot@example.com", company: "Spam Inc" });
    expect(r.res.status).toBe(201);
    expect(await env.DB.prepare(`SELECT COUNT(*) AS n FROM waitlist`).first()).toEqual({ n: 0 });
  });

  it("5 per IP per hour, and a global daily cap", async () => {
    const h = { ...browserHeaders(), "x-dev-client-ip": freshIp() };
    for (let i = 0; i < 5; i++) expect((await join({ email: `p${i}@example.com` }, h)).res.status).toBe(201);
    expectError(await join({ email: "p6@example.com" }, h), 429, "rate_limited");

    const cap = { WAITLIST_DAILY_CAP: "6" }; // 5 already joined today: one more, then full
    expect((await join({ email: "q1@example.com" }, browserHeaders(), cap)).res.status).toBe(201);
    expectError(await join({ email: "q2@example.com" }, browserHeaders(), cap), 429, "rate_limited");
  });
});

// ------------------------------------------------------------ config safety
describe("Stripe settings fail closed", () => {
  it("refuses a live key with DEV_MODE, a plain-http Stripe address, and a malformed price", async () => {
    for (const bad of [
      // Built in pieces so secret scanners don't mistake this fake for a real key.
      { STRIPE_SECRET_KEY: ["sk", "live", "notARealKeyJustForThisTest"].join("_") },
      { STRIPE_API_BASE: "http://stripe.example.com" },
      { STRIPE_PRICE_SUB: "15dollars" },
      { STRIPE_WEBHOOK_SECRET: "nope" },
    ]) {
      expectError(await call("/healthz", { env: bad }), 503, "unavailable");
    }
  });

  it("device_not_linked etc. are unchanged for calculators (no Stripe calls on the solve path)", async () => {
    const stripe = fakeStripe();
    const { device } = await linkedDevice();
    await call("/v1/device/status", { headers: deviceHeaders(device) });
    expect(stripe.calls).toHaveLength(0);
  });
});
