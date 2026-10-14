// 2026-10-11 contract additions for devices: DeviceStatus.pair_code / link_url / plan_reason,
// and per-family firmware channels with a rollback mark.

import { describe, expect, it } from "vitest";
import { createExecutionContext, waitOnExecutionContext } from "cloudflare:test";
import worker from "../src/index";
import {
  ADMIN_TOKEN,
  call,
  claimCode,
  dbRun,
  deviceHeaders,
  expectError,
  linkedDevice,
  nowSec,
  registerDevice,
  signIn,
  testEnv,
} from "./helpers";

const LINK = "http://localhost:8787/link/";

async function status(d: { token: string }) {
  const r = await call("/v1/device/status", { headers: deviceHeaders(d as never) });
  expect(r.res.status, r.text).toBe(200);
  return r.json;
}

describe("DeviceStatus structured fields", () => {
  it("not linked: pair_code is the code the reason names, link_url is the site's /link/", async () => {
    const d = await registerDevice();
    const s = await status(d);
    expect(s).toMatchObject({ linked: false, plan: "none", can_solve: false, plan_reason: "not_linked", link_url: LINK });
    expect(s.pair_code).toMatch(/^[A-HJ-NP-Z2-9]{4}-[A-HJ-NP-Z2-9]{4}$/);
    expect(s.pair_code).toBe(d.pair_code);
    expect(s.reason).toContain(`enter code ${s.pair_code}`);
    // The reason text is unchanged from before the new fields.
    expect(s.reason).toBe(`Link this calculator: go to localhost:8787/link and enter code ${s.pair_code}`);
    // Polling doesn't churn the code.
    expect((await status(d)).pair_code).toBe(s.pair_code);
  });

  it("not linked: follows pair/start's new code", async () => {
    const d = await registerDevice();
    const p = await call("/v1/pair/start", { method: "POST", headers: deviceHeaders(d) });
    expect(p.res.status, p.text).toBe(200);
    expect((await status(d)).pair_code).toBe(p.json.code);
  });

  it("linked trial: plan_reason trial, pair_code null, link_url still present", async () => {
    const { device } = await linkedDevice();
    const s = await status(device);
    expect(s).toMatchObject({ linked: true, plan: "trial", can_solve: true, plan_reason: "trial", pair_code: null, link_url: LINK });
    expect(s.reason).toBeUndefined();
  });

  it("free month over: plan_reason free_month_over agrees with reason", async () => {
    const { device, email } = await linkedDevice();
    await dbRun(`UPDATE users SET trial_ends_at = ?1 WHERE email = ?2`, nowSec() - 60, email);
    const s = await status(device);
    expect(s).toMatchObject({ linked: true, plan: "inactive", can_solve: false, plan_reason: "free_month_over" });
    expect(s.reason).toMatch(/^Your free month is over\./);
  });

  it("subscription ended: plan_reason inactive", async () => {
    const { device, email } = await linkedDevice();
    await dbRun(`UPDATE users SET plan_override = 'inactive' WHERE email = ?1`, email);
    const s = await status(device);
    expect(s).toMatchObject({ plan: "inactive", can_solve: false, plan_reason: "inactive" });
    expect(s.reason).toMatch(/^Your subscription has ended\./);
  });

  it("active and past_due", async () => {
    const { device, email } = await linkedDevice();
    await dbRun(`UPDATE users SET plan_override = 'active' WHERE email = ?1`, email);
    expect(await status(device)).toMatchObject({ plan: "active", can_solve: true, plan_reason: "active" });
    await dbRun(`UPDATE users SET plan_override = 'past_due' WHERE email = ?1`, email);
    expect(await status(device)).toMatchObject({ plan: "past_due", can_solve: true, plan_reason: "past_due" });
  });

  it("linked unenrolled board (no free month): plan_reason no_trial", async () => {
    const d = await registerDevice({}, { enrolled: false });
    const s0 = await signIn();
    const c = await claimCode(s0.cookie, d.pair_code);
    expect(c.res.status, c.text).toBe(201);
    const s = await status(d);
    expect(s).toMatchObject({ linked: true, plan: "none", can_solve: false, plan_reason: "no_trial", pair_code: null });
    expect(s.reason).toMatch(/^No free month for this calculator\./);
  });
});

