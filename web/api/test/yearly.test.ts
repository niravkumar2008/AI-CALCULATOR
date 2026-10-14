// The AI tutor plan monthly ($10, STRIPE_PRICE_SUB) or yearly ($100, STRIPE_PRICE_SUB_YEARLY).
import { env } from "cloudflare:workers";
import { describe, expect, it } from "vitest";
import { sha256Hex } from "../src/crypto";
import { config, type Env } from "../src/env";
import { browserHeaders, call, expectError, linkedDevice, signIn, testEnv } from "./helpers";
import { event, fakeStripe, seedFromEvent, signed, subscription } from "./stripe-fake";

const MONTHLY = "price_subMonthly15";
const YEARLY = "price_subYearly100";

async function webhook(ev: object) {
  seedFromEvent(ev as { data?: { object?: Record<string, any> } });
  const s = await signed(ev);
  return call("/api/stripe/webhook", {
    method: "POST",
    body: s.body,
    headers: { "content-type": "application/json", "stripe-signature": s.signature },
  });
}

function priced(customer: string, status: string, price: string, over: Record<string, any> = {}) {
  const sub = subscription(customer, status, over);
  sub.items.data[0]!.price = { id: price };
  return sub;
}

async function customerAccount(cookie?: string) {
  const s = cookie ? { cookie, email: "" } : await signIn();
  const r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) });
  expect(r.res.status, r.text).toBe(200);
  const sid = s.cookie.slice("sid=".length);
  const row = await env.DB.prepare(
    `SELECT u.id, u.stripe_customer_id FROM sessions x JOIN users u ON u.id = x.user_id WHERE x.id_hash = ?1`,
  )
    .bind(await sha256Hex(sid))
    .first<{ id: string; stripe_customer_id: string }>();
  return { ...s, userId: row!.id, customer: row!.stripe_customer_id };
}

async function me(cookie: string) {
  const r = await call("/api/me", { headers: { cookie } });
  expect(r.res.status, r.text).toBe(200);
  return r.json;
}

function sessionPrice(stripe: ReturnType<typeof fakeStripe>): string | undefined {
  return stripe.calls.filter((c) => c.path === "/v1/checkout/sessions").at(-1)?.params["line_items[0][price]"];
}

describe("POST /api/billing/checkout {interval}", () => {
  it("no body, {} and month use the monthly price; year uses the yearly price", async () => {
    const stripe = fakeStripe();
    for (const [body, price] of [
      [undefined, MONTHLY],
      [{}, MONTHLY],
      [{ interval: "month" }, MONTHLY],
      [{ interval: "year" }, YEARLY],
    ] as const) {
      const s = await signIn();
      const h = browserHeaders(s.cookie);
      const r = await call("/api/billing/checkout", body === undefined ? { method: "POST", headers: h } : { body, headers: h });
      expect(r.res.status, r.text).toBe(200);
      expect(sessionPrice(stripe)).toBe(price);
    }
  });

  it("400 for any other interval or field", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    for (const body of [{ interval: "week" }, { interval: "" }, { interval: null }, { interval: 12 }, { interval: "Year" }, { plan: "x" }]) {
      expectError(await call("/api/billing/checkout", { body, headers: browserHeaders(s.cookie) }), 400, "bad_request");
    }
    expect(stripe.calls).toHaveLength(0);
  });

  it("year while yearly isn't offered: 400 bad_request, nothing at Stripe", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    const r = await call("/api/billing/checkout", {
      body: { interval: "year" },
      headers: browserHeaders(s.cookie),
      env: { STRIPE_PRICE_SUB_YEARLY: "" },
    });
    expect(expectError(r, 400, "bad_request").message).toMatch(/Yearly/);
    expect(stripe.calls).toHaveLength(0);
    // Monthly still works.
    const m = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie), env: { STRIPE_PRICE_SUB_YEARLY: "" } });
    expect(m.res.status, m.text).toBe(200);
  });

  it("a subscription on either price blocks a second one (local rows)", async () => {
    fakeStripe();
    for (const [have, want] of [
      [YEARLY, "month"],
      [MONTHLY, "year"],
      [YEARLY, "year"],
    ] as const) {
      const a = await customerAccount();
      expect((await webhook(event("customer.subscription.updated", priced(a.customer, "active", have)))).res.status).toBe(200);
      expectError(
        await call("/api/billing/checkout", { body: { interval: want }, headers: browserHeaders(a.cookie) }),
        409,
        "already_subscribed",
      );
    }
  });

  it("a subscription on either price blocks a second one (only at Stripe so far)", async () => {
    const stripe = fakeStripe();
    const a = await customerAccount();
    stripe.seed(priced(a.customer, "active", YEARLY)); // paid in another tab; webhook not here yet
    expectError(await call("/api/billing/checkout", { body: {}, headers: browserHeaders(a.cookie) }), 409, "already_subscribed");
  });
});

