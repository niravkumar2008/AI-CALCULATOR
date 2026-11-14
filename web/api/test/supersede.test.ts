// The calculator's effort setting (SolveRequest.effort) and one solve per calculator: a new
// solve replaces the one still running instead of being refused with "still running".

import { createExecutionContext, waitOnExecutionContext } from "cloudflare:test";
import { describe, expect, it, vi } from "vitest";
import worker from "../src/index";
import { config } from "../src/env";
import { MAX_OUTPUT_TOKENS_PER_SEC, stoppedEarlyCostMicro } from "../src/solve";
import {
  BASE,
  type Device,
  cannedSse,
  dbFirst,
  dbRun,
  deviceHeaders,
  expectError,
  linkedDevice,
  mockAnthropic,
  nowSec,
  solve,
  solveBody,
  sseResponse,
  testEnv,
} from "./helpers";

const PRICE_IN = 2;
const PRICE_OUT = 10;
const WORST_ONE_PHOTO = (3000 + 5000) * PRICE_IN + 16000 * PRICE_OUT; // 176,000

describe("SolveRequest.effort: the calculator's Normal / Careful / Max", () => {
  it("maps to the upstream effort; absent keeps the EFFORT default", async () => {
    const { device } = await linkedDevice();
    const { calls } = mockAnthropic();
    const cases: [string | undefined, string][] = [
      [undefined, config(testEnv()).effort], // EFFORT in wrangler.toml
      ["normal", "medium"],
      ["careful", "high"],
      ["max", "xhigh"],
    ];
    for (const [effort, want] of cases) {
      const body = effort === undefined ? solveBody() : { ...solveBody(), effort };
      const r = await solve(device, body);
      expect(r.res.status, r.text).toBe(200);
      expect(calls.at(-1)!.body.output_config.effort).toBe(want);
      expect(calls.at(-1)!.body.thinking).toEqual({ type: "adaptive" });
    }
  });

  it("the mapping is a server setting (EFFORT_NORMAL / EFFORT_CAREFUL / EFFORT_MAX)", async () => {
    const { device } = await linkedDevice();
    const { calls } = mockAnthropic();
    const over = { EFFORT_NORMAL: "low", EFFORT_CAREFUL: "medium", EFFORT_MAX: "max" };
    for (const [effort, want] of [["normal", "low"], ["careful", "medium"], ["max", "max"]]) {
      expect((await solve(device, { ...solveBody(), effort }, over)).res.status).toBe(200);
      expect(calls.at(-1)!.body.output_config.effort).toBe(want);
    }
    expect(() => config(testEnv({ EFFORT_NORMAL: "turbo" }))).toThrow(/EFFORT_NORMAL/);
    expect(() => config(testEnv({ EFFORT_MAX: "MAX" }))).toThrow(/EFFORT_MAX/);
  });

  it("anything outside the allow-list is a 400 and never reaches Claude", async () => {
    const { spy } = mockAnthropic();
    for (const effort of ["ultra", "xhigh", "high", "Normal", "", 3, null, true, ["max"], { level: "max" }]) {
      const { device } = await linkedDevice(); // a fresh one each time: 6 solves a minute per calculator
      const r = await solve(device, { ...solveBody(), effort });
      const e = expectError(r, 400, "bad_request");
      expect(e.message).toBe("effort must be normal, careful or max.");
    }
    expect(spy).not.toHaveBeenCalled();
  });

  it("a legacy (Messages-API) body cannot pick the effort", async () => {
    const { device } = await linkedDevice();
    const { calls } = mockAnthropic();
    const legacy = {
      effort: "max",
      output_config: { effort: "max" },
      messages: [{ role: "user", content: [{ type: "image", source: { type: "base64", media_type: "image/jpeg", data: solveBody().images[0]!.data } }] }],
    };
    expect((await solve(device, legacy)).res.status).toBe(200);
    expect(calls[0]!.body.output_config.effort).toBe(config(testEnv()).effort);
  });
});

function solveRequest(d: Device, body: unknown = solveBody()): Request {
  return new Request(`${BASE}/v1/solve`, {
    method: "POST",
    headers: { ...deviceHeaders(d), "content-type": "application/json" },
    body: JSON.stringify(body),
  });
}

/** The first SSE event of cannedSse(): message_start with the input usage. */
function messageStartOnly(input: number): string {
  const sse = cannedSse({ input, output: 1 });
  return sse.slice(0, sse.indexOf("\n\n") + 2);
}

