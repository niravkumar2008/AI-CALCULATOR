// Tests for the stage-1 security review fixes (PLAN section 4c).
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
  enrolBoard,
  expectError,
  freshEmail,
  mockAnthropic,
  newHwSecret,
  nextHwId,
  nowSec,
  registerDevice,
  signIn,
  solve,
  solveBody,
  sseEvent,
  sseResponse,
} from "./helpers";

const NO_TRIAL = "No free month for this calculator. Subscribe at localhost:8787/account to use AI.";

async function planOf(cookie: string) {
  return (await call("/api/me", { headers: { cookie } })).json.plan;
}

describe("S1: board enrolment and once-only free months", () => {
  it("POST /v1/admin/hardware enrols, validates, and is admin-only", async () => {
    const hw = nextHwId();
    const r = await call("/v1/admin/hardware", { body: { hw_id: hw, hw_secret: newHwSecret() }, headers: { "x-admin-token": ADMIN_TOKEN } });
    expect(r.res.status).toBe(201);
    expect(r.json).toEqual({ hw_id: hw, trial_used: false });
    const stored = await dbFirst<{ secret_hash: string }>(`SELECT secret_hash FROM hw_allow WHERE hw_id = ?`, hw);
    expect(stored!.secret_hash).toMatch(/^[0-9a-f]{64}$/);

    const h = { "x-admin-token": ADMIN_TOKEN };
    expectError(await call("/v1/admin/hardware", { body: { hw_id: "bad", hw_secret: newHwSecret() }, headers: h }), 400, "bad_request");
    expectError(await call("/v1/admin/hardware", { body: { hw_id: hw, hw_secret: "hs_short" }, headers: h }), 400, "bad_request");
    expectError(await call("/v1/admin/hardware", { body: { hw_id: hw, hw_secret: newHwSecret(), x: 1 }, headers: h }), 400, "bad_request");
    expectError(await call("/v1/admin/hardware", { body: { hw_id: hw, hw_secret: newHwSecret() }, headers: { "x-admin-token": "nope" } }), 401, "unauthorized");
  });

  it("register: only the matching secret enrols; a wrong secret looks the same to the caller", async () => {
    const hw = nextHwId();
    const { secret } = await enrolBoard(hw);
    const good = await registerDevice({}, { hwId: hw, secret });
    const wrong = await registerDevice({}, { hwId: hw, secret: newHwSecret() });
    const none = await registerDevice({}, { hwId: hw, enrolled: false });
    for (const d of [good, wrong, none]) expect(Object.keys(d).sort()).toEqual(["device_id", "expires_at", "hw_id", "hw_secret", "link_url", "pair_code", "token"]);
    const flag = async (id: string) => (await dbFirst<{ enrolled: number }>(`SELECT enrolled FROM devices WHERE id = ?`, id))!.enrolled;
    expect(await flag(good.device_id)).toBe(1);
    expect(await flag(wrong.device_id)).toBe(0);
    expect(await flag(none.device_id)).toBe(0);
    expectError(await call("/v1/device/register", { body: { hw_id: hw, hw_secret: "nope" } }), 400, "bad_request");
  });

  it("an unenrolled board links with plan none and the no-free-month message", async () => {
    const d = await registerDevice({}, { enrolled: false });
    const s = await signIn();
    expect((await claimCode(s.cookie, d.pair_code)).res.status).toBe(201);
    expect(await planOf(s.cookie)).toBe("none");
    const st = await call("/v1/device/status", { headers: deviceHeaders(d) });
    expect(st.json).toMatchObject({ linked: true, plan: "none", can_solve: false, reason: NO_TRIAL });
    const { spy } = mockAnthropic();
    const e = expectError(await solve(d), 402, "subscription_inactive");
    expect(e.message).toBe(NO_TRIAL);
    expect(spy).not.toHaveBeenCalled();
  });

  it("one free month per board, ever (revoke + re-register doesn't give another)", async () => {
    const hw = nextHwId();
    const { secret } = await enrolBoard(hw);
    const d1 = await registerDevice({}, { hwId: hw, secret });
    const a = await signIn();
    await claimCode(a.cookie, d1.pair_code);
    expect(await planOf(a.cookie)).toBe("trial");
    await call(`/api/devices/${d1.device_id}/revoke`, { body: {}, headers: browserHeaders(a.cookie) });

    const d2 = await registerDevice({}, { hwId: hw, secret });
    const b = await signIn();
    expect((await claimCode(b.cookie, d2.pair_code)).res.status).toBe(201);
    expect(await planOf(b.cookie)).toBe("none");
    // Re-enrolling the board never resets it.
    const again = await enrolBoard(hw);
    expect(again.json).toEqual({ hw_id: hw, trial_used: true });
    const used = await dbFirst<{ trial_used_at: number }>(`SELECT trial_used_at FROM hw_allow WHERE hw_id = ?`, hw);
    expect(used!.trial_used_at).not.toBeNull();
  });

  it("one free month per e-mail, ever (survives account deletion)", async () => {
    const a = await signIn();
    const d1 = await registerDevice();
    await claimCode(a.cookie, d1.pair_code);
    expect(await planOf(a.cookie)).toBe("trial");
    await call("/api/me", { method: "DELETE", body: { confirm_email: a.email }, headers: browserHeaders(a.cookie) });
    expect(await dbFirst(`SELECT * FROM trials_used`)).not.toBeNull();

    const again = await signIn(a.email);
    const d2 = await registerDevice(); // a fresh enrolled board
    expect((await claimCode(again.cookie, d2.pair_code)).res.status).toBe(201);
    expect(await planOf(again.cookie)).toBe("none");
    // ...but that board's trial is still unused, for someone else.
    const used = await dbFirst<{ trial_used_at: number | null }>(`SELECT trial_used_at FROM hw_allow WHERE hw_id = ?`, d2.hw_id);
    expect(used!.trial_used_at).toBeNull();
  });

  it("daily new-trial cap: 429 with Retry-After to UTC midnight, code not consumed", async () => {
    const cap = { MAX_NEW_TRIALS_PER_DAY: "1" };
    const a = await signIn();
    const d1 = await registerDevice();
    expect((await call("/api/devices/claim", { body: { code: d1.pair_code }, headers: browserHeaders(a.cookie), env: cap })).res.status).toBe(201);

    const b = await signIn();
    const d2 = await registerDevice();
    const r = await call("/api/devices/claim", { body: { code: d2.pair_code }, headers: browserHeaders(b.cookie), env: cap });
    expectError(r, 429, "rate_limited");
    const midnight = (Math.floor(nowSec() / 86400) + 1) * 86400;
    expect(Number(r.res.headers.get("retry-after"))).toBe(midnight - nowSec());
    expect(await dbFirst(`SELECT claimed_at FROM pair_codes WHERE code = ? AND claimed_at IS NULL`, d2.pair_code.replace("-", ""))).not.toBeNull();

    // Ineligible anyway (unenrolled): the cap doesn't matter, it links with plan none.
    const c = await signIn();
    const d3 = await registerDevice({}, { enrolled: false });
    expect((await call("/api/devices/claim", { body: { code: d3.pair_code }, headers: browserHeaders(c.cookie), env: cap })).res.status).toBe(201);

    // Tomorrow (or with a higher cap) the same code still works and gives the trial.
    expect((await call("/api/devices/claim", { body: { code: d2.pair_code }, headers: browserHeaders(b.cookie), env: { MAX_NEW_TRIALS_PER_DAY: "2" } })).res.status).toBe(201);
    expect(await planOf(b.cookie)).toBe("trial");
  });

  it("an enrolled board can't move to another board's hw_id via x-device-id", async () => {
    const d = await registerDevice();
    await call("/v1/device/status", { headers: { ...deviceHeaders(d), "x-device-id": "calc-ffffffffffff" } });
    const row = await dbFirst<{ hw_id: string }>(`SELECT hw_id FROM devices WHERE id = ?`, d.device_id);
    expect(row!.hw_id).toBe(d.hw_id);
    // An unenrolled device still records it (clone spotting in the portal).
    const u = await registerDevice({}, { enrolled: false });
    await call("/v1/device/status", { headers: { ...deviceHeaders(u), "x-device-id": "calc-ffffffffffff" } });
    expect((await dbFirst<{ hw_id: string }>(`SELECT hw_id FROM devices WHERE id = ?`, u.device_id))!.hw_id).toBe("calc-ffffffffffff");
  });
});

