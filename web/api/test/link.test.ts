// QR linking, way 2 (docs/LINK_QR_CONTRACT.md): link tokens, /v1/pair/redeem, Approve / No,
// link_state on /v1/device/status, the 1-device slot and the "calculator linked" notice.

import { env } from "cloudflare:workers";
import { describe, expect, it } from "vitest";
import { cleanup } from "../src/index";
import {
  browserHeaders,
  call,
  claimCode,
  dbFirst,
  dbRun,
  deviceHeaders,
  expectError,
  linkedDevice,
  nowSec,
  registerDevice,
  signIn,
  type Device,
} from "./helpers";

const MSG_INVALID = "This code isn't valid. Get a new code on your phone.";
const MSG_EXPIRED = "This code has expired. Get a new code on your phone.";
const MSG_USED = "This code was already used. Get a new code on your phone.";
const MSG_OTHER = "This calculator is linked to another account. Remove it there first.";

async function newToken(cookie: string) {
  const r = await call("/api/link-tokens", { body: {}, headers: browserHeaders(cookie) });
  expect(r.res.status, r.text).toBe(201);
  return r.json as { id: string; token: string; qr_url: string; expires_at: number };
}

function redeem(d: Device, token: unknown, headers: Record<string, string> = {}) {
  return call("/v1/pair/redeem", { body: { link_token: token }, headers: { ...deviceHeaders(d), ...headers } });
}

function poll(cookie: string, id: string) {
  return call(`/api/link-tokens/${id}`, { headers: { cookie } });
}

function approve(cookie: string, id: string, yes: unknown) {
  return call(`/api/link-tokens/${id}/approve`, { body: { approve: yes }, headers: browserHeaders(cookie) });
}

async function linkState(d: Device) {
  const r = await call("/v1/device/status", { headers: deviceHeaders(d) });
  expect(r.res.status, r.text).toBe(200);
  return r.json.link_state as string;
}

const suffixOf = (d: { hw_id: string }) => d.hw_id.slice(-4).toUpperCase();

describe("POST /api/link-tokens", () => {
  it("201 with id, token, qr_url and a 10-minute expiry; only the hash is stored", async () => {
    const s = await signIn();
    const t = await newToken(s.cookie);
    expect(t.id).toMatch(/^lt_[0-9a-f]{16}$/);
    expect(t.token).toMatch(/^L1\.[A-Za-z0-9_-]{22}$/);
    expect(t.qr_url).toBe(`http://localhost:8787/q/${t.token}`);
    expect(t.expires_at).toBe(nowSec() + 600);
    const row = await dbFirst<any>(`SELECT * FROM link_tokens WHERE id = ?`, t.id);
    expect(row.token_hash).toMatch(/^[0-9a-f]{64}$/);
    expect(JSON.stringify(row)).not.toContain(t.token.slice(3));
    expect(row.ip).toMatch(/^10\.0\./);
  });

  it("a new token cancels the user's earlier unused ones", async () => {
    const s = await signIn();
    const t1 = await newToken(s.cookie);
    const t2 = await newToken(s.cookie);
    expect((await poll(s.cookie, t1.id)).json.state).toBe("expired");
    expect((await poll(s.cookie, t2.id)).json.state).toBe("waiting");
    const d = await registerDevice();
    expect(expectError(await redeem(d, t1.token), 410, "link_token_expired").message).toBe(MSG_EXPIRED);
    expect((await redeem(d, t2.token)).json.state).toBe("pending_approval");
  });

  it("needs a session, the CSRF headers and an empty body", async () => {
    expectError(await call("/api/link-tokens", { body: {}, headers: browserHeaders() }), 401, "unauthorized");
    const s = await signIn();
    expectError(await call("/api/link-tokens", { body: {}, headers: { ...browserHeaders(s.cookie), origin: "https://evil.example" } }), 403, "forbidden");
    expectError(await call("/api/link-tokens", { body: { x: 1 }, headers: browserHeaders(s.cookie) }), 400, "bad_request");
    expect((await call("/api/link-tokens", { method: "POST", headers: browserHeaders(s.cookie) })).res.status).toBe(201);
    expectError(await call("/api/link-tokens", { headers: { cookie: s.cookie } }), 405, "bad_request");
  });

  it("max 10 per user per hour, then 429", async () => {
    const s = await signIn();
    for (let i = 0; i < 10; i++) await newToken(s.cookie);
    const r = await call("/api/link-tokens", { body: {}, headers: browserHeaders(s.cookie) });
    expectError(r, 429, "rate_limited");
    expect(Number(r.res.headers.get("retry-after"))).toBeGreaterThan(0);
    // per user: someone else is not affected
    await newToken((await signIn()).cookie);
  });
});