async function waitFor<T>(f: () => Promise<T | null | undefined>, ms = 5000): Promise<T> {
  const until = Date.now() + ms;
  for (;;) {
    const v = await f();
    if (v) return v;
    if (Date.now() > until) throw new Error("timed out waiting");
    await new Promise((r) => setTimeout(r, 20));
  }
}

describe("one solve per calculator: a new solve replaces the running one", () => {
  it("is not refused by a reservation still open on this calculator; that one is marked replaced", async () => {
    const { device } = await linkedDevice();
    const u = await dbFirst<{ user_id: string }>(`SELECT user_id FROM devices WHERE id = ?`, device.device_id);
    const t = nowSec();
    const day = new Date(t * 1000).toISOString().slice(0, 10);
    const ins = (id: string, startedAt: number) =>
      dbRun(
        `INSERT INTO usage (id, device_id, user_id, started_at, state, reserved_micro, day, month)
         VALUES (?, ?, ?, ?, 'reserved', 1000, ?, ?)`,
        id,
        device.device_id,
        u!.user_id,
        startedAt,
        day,
        day.slice(0, 7),
      );
    await ins("use_running", t - 5);
    await ins("use_stale", t - 601); // the cron settles these; nothing to replace
    const { calls } = mockAnthropic();
    const r = await solve(device);
    expect(r.res.status, r.text).toBe(200);
    expect(calls).toHaveLength(1);

    const sup = await dbFirst<{ by_usage_id: string }>(`SELECT * FROM usage_superseded WHERE usage_id = 'use_running'`);
    const mine = await dbFirst<{ id: string }>(
      `SELECT id FROM usage WHERE device_id = ? AND id NOT IN ('use_running','use_stale')`,
      device.device_id,
    );
    expect(sup!.by_usage_id).toBe(mine!.id);
    expect(await dbFirst(`SELECT 1 FROM usage_superseded WHERE usage_id = 'use_stale'`)).toBeNull();
    // The replaced row keeps its reservation (its own stream settles it), so the caps still count it.
    expect((await dbFirst<{ state: string }>(`SELECT state FROM usage WHERE id = 'use_running'`))!.state).toBe("reserved");
  });

  it("a refused new solve (over the limit) does not stop the running one", async () => {
    const { device } = await linkedDevice();
    const u = await dbFirst<{ user_id: string }>(`SELECT user_id FROM devices WHERE id = ?`, device.device_id);
    const t = nowSec();
    const day = new Date(t * 1000).toISOString().slice(0, 10);
    await dbRun(
      `INSERT INTO usage (id, device_id, user_id, started_at, state, reserved_micro, day, month)
       VALUES ('use_keep', ?, ?, ?, 'reserved', 1000, ?, ?)`,
      device.device_id,
      u!.user_id,
      t - 5,
      day,
      day.slice(0, 7),
    );
    mockAnthropic();
    expectError(await solve(device, solveBody(), { MONTHLY_SOLVES: "1" }), 429, "quota_exceeded");
    expect(await dbFirst(`SELECT 1 FROM usage_superseded WHERE usage_id = 'use_keep'`)).toBeNull();
  });

  it("the replaced stream stops reading from Claude, tells its client, and settles what it used", async () => {
    const { device } = await linkedDevice();
    let upstreamCancelled = false;
    let n = 0;
    mockAnthropic(() => {
      if (n++ > 0) return sseResponse(cannedSse({ input: 1500, output: 800 }));
      // The first solve: Claude has started (input known) and is still thinking.
      const enc = new TextEncoder();
      return sseResponse(
        new ReadableStream<Uint8Array>({
          start(c) {
            c.enqueue(enc.encode(messageStartOnly(1500)));
          },
          cancel() {
            upstreamCancelled = true;
          },
        }),
      );
    });

    const ctxA = createExecutionContext();
    const t0 = Date.now();
    const resA = await worker.fetch(solveRequest(device), testEnv(), ctxA);
    expect(resA.status).toBe(200);
    const textA = resA.text(); // the first calculator is still reading

    const b = await solve(device);
    expect(b.res.status, b.text).toBe(200);
    expect(b.text).toBe(cannedSse({ input: 1500, output: 800 }));

    const a = await textA;
    await waitOnExecutionContext(ctxA);
    const elapsed = Date.now() - t0;
    expect(upstreamCancelled).toBe(true);
    expect(a.startsWith(messageStartOnly(1500))).toBe(true);
    expect(a).toContain("event: error");
    expect(a).toContain('"type":"superseded"');
    expect(a).toContain("A newer solve replaced this one.");

    const rows = await dbFirst<{ n: number }>(`SELECT COUNT(*) AS n FROM usage WHERE device_id = ? AND state = 'reserved'`, device.device_id);
    expect(rows!.n).toBe(0);
    const old = await dbFirst<{ cost_micro: number; input_tokens: number }>(
      `SELECT u.cost_micro, u.input_tokens FROM usage u JOIN usage_superseded s ON s.usage_id = u.id WHERE u.device_id = ? AND u.state = 'settled'`,
      device.device_id,
    );
    // Billed: the input it read plus at most MAX_OUTPUT_TOKENS_PER_SEC for the time it ran. Not the worst case.
    expect(old!.input_tokens).toBe(1500);
    // (workerd's test clock stands still, so `elapsed` is usually 0 here; production clocks move.)
    expect(old!.cost_micro).toBeGreaterThan(1500 * PRICE_IN);
    expect(old!.cost_micro).toBeLessThanOrEqual(
      1500 * PRICE_IN + Math.max(1, Math.ceil((elapsed / 1000) * MAX_OUTPUT_TOKENS_PER_SEC)) * PRICE_OUT,
    );
    expect(old!.cost_micro).toBeLessThan(WORST_ONE_PHOTO);

    // spend_daily: nothing left reserved; settled = both solves' costs.
    const sd = await dbFirst<{ reserved_micro: number; settled_micro: number }>(`SELECT * FROM spend_daily`);
    const all = await dbFirst<{ s: number }>(`SELECT SUM(cost_micro) AS s FROM usage WHERE state = 'settled'`);
    expect(sd!.reserved_micro).toBe(0);
    expect(sd!.settled_micro).toBe(all!.s);
  });

  it("replaced while still waiting for Claude's headers: the upstream call is aborted and nothing is billed", async () => {
    const { device } = await linkedDevice();
    let n = 0;
    let aborted = false;
    vi.spyOn(globalThis, "fetch").mockImplementation(async (input: RequestInfo | URL, init?: RequestInit) => {
      const url = new Request(input as RequestInfo, init).url;
      if (!url.startsWith("https://anthropic.test/")) throw new Error(`unexpected fetch ${url}`);
      if (n++ > 0) return sseResponse(cannedSse());
      return new Promise<Response>((_, rej) => {
        init?.signal?.addEventListener("abort", () => {
          aborted = true;
          rej(new Error("aborted"));
        });
      });
    });

    const ctxA = createExecutionContext();
    const pendingA = worker.fetch(solveRequest(device), testEnv(), ctxA);
    const first = await waitFor(() =>
      dbFirst<{ id: string }>(`SELECT id FROM usage WHERE device_id = ? AND state = 'reserved'`, device.device_id),
    );
    expect((await solve(device)).res.status).toBe(200);

    const resA = await pendingA;
    expect(resA.status).toBe(502);
    await waitOnExecutionContext(ctxA);
    expect(aborted).toBe(true);
    const row = await dbFirst<{ state: string; cost_micro: number }>(`SELECT * FROM usage WHERE id = ?`, first.id);
    expect(row).toMatchObject({ state: "failed", cost_micro: 0 });
    const sd = await dbFirst<{ reserved_micro: number }>(`SELECT * FROM spend_daily`);
    expect(sd!.reserved_micro).toBe(0);
  });
});

describe("stoppedEarlyCostMicro", () => {
  const cfg = config(testEnv());
  const u = { input: 1500, cacheWrite: 0, cacheRead: 0, output: 0 };
  it("bills input plus the most output possible in the time it ran, capped at the reservation", () => {
    expect(stoppedEarlyCostMicro(cfg, u, 0, WORST_ONE_PHOTO)).toBe(1500 * PRICE_IN);
    expect(stoppedEarlyCostMicro(cfg, u, 2000, WORST_ONE_PHOTO)).toBe(1500 * PRICE_IN + 500 * PRICE_OUT);
    expect(stoppedEarlyCostMicro(cfg, { ...u, output: 900 }, 2000, WORST_ONE_PHOTO)).toBe(1500 * PRICE_IN + 900 * PRICE_OUT);
    expect(stoppedEarlyCostMicro(cfg, u, 600_000, WORST_ONE_PHOTO)).toBe(1500 * PRICE_IN + 16000 * PRICE_OUT);
    expect(stoppedEarlyCostMicro(cfg, u, 600_000, 50_000)).toBe(50_000);
  });
});