describe("S4: rate-limit counters stop writing once over", () => {
  it("the counter stays at the limit", async () => {
    const h = { "x-dev-client-ip": "192.0.2.200" };
    for (let i = 0; i < 9; i++) await call("/v1/device/register", { body: { hw_id: "calc-0123456789ab" }, headers: h });
    const row = await dbFirst<{ count: number }>(`SELECT count FROM rate_limits WHERE key = ?`, "register:ip:192.0.2.200");
    expect(row!.count).toBe(5);
  });
});

describe("S5: login mail cap and Turnstile", () => {
  it("over the daily mail cap: still 204, but no token and no mail", async () => {
    const cap = { MAIL_DAILY_CAP: "2" };
    const emails = [freshEmail(), freshEmail(), freshEmail()];
    for (const email of emails) {
      expect((await call("/api/auth/login", { body: { email, age_confirmed: true }, headers: browserHeaders(), env: cap })).res.status).toBe(204);
    }
    const n = await dbFirst<{ n: number }>(`SELECT COUNT(*) AS n FROM login_tokens`);
    expect(n!.n).toBe(2);
    expect(await dbFirst(`SELECT * FROM dev_outbox WHERE email = ?`, emails[2])).toBeNull();
  });

  function mockTurnstile(success: boolean) {
    const seen: FormData[] = [];
    const spy = vi.spyOn(globalThis, "fetch").mockImplementation(async (input: RequestInfo | URL, init?: RequestInit) => {
      const req = new Request(input as RequestInfo, init);
      if (req.url !== "https://challenges.cloudflare.com/turnstile/v0/siteverify") throw new Error(`unexpected fetch ${req.url}`);
      seen.push(await req.formData());
      return Response.json({ success });
    });
    return { spy, seen };
  }

  it("with TURNSTILE_SECRET: token required and verified", async () => {
    const ts = { TURNSTILE_SECRET: "ts-secret" };
    const email = freshEmail();
    let m = mockTurnstile(true);
    expectError(await call("/api/auth/login", { body: { email, age_confirmed: true }, headers: browserHeaders(), env: ts }), 403, "forbidden");
    expect(m.spy).not.toHaveBeenCalled();
    expect((await call("/api/auth/login", { body: { email, age_confirmed: true, turnstile_token: "tok" }, headers: browserHeaders(), env: ts })).res.status).toBe(204);
    expect(m.seen[0]!.get("secret")).toBe("ts-secret");
    expect(m.seen[0]!.get("response")).toBe("tok");
    vi.restoreAllMocks();
    m = mockTurnstile(false);
    expectError(await call("/api/auth/login", { body: { email, age_confirmed: true, turnstile_token: "bad" }, headers: browserHeaders(), env: ts }), 403, "forbidden");
  });

  it("without TURNSTILE_SECRET the extra field is refused (additionalProperties false)", async () => {
    expectError(
      await call("/api/auth/login", { body: { email: freshEmail(), age_confirmed: true, turnstile_token: "x" }, headers: browserHeaders() }),
      400,
      "bad_request",
    );
  });
});