describe("GET /api/link-tokens/{id}", () => {
  it("waiting: no device, no approve_by; replaces names the current calculator", async () => {
    const { cookie, device } = await linkedDevice();
    const t = await newToken(cookie);
    const r = await poll(cookie, t.id);
    expect(r.res.status).toBe(200);
    expect(r.json).toEqual({
      state: "waiting",
      device: null,
      replaces: { id: device.device_id, suffix: suffixOf(device), label: "Calculator" },
      approve_by: null,
    });
  });

  it("owner only: another account, a malformed or unknown id -> 404; no session -> 401", async () => {
    const a = await signIn();
    const b = await signIn();
    const t = await newToken(a.cookie);
    expectError(await poll(b.cookie, t.id), 404, "not_found");
    expectError(await poll(a.cookie, "lt_nothex"), 404, "not_found");
    expectError(await poll(a.cookie, "lt_0123456789abcdef"), 404, "not_found");
    expectError(await call(`/api/link-tokens/${t.id}`), 401, "unauthorized");
  });

  it("an unused token expires after 10 minutes", async () => {
    const s = await signIn();
    const t = await newToken(s.cookie);
    await dbRun(`UPDATE link_tokens SET expires_at = ? WHERE id = ?`, nowSec(), t.id);
    expect((await poll(s.cookie, t.id)).json).toMatchObject({ state: "expired", device: null, replaces: null });
  });
});