describe("entitlement for both intervals", () => {
  it("a yearly subscription unlocks AI and shows plan_interval year", async () => {
    fakeStripe();
    const { device, cookie } = await linkedDevice();
    await env.DB.prepare(`UPDATE users SET trial_ends_at = 1`).run(); // free month over
    const a = await customerAccount(cookie);
    expect((await me(cookie)).plan_interval).toBeNull();
    expect((await webhook(event("customer.subscription.created", priced(a.customer, "active", YEARLY)))).res.status).toBe(200);
    const m = await me(cookie);
    expect(m.plan).toBe("active");
    expect(m.plan_interval).toBe("year");
    const st = await call("/v1/device/status", { headers: { authorization: `Bearer ${device.token}` } });
    expect(st.json.can_solve, st.text).toBe(true);
  });

  it("switching month -> year -> month in the portal keeps AI on the whole time", async () => {
    fakeStripe();
    const { cookie } = await linkedDevice();
    await env.DB.prepare(`UPDATE users SET trial_ends_at = 1`).run();
    const a = await customerAccount(cookie);
    const sub = priced(a.customer, "active", MONTHLY);
    expect((await webhook(event("customer.subscription.created", sub))).res.status).toBe(200);
    expect(await me(cookie)).toMatchObject({ plan: "active", plan_interval: "month" });
    const toYear = priced(a.customer, "active", YEARLY, { id: sub.id });
    expect((await webhook(event("customer.subscription.updated", toYear))).res.status).toBe(200);
    expect(await me(cookie)).toMatchObject({ plan: "active", plan_interval: "year" });
    const back = priced(a.customer, "active", MONTHLY, { id: sub.id });
    expect((await webhook(event("customer.subscription.updated", back))).res.status).toBe(200);
    expect(await me(cookie)).toMatchObject({ plan: "active", plan_interval: "month" });
  });

  it("past_due on the yearly price keeps plan_interval during the grace; other prices never count", async () => {
    fakeStripe();
    const { cookie } = await linkedDevice();
    await env.DB.prepare(`UPDATE users SET trial_ends_at = 1`).run();
    const a = await customerAccount(cookie);
    const other = priced(a.customer, "active", "price_somethingElse1");
    expect((await webhook(event("customer.subscription.created", other))).res.status).toBe(200);
    expect(await me(cookie)).toMatchObject({ plan: "inactive", plan_interval: null });
    expect((await webhook(event("customer.subscription.created", priced(a.customer, "past_due", YEARLY)))).res.status).toBe(200);
    expect(await me(cookie)).toMatchObject({ plan: "past_due", plan_interval: "year" });
  });

  it("with yearly not configured, a yearly subscription doesn't unlock AI", async () => {
    fakeStripe();
    const { cookie } = await linkedDevice();
    await env.DB.prepare(`UPDATE users SET trial_ends_at = 1`).run();
    const a = await customerAccount(cookie);
    expect((await webhook(event("customer.subscription.created", priced(a.customer, "active", YEARLY)))).res.status).toBe(200);
    const r = await call("/api/me", { headers: { cookie }, env: { STRIPE_PRICE_SUB_YEARLY: "" } });
    expect(r.json).toMatchObject({ plan: "inactive", plan_interval: null });
  });
});

describe("settings", () => {
  const base = (over: Partial<Env>) => testEnv({ DEV_MODE: "1", ...over });

  it("STRIPE_PRICE_SUB_YEARLY must be a price id, needs the monthly price, and must differ", () => {
    expect(config(base({})).stripe.priceSubYearly).toBe(YEARLY);
    for (const over of [
      { STRIPE_PRICE_SUB_YEARLY: "100dollars" },
      { STRIPE_PRICE_SUB: "", STRIPE_PRICE_SUB_YEARLY: YEARLY },
      { STRIPE_PRICE_SUB_YEARLY: MONTHLY },
      { STRIPE_PRICE_SUB_YEARLY: "price_device225" },
    ]) {
      expect(() => config(base(over)), JSON.stringify(over)).toThrow();
    }
  });

  it("the monthly AI spend cap defaults to $5", () => {
    expect(config(base({ MONTHLY_SPEND_USD: undefined })).monthlySpendMicro).toBe(5_000_000);
  });
});
