import { env } from "cloudflare:workers";
import { createExecutionContext, createScheduledController, waitOnExecutionContext } from "cloudflare:test";
import { describe, expect, it, vi } from "vitest";
import worker from "../src/index";
import {
  ADMIN_TOKEN,
  browserHeaders,
  call,
  claimCode,
  dbFirst,
  dbRun,
  deviceHeaders,
  expectError,
  freshEmail,
  linkedDevice,
  mockAnthropic,
  nowSec,
  registerDevice,
  signIn,
  solve,
  testEnv,
} from "./helpers";

describe("sign-in", () => {
  it("login always answers 204 and never reveals whether the account exists", async () => {
    const known = await signIn();
    const a = await call("/api/auth/login", { body: { email: known.email, age_confirmed: true }, headers: browserHeaders() });
    const b = await call("/api/auth/login", { body: { email: freshEmail(), age_confirmed: true }, headers: browserHeaders() });
    expect(a.res.status).toBe(204);
    expect(b.res.status).toBe(204);
    expect(a.text).toBe("");
    expect(b.text).toBe("");
  });

  it("the user is created on confirm, not on login", async () => {
    const email = freshEmail();
    await call("/api/auth/login", { body: { email, age_confirmed: true }, headers: browserHeaders() });
    expect(await dbFirst(`SELECT * FROM users WHERE email = ?`, email)).toBeNull();
    const o = await call(`/__dev/outbox?email=${encodeURIComponent(email.toUpperCase())}`); // case-insensitive
    expect(o.json.link).toBe(`http://localhost:8787/login/confirm?token=${o.json.token}`);
    expect(o.json.token).toMatch(/^[A-Za-z0-9_-]{43}$/);
    // stored hashed only
    expect(await dbFirst(`SELECT * FROM login_tokens WHERE token_hash = ?`, o.json.token)).toBeNull();
  });

  it("per-e-mail limit: 5 per hour, extra requests are silently dropped (still 204)", async () => {
    const email = freshEmail();
    for (let i = 0; i < 7; i++) {
      const r = await call("/api/auth/login", { body: { email, age_confirmed: true }, headers: browserHeaders() });
      expect(r.res.status).toBe(204);
    }
    const n = await dbFirst<{ n: number }>(`SELECT COUNT(*) AS n FROM dev_outbox WHERE email = ?`, email);
    expect(n!.n).toBe(5);
  });

  it("per-IP limit: 20 per hour, then 429 with Retry-After", async () => {
    const h = { ...browserHeaders(), "x-dev-client-ip": "198.51.100.9" };
    for (let i = 0; i < 20; i++) {
      expect((await call("/api/auth/login", { body: { email: freshEmail(), age_confirmed: true }, headers: h })).res.status).toBe(204);
    }
    const r = await call("/api/auth/login", { body: { email: freshEmail(), age_confirmed: true }, headers: h });
    expectError(r, 429, "rate_limited");
    expect(Number(r.res.headers.get("retry-after"))).toBeGreaterThan(0);
  });

  it("x-dev-client-ip is ignored without DEV_MODE", async () => {
    const off = { DEV_MODE: "", PUBLIC_URL: "https://aicalc.example" };
    for (let i = 0; i < 20; i++) {
      const h = { ...browserHeaders(), origin: "https://aicalc.example", "x-dev-client-ip": `203.0.113.${i}`, "cf-connecting-ip": "203.0.113.200" };
      expect((await call("/api/auth/login", { body: { email: freshEmail(), age_confirmed: true }, headers: h, env: off })).res.status).toBe(204);
    }
    const h = { ...browserHeaders(), origin: "https://aicalc.example", "x-dev-client-ip": "203.0.113.99", "cf-connecting-ip": "203.0.113.200" };
    expectError(await call("/api/auth/login", { body: { email: freshEmail(), age_confirmed: true }, headers: h, env: off }), 429, "rate_limited");
  });

  it("login validates its body", async () => {
    for (const body of [
      { email: "not-an-email", age_confirmed: true },
      { email: freshEmail(), age_confirmed: false },
      { email: freshEmail() },
      { email: freshEmail(), age_confirmed: true, extra: 1 },
    ]) {
      expectError(await call("/api/auth/login", { body, headers: browserHeaders() }), 400, "bad_request");
    }
  });

  it("confirm is single use and sets a proper cookie", async () => {
    const email = freshEmail();
    await call("/api/auth/login", { body: { email, age_confirmed: true }, headers: browserHeaders() });
    const { token } = (await call(`/__dev/outbox?email=${email}`)).json;
    const c1 = await call("/api/auth/confirm", { body: { token }, headers: browserHeaders() });
    expect(c1.res.status).toBe(204);
    const cookie = c1.res.headers.get("set-cookie")!;
    expect(cookie).toMatch(/^sid=[A-Za-z0-9_-]{43}; HttpOnly; SameSite=Lax; Path=\/; Max-Age=2592000$/); // no Secure on http://localhost
    expectError(await call("/api/auth/confirm", { body: { token }, headers: browserHeaders() }), 400, "invalid_token");
  });

  it("production: e-mail via Resend with an https link, Secure cookie", async () => {
    const over = { PUBLIC_URL: "https://aicalc.example", DEV_MODE: "", RESEND_API_KEY: "re_test", MAIL_FROM: "AI Calc <login@aicalc.example>" };
    const email = freshEmail();
    const h = { ...browserHeaders(), origin: "https://aicalc.example" };
    const mails: any[] = [];
    vi.spyOn(globalThis, "fetch").mockImplementation(async (input: RequestInfo | URL, init?: RequestInit) => {
      const req = new Request(input as RequestInfo, init);
      expect(req.url).toBe("https://api.resend.com/emails");
      expect(req.headers.get("authorization")).toBe("Bearer re_test");
      mails.push(await req.json());
      return Response.json({ id: "x" });
    });
    expect((await call("/api/auth/login", { body: { email, age_confirmed: true }, headers: h, env: over })).res.status).toBe(204);
    expect(mails).toHaveLength(1);
    expect(mails[0].to).toEqual([email]);
    expect(mails[0].from).toBe("AI Calc <login@aicalc.example>");
    const m = /https:\/\/aicalc\.example\/login\/confirm\?token=([A-Za-z0-9_-]{43})/.exec(mails[0].text);
    expect(m).not.toBeNull();
    const token = m![1]!;
    expect(await dbFirst(`SELECT * FROM dev_outbox`)).toBeNull();
    const c = await call("/api/auth/confirm", { body: { token }, headers: h, env: over });
    expect(c.res.headers.get("set-cookie")).toMatch(/; HttpOnly; Secure; SameSite=Lax; Path=\/; Max-Age=2592000$/);
  });

  it("expired and unknown tokens are invalid_token", async () => {
    const email = freshEmail();
    await call("/api/auth/login", { body: { email, age_confirmed: true }, headers: browserHeaders() });
    const { token } = (await call(`/__dev/outbox?email=${email}`)).json;
    await dbRun(`UPDATE login_tokens SET expires_at = ?`, nowSec() - 1);
    expectError(await call("/api/auth/confirm", { body: { token }, headers: browserHeaders() }), 400, "invalid_token");
    expectError(
      await call("/api/auth/confirm", { body: { token: "x".repeat(43) }, headers: browserHeaders() }),
      400,
      "invalid_token",
    );
  });

  it("logout clears the session and is idempotent", async () => {
    const { cookie } = await signIn();
    expect((await call("/api/me", { headers: { cookie } })).res.status).toBe(200);
    const r = await call("/api/auth/logout", { body: {}, headers: browserHeaders(cookie) });
    expect(r.res.status).toBe(204);
    expect(r.res.headers.get("set-cookie")).toMatch(/^sid=; .*Max-Age=0$/);
    expectError(await call("/api/me", { headers: { cookie } }), 401, "unauthorized");
    expect((await call("/api/auth/logout", { method: "POST", headers: browserHeaders() })).res.status).toBe(204);
  });
});

