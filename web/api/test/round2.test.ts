// Tests for the security re-review round 2 fixes.
import { env } from "cloudflare:workers";
import { describe, expect, it } from "vitest";
import { hmacHex } from "../src/crypto";
import { ipKey } from "../src/util";
import {
  browserHeaders,
  call,
  claimCode,
  dbFirst,
  enrolBoard,
  expectError,
  nextHwId,
  registerDevice,
  signIn,
} from "./helpers";

const TEST_PEPPER = "test-trial-pepper-0123456789abcdefghijklmnop"; // vitest.config.ts

function reg(ip: string, over = {}) {
  return call("/v1/device/register", { body: { hw_id: "calc-0123456789ab" }, headers: { "x-dev-client-ip": ip }, env: over });
}

describe("R2-1: IPv6 /64 keys and the global register cap", () => {
  it("ipKey", () => {
    expect(ipKey("203.0.113.7")).toBe("203.0.113.7");
    expect(ipKey("2001:db8:abcd:12:1::5")).toBe("2001:db8:abcd:12::/64");
    expect(ipKey("2001:0DB8:ABCD:0012:ffff:ffff:ffff:ffff")).toBe("2001:db8:abcd:12::/64");
    expect(ipKey("2001:db8::1")).toBe("2001:db8:0:0::/64");
    expect(ipKey("::ffff:198.51.100.4")).toBe("198.51.100.4");
    expect(ipKey("[2001:db8:1:2::9]")).toBe("2001:db8:1:2::/64");
  });

  it("addresses in one IPv6 /64 share the per-IP register limit", async () => {
    for (let i = 1; i <= 5; i++) expect((await reg(`2001:db8:aaaa:1::${i}`)).res.status).toBe(201);
    expectError(await reg("2001:db8:aaaa:1:ffff::77"), 429, "rate_limited");
    expect((await reg("2001:db8:aaaa:2::1")).res.status).toBe(201); // another /64
  });

  it("global cap: refused with a read only, nothing written for the refused request", async () => {
    const over = { REGISTER_GLOBAL_PER_HOUR: "3" };
    for (let i = 1; i <= 3; i++) expect((await reg(`198.51.100.${i}`, over)).res.status).toBe(201);
    const r = await reg("198.51.100.99", over);
    expectError(r, 429, "rate_limited");
    expect(Number(r.res.headers.get("retry-after"))).toBeGreaterThan(0);
    expect(await dbFirst(`SELECT * FROM rate_limits WHERE key = 'register:ip:198.51.100.99'`)).toBeNull();
    expect((await dbFirst<{ count: number }>(`SELECT count FROM rate_limits WHERE key = 'register:global'`))!.count).toBe(3);
  });
});

describe("R2-3: peppered e-mail hashes", () => {
  it("trials_used stores HMAC-SHA256(TRIAL_PEPPER, email), not a plain hash", async () => {
    const s = await signIn();
    const d = await registerDevice();
    await claimCode(s.cookie, d.pair_code);
    const row = await dbFirst<{ email_hash: string }>(`SELECT email_hash FROM trials_used`);
    expect(row!.email_hash).toBe(await hmacHex(TEST_PEPPER, s.email.toLowerCase()));
    const plain = [...new Uint8Array(await crypto.subtle.digest("SHA-256", new TextEncoder().encode(s.email.toLowerCase())))]
      .map((b) => b.toString(16).padStart(2, "0"))
      .join("");
    expect(row!.email_hash).not.toBe(plain);
    const keys = await env.DB.prepare(`SELECT key FROM rate_limits WHERE key LIKE 'login:email:%'`).all<{ key: string }>();
    expect(keys.results.map((k) => k.key)).toContain(`login:email:${row!.email_hash}`);
  });

  it("production refuses to run without a 32+ char TRIAL_PEPPER; DEV_MODE uses a fixed one", async () => {
    const prod = { DEV_MODE: "", PUBLIC_URL: "https://aicalc.example" };
    expectError(await call("/healthz", { env: { ...prod, TRIAL_PEPPER: "" } }), 503, "unavailable");
    expectError(await call("/healthz", { env: { ...prod, TRIAL_PEPPER: "short" } }), 503, "unavailable");
    expect((await call("/healthz", { env: prod })).res.status).toBe(200);
    expect((await call("/healthz", { env: { TRIAL_PEPPER: "" } })).res.status).toBe(200); // DEV_MODE on
  });
});