describe("POST /v1/pair/redeem", () => {
  it("pending_approval with approve_by = now + 300 and the calculator's suffix; the phone sees it", async () => {
    const s = await signIn();
    const t = await newToken(s.cookie);
    const d = await registerDevice();
    expect(await linkState(d)).toBe("");
    const r = await redeem(d, t.token);
    expect(r.res.status, r.text).toBe(200);
    expect(r.json).toEqual({ state: "pending_approval", approve_by: nowSec() + 300, suffix: suffixOf(d) });
    const row = await dbFirst<any>(`SELECT used_at, used_by_device_id FROM link_tokens WHERE id = ?`, t.id);
    expect(row).toEqual({ used_at: nowSec(), used_by_device_id: d.device_id });
    const p = await poll(s.cookie, t.id);
    expect(p.json).toEqual({
      state: "pending_approval",
      device: { id: d.device_id, suffix: suffixOf(d), label: "Calculator" },
      replaces: null,
      approve_by: nowSec() + 300,
    });
    expect(await linkState(d)).toBe("pending_approval");
    // Not linked until the tap.
    expect((await dbFirst<any>(`SELECT user_id FROM devices WHERE id = ?`, d.device_id))!.user_id).toBeNull();
  });

  it("malformed or unknown token -> 404 link_token_invalid", async () => {
    const d = await registerDevice();
    for (const bad of ["", "L1.short", "L2." + "a".repeat(22), "L1." + "a".repeat(23), "https://x/q/L1." + "a".repeat(22), 42, null]) {
      expect(expectError(await redeem(d, bad), 404, "link_token_invalid").message).toBe(MSG_INVALID);
    }
    expectError(await redeem(d, "L1." + "a".repeat(22)), 404, "link_token_invalid");
    expectError(await call("/v1/pair/redeem", { body: { link_token: "L1." + "a".repeat(22), x: 1 }, headers: deviceHeaders(d) }), 400, "bad_request");
  });

  it("needs a device token", async () => {
    expectError(await call("/v1/pair/redeem", { body: { link_token: "L1." + "a".repeat(22) } }), 401, "unauthorized");
    expectError(await call("/v1/pair/redeem", { body: { link_token: "L1." + "a".repeat(22) }, headers: { authorization: "Bearer dt_" + "x".repeat(43) } }), 401, "unauthorized");
  });

  it("expired -> 410 link_token_expired", async () => {
    const s = await signIn();
    const t = await newToken(s.cookie);
    await dbRun(`UPDATE link_tokens SET expires_at = ? WHERE id = ?`, nowSec(), t.id);
    const d = await registerDevice();
    expect(expectError(await redeem(d, t.token), 410, "link_token_expired").message).toBe(MSG_EXPIRED);
  });

  it("single use: another calculator gets 409 link_token_used; the same one gets the same answer again", async () => {
    const s = await signIn();
    const t = await newToken(s.cookie);
    const d1 = await registerDevice();
    const d2 = await registerDevice();
    const first = await redeem(d1, t.token);
    expect(first.json.state).toBe("pending_approval");
    expect(expectError(await redeem(d2, t.token), 409, "link_token_used").message).toBe(MSG_USED);
    expect((await redeem(d1, t.token)).json).toEqual(first.json);
    expect((await dbFirst<any>(`SELECT used_by_device_id FROM link_tokens WHERE id = ?`, t.id))!.used_by_device_id).toBe(d1.device_id);
  });

  it("a calculator on another account -> 409 already_linked, and the token stays usable", async () => {
    const other = await linkedDevice();
    const s = await signIn();
    const t = await newToken(s.cookie);
    expect(expectError(await redeem(other.device, t.token), 409, "already_linked").message).toBe(MSG_OTHER);
    expect((await poll(s.cookie, t.id)).json.state).toBe("waiting");
    const d = await registerDevice();
    expect((await redeem(d, t.token)).json.state).toBe("pending_approval");
  });

  it("same-user relink: 200 linked, token used, phone sees approved", async () => {
    const { cookie, device } = await linkedDevice();
    const t = await newToken(cookie);
    const r = await redeem(device, t.token);
    expect(r.res.status, r.text).toBe(200);
    expect(r.json).toEqual({ state: "linked" });
    const row = await dbFirst<any>(`SELECT used_at, used_by_device_id FROM link_tokens WHERE id = ?`, t.id);
    expect(row).toEqual({ used_at: nowSec(), used_by_device_id: device.device_id });
    expect((await poll(cookie, t.id)).json).toMatchObject({ state: "approved", device: { id: device.device_id } });
    expect((await redeem(device, t.token)).json).toEqual({ state: "linked" });
    const d2 = await registerDevice();
    expectError(await redeem(d2, t.token), 409, "link_token_used");
    expect(await linkState(device)).toBe("");
  });

  it("rate limit: 10 per calculator per minute", async () => {
    const d = await registerDevice();
    for (let i = 0; i < 10; i++) expectError(await redeem(d, "L1." + "b".repeat(22)), 404, "link_token_invalid");
    const r = await redeem(d, "L1." + "b".repeat(22));
    expectError(r, 429, "rate_limited");
    expect(Number(r.res.headers.get("retry-after"))).toBeGreaterThan(0);
    // another calculator (another address) is not affected
    expectError(await redeem(await registerDevice(), "L1." + "b".repeat(22), { "x-dev-client-ip": "198.51.100.77" }), 404, "link_token_invalid");
  });

  it("rate limit: 30 per address per minute", async () => {
    const ip = { "x-dev-client-ip": "198.51.100.5" };
    const devices: Device[] = [];
    for (let i = 0; i < 4; i++) devices.push(await registerDevice());
    for (let i = 0; i < 30; i++) expectError(await redeem(devices[i % 3]!, "L1." + "c".repeat(22), ip), 404, "link_token_invalid");
    expectError(await redeem(devices[3]!, "L1." + "c".repeat(22), ip), 429, "rate_limited");
    expectError(await redeem(devices[3]!, "L1." + "c".repeat(22), { "x-dev-client-ip": "198.51.100.6" }), 404, "link_token_invalid");
  });
});