describe("browser CSRF guard", () => {
  it("state-changing calls need Origin == site origin and JSON content type", async () => {
    const { cookie } = await signIn();
    const good = browserHeaders(cookie);
    const variants: Record<string, string>[] = [
      { ...good, origin: "https://evil.example" },
      Object.fromEntries(Object.entries(good).filter(([k]) => k !== "origin")),
      { ...good, "content-type": "text/plain" },
      { ...good, "content-type": "application/x-www-form-urlencoded" },
    ];
    for (const headers of variants) {
      expectError(await call("/api/auth/login", { body: JSON.stringify({ email: freshEmail(), age_confirmed: true }), headers }), 403, "forbidden");
      expectError(await call("/api/devices/claim", { body: JSON.stringify({ code: "ABCD-EFGH" }), headers }), 403, "forbidden");
      expectError(await call("/api/me", { method: "DELETE", body: "{}", headers }), 403, "forbidden");
      expectError(await call("/api/auth/logout", { method: "POST", body: "{}", headers }), 403, "forbidden");
    }
  });

  it("account endpoints need a session", async () => {
    expectError(await call("/api/me"), 401, "unauthorized");
    expectError(await call("/api/me", { headers: { cookie: "sid=" + "a".repeat(43) } }), 401, "unauthorized");
    expectError(await claimCode("sid=" + "a".repeat(43), "ABCD-EFGH"), 401, "unauthorized");
  });
});