describe("firmware families and rollback", () => {
  function image(size: number, fill: number): Uint8Array {
    const b = new Uint8Array(size).fill(fill);
    b[0] = 0xe9;
    return b;
  }
  const admin = { "x-admin-token": ADMIN_TOKEN, "content-type": "application/octet-stream" };
  const publish = (q: string, img: Uint8Array) => call(`/v1/admin/firmware?${q}`, { body: img, headers: admin });

  async function download(d: { token: string }, path: string): Promise<Uint8Array> {
    const req = new Request("http://localhost:8787" + path, { headers: deviceHeaders(d as never) });
    const ctx = createExecutionContext();
    const res = await worker.fetch(req, testEnv(), ctx);
    expect(res.status).toBe(200);
    const got = new Uint8Array(await res.arrayBuffer());
    await waitOnExecutionContext(ctx);
    return got;
  }

  it("a family only sees its own images; no family = legacy channel as before", async () => {
    const d = await registerDevice();
    // nothing for the family yet
    let chk = await call("/v1/firmware?family=v15lcd", { headers: deviceHeaders(d) });
    expect(chk.json).toEqual({ update: false, family: "v15lcd" });

    expect((await publish("version=stage14-1", image(120_000, 1))).res.status).toBe(201);
    // the legacy image is NOT offered to a family
    chk = await call("/v1/firmware?family=v15lcd", { headers: deviceHeaders(d) });
    expect(chk.json).toEqual({ update: false, family: "v15lcd" });

    const p = await publish("version=v15lcd-2026.10.11&family=v15lcd", image(130_000, 2));
    expect(p.res.status, p.text).toBe(201);
    expect(p.json).toMatchObject({ version: "v15lcd-2026.10.11", size: 130_000, family: "v15lcd", rollback: false });

    chk = await call("/v1/firmware?family=v15lcd", { headers: { ...deviceHeaders(d), "x-firmware": "v15lcd-2026.10.10" } });
    expect(chk.json).toMatchObject({
      update: true,
      version: "v15lcd-2026.10.11",
      size: 130_000,
      path: "/v1/firmware/image?family=v15lcd",
      family: "v15lcd",
      rollback: false,
    });
    expect((await download(d, chk.json.path))[1]).toBe(2);

    // legacy channel unchanged by the family publish
    chk = await call("/v1/firmware", { headers: deviceHeaders(d) });
    expect(chk.json).toMatchObject({ update: true, version: "stage14-1", path: "/v1/firmware/image", family: null, rollback: false });
    expect((await download(d, "/v1/firmware/image"))[1]).toBe(1);

    // another family sees nothing; its image 404s
    chk = await call("/v1/firmware?family=v16", { headers: deviceHeaders(d) });
    expect(chk.json).toEqual({ update: false, family: "v16" });
    expectError(await call("/v1/firmware/image?family=v16", { headers: deviceHeaders(d) }), 404, "not_found");

    // empty family = no family
    chk = await call("/v1/firmware?family=", { headers: deviceHeaders(d) });
    expect(chk.json.version).toBe("stage14-1");
  });

  it("malformed family -> 400 on check and download", async () => {
    const d = await registerDevice();
    for (const f of ["v15-lcd", "a b", "x".repeat(25), "../meta"]) {
      expectError(await call(`/v1/firmware?family=${encodeURIComponent(f)}`, { headers: deviceHeaders(d) }), 400, "bad_request");
      expectError(await call(`/v1/firmware/image?family=${encodeURIComponent(f)}`, { headers: deviceHeaders(d) }), 400, "bad_request");
    }
  });

  it("admin publish: family must prefix the version; family and rollback validated", async () => {
    expectError(await publish("version=v16-1&family=v15lcd", image(120_000, 3)), 400, "bad_request");
    expectError(await publish("version=v15lcd-1&family=v15-lcd", image(120_000, 3)), 400, "bad_request");
    expectError(await publish("version=v15lcd-1&family=v15lcd&rollback=yes", image(120_000, 3)), 400, "bad_request");
  });

  it("rollback: the manifest says so until the next normal publish", async () => {
    const d = await registerDevice();
    const h = { ...deviceHeaders(d), "x-firmware": "v15lcd-2026.10.11" };
    const rb = await publish("version=v15lcd-2026.10.09&family=v15lcd&rollback=1", image(120_000, 4));
    expect(rb.res.status, rb.text).toBe(201);
    expect(rb.json.rollback).toBe(true);
    let chk = await call("/v1/firmware?family=v15lcd", { headers: h });
    expect(chk.json).toMatchObject({ update: true, version: "v15lcd-2026.10.09", rollback: true, family: "v15lcd" });

    await publish("version=v15lcd-2026.10.12&family=v15lcd", image(120_000, 5));
    chk = await call("/v1/firmware?family=v15lcd", { headers: h });
    expect(chk.json).toMatchObject({ update: true, version: "v15lcd-2026.10.12", rollback: false });

    // rollback=true works on the legacy channel too
    const leg = await publish("version=stage13&rollback=true", image(120_000, 6));
    expect(leg.json).toMatchObject({ family: null, rollback: true });
  });
});
