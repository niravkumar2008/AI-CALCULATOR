// Store launch (2026-11-18): several shipping rates, promotion codes, the ship-time line on
// Checkout and the store status, and STORE_STOCK (a batch can't be oversold).

import { env } from "cloudflare:workers";
import { beforeEach, describe, expect, it } from "vitest";
import { browserHeaders, call, expectError, freshIp } from "./helpers";
import { event, fakeStripe, signed } from "./stripe-fake";

const now = () => Math.floor(Date.now() / 1000);
const headers = () => ({ ...browserHeaders(), "x-dev-client-ip": freshIp() });
const buy = (quantity: number, vars: Record<string, string> = {}) =>
  call("/api/store/checkout", { body: { quantity }, headers: headers(), env: vars });

async function webhook(ev: object) {
  const s = await signed(ev);
  return call("/api/stripe/webhook", {
    method: "POST",
    body: s.body,
    headers: { "content-type": "application/json", "stripe-signature": s.signature },
  });
}

const sessionId = (r: { json: any }) => String(r.json.url).split("/").pop()!;
const deviceSession = (id: string, quantity: number, over: Record<string, any> = {}) => ({
  id,
  object: "checkout.session",
  mode: "payment",
  metadata: { kind: "device", quantity: String(quantity) },
  payment_status: "paid",
  amount_total: 21500 * quantity,
  currency: "usd",
  customer_details: { email: "buyer@example.com" },
  ...over,
});

beforeEach(async () => {
  await env.DB.prepare(`DELETE FROM orders`).run();
});

describe("store checkout settings", () => {
  it("offers every shipping rate in STRIPE_SHIPPING_RATE, first one first", async () => {
    const stripe = fakeStripe();
    const r = await buy(1, { STRIPE_SHIPPING_RATE: "shr_1Standard, shr_1Priority" });
    expect(r.res.status, r.text).toBe(200);
    const p = stripe.calls.at(-1)!.params;
    expect(p["shipping_options[0][shipping_rate]"]).toBe("shr_1Standard");
    expect(p["shipping_options[1][shipping_rate]"]).toBe("shr_1Priority");
    expectError(await call("/healthz", { env: { STRIPE_SHIPPING_RATE: "shr_1Standard,nope" } }), 503, "unavailable");
  });

  it("promotion codes only with STRIPE_PROMO_CODES=1", async () => {
    const stripe = fakeStripe();
    expect((await buy(1)).res.status).toBe(200);
    expect(stripe.calls.at(-1)!.params["allow_promotion_codes"]).toBeUndefined();
    expect((await buy(1, { STRIPE_PROMO_CODES: "1" })).res.status).toBe(200);
    expect(stripe.calls.at(-1)!.params["allow_promotion_codes"]).toBe("true");
  });

  it("shows the ship time above the Pay button and keeps the Terms consent", async () => {
    const stripe = fakeStripe();
    expect((await buy(2, { STORE_SHIP_NOTE: "Made to order: ships in 3-5 weeks" })).res.status).toBe(200);
    const p = stripe.calls.at(-1)!.params;
    expect(p["custom_text[submit][message]"]).toBe("Made to order: ships in 3-5 weeks. We email you tracking when it ships.");
    expect(p["custom_text[terms_of_service_acceptance][message]"]).toContain("18 or older");
    expect(p["metadata[quantity]"]).toBe("2");
    expectError(await call("/healthz", { env: { STORE_SHIP_NOTE: "<b>soon</b>" } }), 503, "unavailable");
  });

  it("status carries the ship note", async () => {
    const r = await call("/api/store/status", { env: { STORE_SHIP_NOTE: "Ships in 3-5 weeks" } });
    expect(r.json).toEqual({ open: true, max_quantity: 3, ships: "Ships in 3-5 weeks" });
  });
});