describe("devices", () => {
  it("GET /api/me lists the account, plan, usage and devices", async () => {
    const { cookie, email, device } = await linkedDevice();
    const r = await call("/api/me", { headers: { cookie } });
    expect(r.res.status).toBe(200);
    expect(r.json).toMatchObject({
      email,
      plan: "trial",
      usage: { solves_this_month: 0, solves_limit: 300, spend_limit_reached: false },
    });
    expect(r.json.devices).toHaveLength(1);
    expect(r.json.devices[0]).toMatchObject({ id: device.device_id, label: "Calculator", revoked: false, hw_id: expect.stringMatching(/^calc-/) });
  });

  it("claim race: two accounts, one code, exactly one wins", async () => {
    const d = await registerDevice();
    const a = await signIn();
    const b = await signIn();
    const [ra, rb] = await Promise.all([claimCode(a.cookie, d.pair_code), claimCode(b.cookie, d.pair_code)]);
    const statuses = [ra.res.status, rb.res.status].sort();
    expect(statuses).toEqual([201, 400]);
    const loser = ra.res.status === 400 ? ra : rb;
    expectError(loser, 400, "invalid_code");
    const owners = await dbFirst<{ n: number }>(`SELECT COUNT(*) AS n FROM devices WHERE id = ? AND user_id IS NOT NULL`, d.device_id);
    expect(owners!.n).toBe(1);
  });

  it("a code can't be used twice, and expired codes fail with the same message", async () => {
    const d = await registerDevice();
    const a = await signIn();
    expect((await claimCode(a.cookie, d.pair_code)).res.status).toBe(201);
    const e1 = expectError(await claimCode(a.cookie, d.pair_code), 400, "invalid_code");

    const d2 = await registerDevice();
    await dbRun(`UPDATE pair_codes SET expires_at = ? WHERE device_id = ?`, nowSec() - 1, d2.device_id);
    const e2 = expectError(await claimCode(a.cookie, d2.pair_code), 400, "invalid_code");
    const e3 = expectError(await claimCode(a.cookie, "ZZZZ-ZZZZ"), 400, "invalid_code");
    expect(e1.message).toBe(e2.message);
    expect(e2.message).toBe(e3.message);
    expectError(await claimCode(a.cookie, "ABCD-EFG0"), 400, "invalid_code");
    expectError(await claimCode(a.cookie, "ABC"), 400, "bad_request");
  });

  it("the trial starts on the first claim only", async () => {
    const s = await signIn();
    const d1 = await registerDevice();
    await claimCode(s.cookie, d1.pair_code);
    const t1 = (await dbFirst<{ trial_ends_at: number }>(`SELECT trial_ends_at FROM users WHERE email = ?`, s.email))!.trial_ends_at;
    expect(t1).toBeGreaterThan(nowSec() + 29 * 86400);
    await dbRun(`UPDATE users SET trial_ends_at = ? WHERE email = ?`, 1000, s.email);
    const d2 = await registerDevice();
    await claimCode(s.cookie, d2.pair_code);
    const t2 = (await dbFirst<{ trial_ends_at: number }>(`SELECT trial_ends_at FROM users WHERE email = ?`, s.email))!.trial_ends_at;
    expect(t2).toBe(1000);
  });

  it("max 3 active devices; a refused claim keeps the code usable", async () => {
    const s = await signIn();
    const ids: string[] = [];
    for (let i = 0; i < 3; i++) {
      const d = await registerDevice();
      const c = await claimCode(s.cookie, d.pair_code);
      expect(c.res.status).toBe(201);
      ids.push(d.device_id);
    }
    const d4 = await registerDevice();
    expectError(await claimCode(s.cookie, d4.pair_code), 409, "device_limit");
    // bad label: also doesn't consume the code
    expectError(await claimCode(s.cookie, d4.pair_code, "<script>"), 400, "bad_request");
    expect((await call(`/api/devices/${ids[0]}/revoke`, { body: {}, headers: browserHeaders(s.cookie) })).res.status).toBe(204);
    expect((await claimCode(s.cookie, d4.pair_code)).res.status).toBe(201);
  });

  it("claim is rate limited to 10 attempts per 10 minutes", async () => {
    const s = await signIn();
    for (let i = 0; i < 10; i++) expectError(await claimCode(s.cookie, "ZZZZ-ZZZZ"), 400, "invalid_code");
    const r = await claimCode(s.cookie, "ZZZZ-ZZZZ");
    expectError(r, 429, "rate_limited");
    expect(r.res.headers.get("retry-after")).not.toBeNull();
  });

  it("rename", async () => {
    const { cookie, device } = await linkedDevice();
    const r = await call(`/api/devices/${device.device_id}`, { method: "PATCH", body: { label: "Kitchen" }, headers: browserHeaders(cookie) });
    expect(r.res.status).toBe(200);
    expect(r.json).toMatchObject({ id: device.device_id, label: "Kitchen" });
    expectError(
      await call(`/api/devices/${device.device_id}`, { method: "PATCH", body: { label: "" }, headers: browserHeaders(cookie) }),
      400,
      "bad_request",
    );
    expectError(
      await call(`/api/devices/${device.device_id}`, { method: "PATCH", body: { label: "x".repeat(41) }, headers: browserHeaders(cookie) }),
      400,
      "bad_request",
    );
    const other = await signIn();
    expectError(
      await call(`/api/devices/${device.device_id}`, { method: "PATCH", body: { label: "Mine" }, headers: browserHeaders(other.cookie) }),
      404,
      "not_found",
    );
  });

  it("revoke: the calculator gets device_revoked; other accounts get 404", async () => {
    const { cookie, device } = await linkedDevice();
    const other = await signIn();
    expectError(
      await call(`/api/devices/${device.device_id}/revoke`, { body: {}, headers: browserHeaders(other.cookie) }),
      404,
      "not_found",
    );
    expectError(await call(`/api/devices/dev_nothex/revoke`, { body: {}, headers: browserHeaders(cookie) }), 404, "not_found");
    // empty body with JSON content type is accepted
    const r = await call(`/api/devices/${device.device_id}/revoke`, { method: "POST", headers: browserHeaders(cookie) });
    expect(r.res.status).toBe(204);
    const { spy } = mockAnthropic();
    expectError(await solve(device), 401, "device_revoked");
    expectError(await call("/v1/firmware", { headers: deviceHeaders(device) }), 401, "device_revoked");
    expect(spy).not.toHaveBeenCalled();
    const me = await call("/api/me", { headers: { cookie } });
    expect(me.json.devices[0].revoked).toBe(true);
    // re-registering gives a new, unlinked token
    const fresh = await registerDevice();
    const st = await call("/v1/device/status", { headers: deviceHeaders(fresh) });
    expect(st.json.linked).toBe(false);
  });
});

