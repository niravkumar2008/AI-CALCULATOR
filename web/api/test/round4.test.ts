// Tests for the stage 3 security review, round 2 (S4-*).
import { env } from "cloudflare:workers";
import { describe, expect, it } from "vitest";
import { browserHeaders, call, dbFirst, expectError, freshIp, linkedDevice, signIn } from "./helpers";
import { fakeStripe } from "./stripe-fake";

const now = () => Math.floor(Date.now() / 1000);

describe("S4-1: one address can't use up the store's global hourly cap", () => {
  it("requests refused by the per-IP limit don't count toward the global cap", async () => {
    fakeStripe();
    const over = { STORE_GLOBAL_PER_HOUR: "15" };
    const attacker = { ...browserHeaders(), "x-dev-client-ip": freshIp() };
    for (let i = 0; i < 40; i++) await call("/api/store/checkout", { body: { quantity: 1 }, headers: attacker, env: over });
    // The attacker got its 10; a real customer from another address still can buy.
    const r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: browserHeaders(), env: over });
    expect(r.res.status, r.text).toBe(200);
  });

  it("the global cap still holds across many addresses (refused before any per-IP write)", async () => {
    fakeStripe();
    const over = { STORE_GLOBAL_PER_HOUR: "3" };
    for (let i = 0; i < 3; i++) {
      expect((await call("/api/store/checkout", { body: { quantity: 1 }, headers: browserHeaders(), env: over })).res.status).toBe(200);
    }
    const ip = freshIp();
    const r = await call("/api/store/checkout", { body: { quantity: 1 }, headers: { ...browserHeaders(), "x-dev-client-ip": ip }, env: over });
    expectError(r, 429, "rate_limited");
    expect(r.json.error.message).toMatch(/store is busy/);
    expect(await dbFirst(`SELECT 1 FROM rate_limits WHERE key = ?1`, `store:ip:${ip}`)).toBeNull();
  });
});

describe("S4-2: a malformed %-escape in a device path is a 404, not a server error", () => {
  it("404 not_found", async () => {
    const s = await signIn();
    for (const p of ["/api/devices/%E0%A4%A/revoke", "/api/devices/%zz"]) {
      const method = p.endsWith("revoke") ? "POST" : "PATCH";
      const r = await call(p, { method, body: { label: "x" }, headers: browserHeaders(s.cookie) });
      expectError(r, 404, "not_found");
    }
  });
});

describe("S4-3: account deletion also erases device names", () => {
  it("labels are cleared", async () => {
    fakeStripe();
    const { cookie, email, device } = await linkedDevice();
    const p = await call(`/api/devices/${device.device_id}`, {
      method: "PATCH",
      body: { label: "Emma's calculator" },
      headers: browserHeaders(cookie),
    });
    expect(p.res.status, p.text).toBe(200);
    const d = await call("/api/me", { method: "DELETE", body: { confirm_email: email }, headers: browserHeaders(cookie) });
    expect(d.res.status, d.text).toBe(204);
    const row = await dbFirst<{ label: string | null; revoked_at: number | null }>(
      `SELECT label, revoked_at FROM devices WHERE id = ?1`,
      device.device_id,
    );
    expect(row!.revoked_at).not.toBeNull();
    expect(row!.label).toBeNull();
  });
});

describe("S4-4: a checkout racing an account deletion can't attach a customer to the deleted account", () => {
  it("the new customer is deleted at Stripe and no checkout page is made", async () => {
    const stripe = fakeStripe();
    const s = await signIn();
    const user = await dbFirst<{ id: string }>(`SELECT id FROM users WHERE email = ?1`, s.email);
    // The account is deleted (in another tab) while the customer is being created at Stripe.
    stripe.beforeAnswer = async (c) => {
      if (c.method === "POST" && c.path === "/v1/customers") {
        await env.DB.prepare(`UPDATE users SET email = ?1, deleted_at = ?2 WHERE id = ?3`)
          .bind(`deleted:${user!.id}`, now(), user!.id)
          .run();
        await env.DB.prepare(`DELETE FROM sessions WHERE user_id = ?1`).bind(user!.id).run();
      }
    };
    const r = await call("/api/billing/checkout", { body: {}, headers: browserHeaders(s.cookie) });
    expectError(r, 401, "unauthorized");
    expect(stripe.calls.some((c) => c.path === "/v1/checkout/sessions")).toBe(false);
    const row = await dbFirst<{ stripe_customer_id: string | null }>(`SELECT stripe_customer_id FROM users WHERE id = ?1`, user!.id);
    expect(row!.stripe_customer_id).toBeNull();
    expect(stripe.deletedCustomers.size).toBe(1);
  });
});