describe("STORE_STOCK", () => {
  const stock = (n: number) => ({ STORE_STOCK: String(n) });

  it("holds calculators while Checkout is open and refuses more than the batch", async () => {
    fakeStripe();
    const a = await buy(3, stock(4));
    expect(a.res.status, a.text).toBe(200);
    // The hold carries the Checkout session id.
    const held = await env.DB.prepare(`SELECT id, status, quantity FROM orders`).all<any>();
    expect(held.results).toEqual([{ id: sessionId(a), status: "open", quantity: 3 }]);

    let s = await call("/api/store/status", { env: stock(4) });
    expect(s.json).toMatchObject({ open: true, sold_out: false, max_quantity: 1, left: 1 });

    const tooMany = await buy(2, stock(4));
    expectError(tooMany, 400, "bad_request");
    expect(tooMany.json.error.message).toContain("Only 1 left");
    expect((await buy(1, stock(4))).res.status).toBe(200);

    expectError(await buy(1, stock(4)), 503, "sold_out");
    s = await call("/api/store/status", { env: stock(4) });
    expect(s.json).toMatchObject({ open: false, sold_out: true, left: 0 });
  });

  it("paid orders keep their calculators; expired ones give them back", async () => {
    fakeStripe();
    const a = await buy(2, stock(3));
    const b = await buy(1, stock(3));
    expectError(await buy(1, stock(3)), 503, "sold_out");

    // a pays: still taken (now a paid order with its amount)
    expect((await webhook(event("checkout.session.completed", deviceSession(sessionId(a), 2)))).res.status).toBe(200);
    const paid = await env.DB.prepare(`SELECT status, quantity, amount_total FROM orders WHERE id = ?1`)
      .bind(sessionId(a))
      .first<any>();
    expect(paid).toEqual({ status: "paid", quantity: 2, amount_total: 43000 });
    expectError(await buy(1, stock(3)), 503, "sold_out");

    // b leaves the Checkout page: its calculator is free again at once
    const expired = deviceSession(sessionId(b), 1, { payment_status: "unpaid", status: "expired" });
    expect((await webhook(event("checkout.session.expired", expired))).res.status).toBe(200);
    expect((await buy(1, stock(3))).res.status).toBe(200);
  });

  it("a full refund gives the calculators back", async () => {
    fakeStripe();
    const a = await buy(2, stock(2));
    const paid = deviceSession(sessionId(a), 2, { payment_intent: "pi_3TestRefund1" });
    expect((await webhook(event("checkout.session.completed", paid))).res.status).toBe(200);
    expectError(await buy(1, stock(2)), 503, "sold_out");
    const charge = { id: "ch_1", object: "charge", payment_intent: "pi_3TestRefund1", amount: 43000, amount_refunded: 43000, refunded: true };
    expect((await webhook(event("charge.refunded", charge))).res.status).toBe(200);
    const again = await buy(2, stock(2));
    expect(again.res.status, JSON.stringify((await env.DB.prepare(`SELECT * FROM orders`).all()).results) + again.text).toBe(200);
  });

  it("a page left open longer than Checkout's lifetime stops holding stock", async () => {
    fakeStripe();
    await env.DB.prepare(`INSERT INTO orders (id, status, quantity, created_at) VALUES ('cs_test_old', 'open', 3, ?1)`)
      .bind(now() - 40 * 60)
      .run();
    expect((await buy(3, stock(3))).res.status).toBe(200);
  });

  it("a failed Stripe call drops the hold", async () => {
    const r = await call("/api/store/checkout", {
      body: { quantity: 1 },
      headers: headers(),
      env: { ...stock(1), STRIPE_API_BASE: "http://127.0.0.1:9" },
    });
    expect(r.res.status).toBe(503);
    const n = await env.DB.prepare(`SELECT COUNT(*) AS n FROM orders`).first<{ n: number }>();
    expect(n!.n).toBe(0);
  });

  it("no limit when STORE_STOCK is unset: status has no count", async () => {
    const r = await call("/api/store/status");
    expect(r.json).toEqual({ open: true, max_quantity: 3 });
  });
});