describe("account deletion", () => {
  it("needs the matching e-mail, then revokes devices, ends sessions and erases the e-mail", async () => {
    const { cookie, email, device } = await linkedDevice();
    expectError(await call("/api/me", { method: "DELETE", body: { confirm_email: "x@example.com" }, headers: browserHeaders(cookie) }), 400, "bad_request");
    expectError(await call("/api/me", { method: "DELETE", body: {}, headers: browserHeaders(cookie) }), 400, "bad_request");
    const r = await call("/api/me", { method: "DELETE", body: { confirm_email: email.toUpperCase() }, headers: browserHeaders(cookie) });
    expect(r.res.status).toBe(204);
    expect(r.res.headers.get("set-cookie")).toMatch(/Max-Age=0/);
    expectError(await call("/api/me", { headers: { cookie } }), 401, "unauthorized");
    expectError(await call("/v1/device/status", { headers: deviceHeaders(device) }), 401, "device_revoked");
    expect(await dbFirst(`SELECT * FROM users WHERE email = ?`, email)).toBeNull();
    // The same address can sign up again as a new account.
    const again = await signIn(email);
    const me = await call("/api/me", { headers: { cookie: again.cookie } });
    expect(me.json).toMatchObject({ email, plan: "none", devices: [] });
  });
});