describe("S6: solve checks run before the body is read", () => {
  it("an unlinked device sending a huge or broken body gets device_not_linked", async () => {
    const d = await registerDevice();
    const { spy } = mockAnthropic();
    expectError(await call("/v1/solve", { body: "x".repeat(9 * 1024 * 1024), headers: { ...deviceHeaders(d), "content-type": "application/json" } }), 402, "device_not_linked");
    expectError(await call("/v1/solve", { body: "not json", headers: { ...deviceHeaders(d), "content-type": "application/json" } }), 402, "device_not_linked");
    expect(spy).not.toHaveBeenCalled();
  });
});

describe("S8: https PUBLIC_URL unless DEV_MODE", () => {
  it("http PUBLIC_URL without DEV_MODE fails closed", async () => {
    expectError(await call("/healthz", { env: { DEV_MODE: "", PUBLIC_URL: "http://aicalc.example" } }), 503, "unavailable");
    expect((await call("/healthz", { env: { DEV_MODE: "", PUBLIC_URL: "https://aicalc.example" } })).res.status).toBe(200);
    expect((await call("/healthz", { env: { DEV_MODE: "1", PUBLIC_URL: "http://localhost:8787" } })).res.status).toBe(200);
  });
});

describe("S10: admin needs a token of 32+ characters", () => {
  it("a short ADMIN_TOKEN keeps both admin endpoints off", async () => {
    const short = "a".repeat(31);
    const over = { ADMIN_TOKEN: short };
    expectError(await call("/v1/admin/hardware", { body: { hw_id: nextHwId(), hw_secret: newHwSecret() }, headers: { "x-admin-token": short }, env: over }), 404, "not_found");
    expectError(await call("/v1/admin/firmware?version=v1", { body: new Uint8Array(200_000).fill(0xe9), headers: { "x-admin-token": short }, env: over }), 404, "not_found");
  });
});