describe("R2-6 / R2-7: config sanity", () => {
  it("refuses stream cap + upstream timeout that reach the stale limit", async () => {
    expectError(await call("/healthz", { env: { STREAM_MAX_MS: "540000", UPSTREAM_TIMEOUT_MS: "60000" } }), 503, "unavailable");
    expect((await call("/healthz", { env: { STREAM_MAX_MS: "530000", UPSTREAM_TIMEOUT_MS: "60000" } })).res.status).toBe(200);
  });

  it("refuses DEV_MODE with a non-local PUBLIC_URL", async () => {
    expectError(await call("/healthz", { env: { DEV_MODE: "1", PUBLIC_URL: "https://aicalc.example" } }), 503, "unavailable");
    expect((await call("/healthz", { env: { DEV_MODE: "1", PUBLIC_URL: "http://127.0.0.1:8787" } })).res.status).toBe(200);
  });
});

describe("R2-4 / R2-5: the claim batch", () => {
  it("cap reached between the pre-check and the batch: 429, nothing linked, code kept", async () => {
    const s = await signIn();
    const d = await registerDevice();
    const realDb = env.DB;
    // Simulates another claim taking the last free month just before our batch runs.
    const racyDb = new Proxy(realDb, {
      get(target, prop) {
        if (prop === "batch") {
          return async (stmts: D1PreparedStatement[]) => {
            await target.prepare(`INSERT INTO trials_used (email_hash, used_at) VALUES ('someone-else', ?)`).bind(Math.floor(Date.now() / 1000)).run();
            return target.batch(stmts);
          };
        }
        const v = Reflect.get(target, prop);
        return typeof v === "function" ? v.bind(target) : v;
      },
    });
    const r = await call("/api/devices/claim", {
      body: { code: d.pair_code },
      headers: browserHeaders(s.cookie),
      env: { DB: racyDb, MAX_NEW_TRIALS_PER_DAY: "1" },
    });
    expectError(r, 429, "rate_limited");
    expect(r.res.headers.get("retry-after")).not.toBeNull();
    expect((await dbFirst<any>(`SELECT user_id FROM devices WHERE id = ?`, d.device_id))!.user_id).toBeNull();
    expect((await dbFirst<any>(`SELECT claimed_at FROM pair_codes WHERE code = ?`, d.pair_code.replace("-", "")))!.claimed_at).toBeNull();
    expect((await dbFirst<any>(`SELECT trial_used_at FROM hw_allow WHERE hw_id = ?`, d.hw_id))!.trial_used_at).toBeNull();
    expect((await dbFirst<any>(`SELECT trial_ends_at FROM users WHERE email = ?`, s.email))!.trial_ends_at).toBeNull();
    // Later (cap free again), the same code works and gives the trial.
    const ok = await claimCode(s.cookie, d.pair_code);
    expect(ok.res.status).toBe(201);
    expect((await call("/api/me", { headers: { cookie: s.cookie } })).json.plan).toBe("trial");
  });

  it("race: two registrations of one enrolled board claimed at once -> one free month, both linked", async () => {
    const hw = nextHwId();
    const { secret } = await enrolBoard(hw);
    const d1 = await registerDevice({}, { hwId: hw, secret });
    const d2 = await registerDevice({}, { hwId: hw, secret });
    const a = await signIn();
    const b = await signIn();
    const [ra, rb] = await Promise.all([claimCode(a.cookie, d1.pair_code), claimCode(b.cookie, d2.pair_code)]);
    expect(ra.res.status).toBe(201);
    expect(rb.res.status).toBe(201);
    const plans = [
      (await call("/api/me", { headers: { cookie: a.cookie } })).json.plan,
      (await call("/api/me", { headers: { cookie: b.cookie } })).json.plan,
    ].sort();
    expect(plans).toEqual(["none", "trial"]);
    expect((await dbFirst<{ n: number }>(`SELECT COUNT(*) AS n FROM trials_used`))!.n).toBe(1);
  });

  it("an unenrolled device sharing an enrolled board's hw_id can't use that board's trial", async () => {
    const hw = nextHwId();
    await enrolBoard(hw);
    const fake = await registerDevice({}, { hwId: hw, enrolled: false });
    const s = await signIn();
    expect((await claimCode(s.cookie, fake.pair_code)).res.status).toBe(201);
    expect((await call("/api/me", { headers: { cookie: s.cookie } })).json.plan).toBe("none");
    expect((await dbFirst<any>(`SELECT trial_used_at FROM hw_allow WHERE hw_id = ?`, hw))!.trial_used_at).toBeNull();
  });
});