describe("firmware", () => {
  function image(size: number, first = 0xe9): Uint8Array {
    const b = new Uint8Array(size);
    b[0] = first;
    for (let i = 1; i < size; i++) b[i] = i & 255;
    return b;
  }
  const admin = { "x-admin-token": ADMIN_TOKEN, "content-type": "application/octet-stream" };

  it("publish, check and download (works for unlinked devices too)", async () => {
    const d = await registerDevice();
    let chk = await call("/v1/firmware", { headers: deviceHeaders(d) });
    expect(chk.json).toEqual({ update: false });
    expectError(await call("/v1/firmware/image", { headers: deviceHeaders(d) }), 404, "not_found");

    const img = image(150_000);
    const p = await call("/v1/admin/firmware?version=stage14-2026.10.08", { body: img, headers: admin });
    expect(p.res.status, p.text).toBe(201);
    const digest = [...new Uint8Array(await crypto.subtle.digest("SHA-256", img))].map((b) => b.toString(16).padStart(2, "0")).join("");
    expect(p.json).toEqual({ version: "stage14-2026.10.08", size: 150_000, sha256: digest, family: null, rollback: false });

    chk = await call("/v1/firmware", { headers: deviceHeaders(d) }); // x-firmware: stage13
    expect(chk.json).toEqual({
      update: true,
      version: "stage14-2026.10.08",
      size: 150_000,
      sha256: digest,
      path: "/v1/firmware/image",
      family: null,
      rollback: false,
    });
    chk = await call("/v1/firmware", { headers: { ...deviceHeaders(d), "x-firmware": "stage14-2026.10.08" } });
    expect(chk.json.update).toBe(false);

    const req = new Request("http://localhost:8787/v1/firmware/image", { headers: deviceHeaders(d) });
    const ctx = createExecutionContext();
    const res = await worker.fetch(req, testEnv(), ctx);
    expect(res.status).toBe(200);
    expect(res.headers.get("content-type")).toBe("application/octet-stream");
    expect(res.headers.get("x-firmware-sha256")).toBe(digest);
    const got = new Uint8Array(await res.arrayBuffer());
    await waitOnExecutionContext(ctx);
    expect(got.byteLength).toBe(150_000);
    expect(got[0]).toBe(0xe9);
    expect(got[12345]).toBe(12345 & 255);
  });

  it("needs a device token", async () => {
    expectError(await call("/v1/firmware"), 401, "unauthorized");
    expectError(await call("/v1/firmware/image"), 401, "unauthorized");
  });

  it("admin publish: token, version and image checks", async () => {
    expectError(await call("/v1/admin/firmware?version=v1", { body: image(150_000), headers: { ...admin, "x-admin-token": "wrong" } }), 401, "unauthorized");
    expectError(await call("/v1/admin/firmware?version=v1", { body: image(150_000), headers: { "content-type": "application/octet-stream" } }), 401, "unauthorized");
    expectError(await call("/v1/admin/firmware?version=bad version", { body: image(150_000), headers: admin }), 400, "bad_request");
    expectError(await call("/v1/admin/firmware", { body: image(150_000), headers: admin }), 400, "bad_request");
    expectError(await call("/v1/admin/firmware?version=v1", { body: image(99_999), headers: admin }), 400, "bad_request");
    expectError(await call("/v1/admin/firmware?version=v1", { body: image(0x1e0001), headers: admin }), 400, "bad_request");
    expectError(await call("/v1/admin/firmware?version=v1", { body: image(150_000, 0x7f), headers: admin }), 400, "bad_request");
    expect((await call("/v1/admin/firmware?version=v1", { body: image(0x1e0000), headers: admin })).res.status).toBe(201);
  });

  it("admin endpoint is off (404) when ADMIN_TOKEN is unset", async () => {
    expectError(
      await call("/v1/admin/firmware?version=v1", { body: image(150_000), headers: admin, env: { ADMIN_TOKEN: "" } }),
      404,
      "not_found",
    );
  });
});