describe("POST /api/link-tokens/{id}/approve", () => {
  async function pending() {
    const s = await signIn();
    const t = await newToken(s.cookie);
    const d = await registerDevice();
    expect((await redeem(d, t.token)).json.state).toBe("pending_approval");
    return { ...s, t, d };
  }

  it("Approve links the calculator (free month as with a code), e-mails a notice, and the calculator sees linked", async () => {
    const { cookie, email, t, d } = await pending();
    const r = await approve(cookie, t.id, true);
    expect(r.res.status, r.text).toBe(200);
    expect(r.json).toEqual({ state: "approved", device: { id: d.device_id, suffix: suffixOf(d), label: "Calculator" } });
    expect((await dbFirst<any>(`SELECT approved_at FROM link_tokens WHERE id = ?`, t.id))!.approved_at).toBe(nowSec());
    const me = await call("/api/me", { headers: { cookie } });
    expect(me.json.plan).toBe("trial");
    expect(me.json.devices.map((x: any) => x.id)).toEqual([d.device_id]);
    expect((await dbFirst<any>(`SELECT trial_used_at FROM hw_allow WHERE hw_id = ?`, d.hw_id))!.trial_used_at).toBe(nowSec());
    const st = await call("/v1/device/status", { headers: deviceHeaders(d) });
    expect(st.json).toMatchObject({ linked: true, plan: "trial", link_state: "" });
    const mail = await call(`/__dev/outbox?email=${encodeURIComponent(email)}&kind=linked`);
    expect(mail.res.status, mail.text).toBe(200);
    expect(mail.json.link).toBe("http://localhost:8787/account/");
    // The sign-in outbox still returns sign-in links only.
    expect((await call(`/__dev/outbox?email=${encodeURIComponent(email)}`)).json.token).toMatch(/^[A-Za-z0-9_-]{43}$/);
    expect((await poll(cookie, t.id)).json.state).toBe("approved");
    // Approving again: wrong state.
    expectError(await approve(cookie, t.id, true), 409, "link_token_state");
  });

  it("an unenrolled board links without a free month (same rules as a code)", async () => {
    const s = await signIn();
    const t = await newToken(s.cookie);
    const d = await registerDevice({}, { enrolled: false });
    await redeem(d, t.token);
    expect((await approve(s.cookie, t.id, true)).res.status).toBe(200);
    expect((await call("/api/me", { headers: { cookie: s.cookie } })).json.plan).toBe("none");
  });

  it("No: declined, token dead, the calculator sees declined for 10 minutes, then nothing", async () => {
    const { cookie, t, d } = await pending();
    const r = await approve(cookie, t.id, false);
    expect(r.res.status, r.text).toBe(200);
    expect(r.json).toEqual({ state: "declined" });
    expect((await poll(cookie, t.id)).json).toMatchObject({ state: "declined", replaces: null });
    expect(await linkState(d)).toBe("declined");
    expect((await dbFirst<any>(`SELECT user_id FROM devices WHERE id = ?`, d.device_id))!.user_id).toBeNull();
    expectError(await approve(cookie, t.id, true), 409, "link_token_state");
    expectError(await redeem(d, t.token), 409, "link_token_used");
    await dbRun(`UPDATE link_tokens SET declined_at = ? WHERE id = ?`, nowSec() - 600, t.id);
    expect(await linkState(d)).toBe("");
  });

  it("the approve window: after approve_by the tap is refused and the token is expired", async () => {
    const { cookie, t, d } = await pending();
    await dbRun(`UPDATE link_tokens SET approve_by = ? WHERE id = ?`, nowSec(), t.id);
    expect(await linkState(d)).toBe("pending_approval"); // the last second still counts
    await dbRun(`UPDATE link_tokens SET approve_by = ? WHERE id = ?`, nowSec() - 1, t.id);
    expect((await poll(cookie, t.id)).json.state).toBe("expired");
    expect(await linkState(d)).toBe("");
    expectError(await approve(cookie, t.id, true), 409, "link_token_state");
    expectError(await approve(cookie, t.id, false), 409, "link_token_state");
    expect((await dbFirst<any>(`SELECT user_id FROM devices WHERE id = ?`, d.device_id))!.user_id).toBeNull();
  });

  it("wrong state: a waiting token can't be approved; wrong owner -> 404; bad body -> 400", async () => {
    const s = await signIn();
    const t = await newToken(s.cookie);
    expectError(await approve(s.cookie, t.id, true), 409, "link_token_state");
    const p = await pending();
    const other = await signIn();
    expectError(await approve(other.cookie, p.t.id, true), 404, "not_found");
    expectError(await approve(p.cookie, p.t.id, "yes"), 400, "bad_request");
    expectError(await call(`/api/link-tokens/${p.t.id}/approve`, { body: {}, headers: browserHeaders(p.cookie) }), 400, "bad_request");
    expectError(
      await call(`/api/link-tokens/${p.t.id}/approve`, { body: { approve: true }, headers: { ...browserHeaders(p.cookie), origin: "https://evil.example" } }),
      403,
      "forbidden",
    );
    expect((await poll(p.cookie, p.t.id)).json.state).toBe("pending_approval");
  });

  it("replace: the phone is warned, and approving revokes the old calculator", async () => {
    const { cookie, device: old } = await linkedDevice();
    const t = await newToken(cookie);
    const d = await registerDevice();
    await redeem(d, t.token);
    const p = await poll(cookie, t.id);
    expect(p.json.replaces).toEqual({ id: old.device_id, suffix: suffixOf(old), label: "Calculator" });
    const r = await approve(cookie, t.id, true);
    expect(r.res.status, r.text).toBe(200);
    expect((await dbFirst<any>(`SELECT revoked_at FROM devices WHERE id = ?`, old.device_id))!.revoked_at).toBe(nowSec());
    expectError(await call("/v1/device/status", { headers: deviceHeaders(old) }), 401, "device_revoked");
    expect((await call("/v1/device/status", { headers: deviceHeaders(d) })).json.linked).toBe(true);
    // The old free month carries over (the account had it already); no second one.
    expect((await call("/api/me", { headers: { cookie } })).json.plan).toBe("trial");
  });

  it("the calculator was linked by code (way 1) before the tap: 409, nothing revoked", async () => {
    const { cookie, t, d } = await pending();
    const other = await signIn();
    expect((await claimCode(other.cookie, d.pair_code)).res.status).toBe(201);
    expectError(await approve(cookie, t.id, true), 409, "link_token_state");
    expect((await dbFirst<any>(`SELECT user_id FROM devices WHERE id = ?`, d.device_id))!.user_id).not.toBeNull();
  });
});

describe("cleanup", () => {
  it("the hourly cron deletes link tokens older than a day", async () => {
    const s = await signIn();
    const t1 = await newToken(s.cookie);
    const t2 = await newToken(s.cookie);
    await dbRun(`UPDATE link_tokens SET created_at = ? WHERE id = ?`, nowSec() - 86401, t1.id);
    await cleanup(env as any, nowSec());
    expect(await dbFirst(`SELECT id FROM link_tokens WHERE id = ?`, t1.id)).toBeNull();
    expect(await dbFirst(`SELECT id FROM link_tokens WHERE id = ?`, t2.id)).not.toBeNull();
  });
});

describe("/q/ (where a phone camera lands)", () => {
  it("/q/L1.<token> -> 302 to /q/ (the token leaves the address bar)", async () => {
    const r = await call("/q/L1." + "a".repeat(22));
    expect(r.res.status).toBe(302);
    expect(r.res.headers.get("location")).toBe("/q/");
    expect(r.res.headers.get("referrer-policy")).toBe("no-referrer");
  });

  it("/q/ serves the static explanation page", async () => {
    const r = await call("/q/");
    expect(r.res.status).toBe(200);
    expect(r.text).toContain("This code is for your Proofline calculator");
  });
});