describe("S12: stream time cap and stale reservations", () => {
  it("a stream longer than STREAM_MAX_MS is cut with an error event and settled at worst case", async () => {
    const d = await registerDevice();
    const s = await signIn();
    await claimCode(s.cookie, d.pair_code);
    const start = sseEvent("message_start", { message: { usage: { input_tokens: 100, output_tokens: 1 } } });
    let sent = false;
    mockAnthropic(() =>
      sseResponse(
        new ReadableStream<Uint8Array>({
          pull(c) {
            if (!sent) {
              sent = true;
              c.enqueue(new TextEncoder().encode(start));
            }
            return new Promise(() => {}); // then silence (still connected)
          },
        }),
      ),
    );
    vi.mocked(Date.now).mockRestore?.(); // the real clock is needed for the deadline
    const r = await solve(d, solveBody(), { STREAM_MAX_MS: "200" });
    expect(r.res.status).toBe(200);
    expect(r.text.startsWith(start)).toBe(true);
    expect(r.text).toMatch(/event: error\ndata: \{"type":"error","error":\{"type":"upstream_error"/);
    const u = await dbFirst<any>(`SELECT state, cost_micro, reserved_micro FROM usage WHERE device_id = ?`, d.device_id);
    expect(u.state).toBe("settled");
    expect(u.cost_micro).toBe(u.reserved_micro);
  });

  it("the scheduled job settles reservations older than 10 minutes at worst case", async () => {
    const t = nowSec();
    const day = new Date(t * 1000).toISOString().slice(0, 10);
    await dbRun(
      `INSERT INTO usage (id, device_id, user_id, started_at, state, reserved_micro, day, month) VALUES
         ('use_old', 'dev_a', 'usr_a', ?1, 'reserved', 5000, ?3, ?4),
         ('use_new', 'dev_b', 'usr_b', ?2, 'reserved', 7000, ?3, ?4)`,
      t - 601,
      t - 10,
      day,
      day.slice(0, 7),
    );
    await dbRun(`INSERT INTO spend_daily (day, reserved_micro, settled_micro) VALUES (?, 12000, 100)`, day);
    const ctx = createExecutionContext();
    await worker.scheduled!(createScheduledController(), env as any, ctx);
    await waitOnExecutionContext(ctx);
    expect(await dbFirst(`SELECT state, cost_micro FROM usage WHERE id = 'use_old'`)).toEqual({ state: "settled", cost_micro: 5000 });
    expect(await dbFirst(`SELECT state FROM usage WHERE id = 'use_new'`)).toEqual({ state: "reserved" });
    expect(await dbFirst(`SELECT reserved_micro, settled_micro FROM spend_daily WHERE day = ?`, day)).toEqual({
      reserved_micro: 7000,
      settled_micro: 5100,
    });
  });
});