describe("dev endpoints", () => {
  it("are refused without DEV_MODE or off localhost", async () => {
    const prod = { DEV_MODE: "", PUBLIC_URL: "https://aicalc.example" };
    expectError(await call("/__dev/outbox?email=a@example.com", { env: prod }), 404, "not_found");
    expectError(await call("/__dev/user", { body: { email: "a@example.com" }, env: prod }), 404, "not_found");
    expectError(await call("/__dev/outbox?email=a@example.com", { base: "https://aicalc.example" }), 404, "not_found");
    expectError(await call("/__dev/user", { body: { email: "a@example.com" }, base: "https://api.aicalc.example" }), 404, "not_found");
  });

  it("no outbox rows are written without DEV_MODE", async () => {
    const email = freshEmail();
    const r = await call("/api/auth/login", {
      body: { email, age_confirmed: true },
      headers: { ...browserHeaders(), origin: "https://aicalc.example" },
      env: { DEV_MODE: "", PUBLIC_URL: "https://aicalc.example" },
    });
    expect(r.res.status).toBe(204);
    expect(await dbFirst(`SELECT * FROM dev_outbox`)).toBeNull();
  });

  it("/__dev/user creates and updates users", async () => {
    const email = freshEmail();
    const r = await call("/__dev/user", { body: { email, trial_ends_at: 12345, plan_override: "active" } });
    expect(r.json).toMatchObject({ email, trial_ends_at: 12345, plan_override: "active" });
    expectError(await call("/__dev/user", { body: { email, plan_override: "gold" } }), 400, "bad_request");
  });
});

describe("scheduled clean-up", () => {
  it("purges unlinked devices unused for 30 days but keeps linked ones", async () => {
    const old = await registerDevice();
    const { device: linked } = await linkedDevice();
    const longAgo = nowSec() - 31 * 86400;
    await dbRun(`UPDATE devices SET last_seen_at = ?, created_at = ?`, longAgo, longAgo);
    const ctx = createExecutionContext();
    await worker.scheduled!(createScheduledController(), env as any, ctx);
    await waitOnExecutionContext(ctx);
    expect(await dbFirst(`SELECT id FROM devices WHERE id = ?`, old.device_id)).toBeNull();
    expect(await dbFirst(`SELECT id FROM devices WHERE id = ?`, linked.device_id)).not.toBeNull();
    expect(await dbFirst(`SELECT * FROM pair_codes WHERE device_id = ?`, old.device_id)).toBeNull();
  });
});
