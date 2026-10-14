// The waitlist for the marketing launch: the 13+ confirmation, per-channel tracking (utm_* and
// a referral code), and the daily cap as an abuse backstop. Basic behaviour is in billing.test.ts.

import { env } from "cloudflare:workers";
import { describe, expect, it } from "vitest";
import { ORIGIN, browserHeaders, call, expectError, freshIp } from "./helpers";
import { AGE_MSG } from "../src/waitlist";

const headers = () => ({ ...browserHeaders(), "x-dev-client-ip": freshIp() });
const join = (body: object, e = {}) => call("/api/waitlist", { body, headers: headers(), env: e });
const post = (form: string) =>
  call("/api/waitlist", { body: form, headers: { ...headers(), "content-type": "application/x-www-form-urlencoded" } });
const row = (email: string) =>
  env.DB.prepare(
    `SELECT source, utm_source, utm_medium, utm_campaign, ref, age_ok FROM waitlist WHERE email = ?1`,
  )
    .bind(email)
    .first();
const count = async () => ((await env.DB.prepare(`SELECT COUNT(*) AS n FROM waitlist`).first()) as { n: number }).n;

describe("waitlist: I'm 13 or older", () => {
  it("JSON needs age_ok: true; missing, false or a string is a 400 with a clear message", async () => {
    for (const age_ok of [undefined, false, "true", 1, "on"]) {
      const e = expectError(await join({ email: "a@example.com", age_ok }), 400, "bad_request");
      expect(e.message).toBe(AGE_MSG);
    }
    expect(await count()).toBe(0);
    expect((await join({ email: "a@example.com", age_ok: true })).res.status).toBe(201);
    expect(await row("a@example.com")).toMatchObject({ age_ok: 1 });
  });

  it("a form post needs age_ok=on (an unticked checkbox sends nothing)", async () => {
    expectError(await post("email=f%40example.com"), 400, "bad_request");
    expectError(await post("email=f%40example.com&age_ok=yes"), 400, "bad_request");
    expect(await count()).toBe(0);
    const r = await post("email=f%40example.com&age_ok=on");
    expect(r.res.status).toBe(303);
    expect(r.res.headers.get("location")).toBe(`${ORIGIN}/?waitlist=joined#waitlist`);
    expect(await row("f@example.com")).toMatchObject({ age_ok: 1 });
  });

  it("a bad email is still reported first", async () => {
    expect(expectError(await join({ email: "nope" }), 400, "bad_request").message).toMatch(/email/i);
  });
});

describe("waitlist: per-channel tracking", () => {
  it("stores utm_source, utm_medium, utm_campaign and ref (lower-cased)", async () => {
    const r = await join({
      email: "t@example.com",
      age_ok: true,
      source: "hero",
      utm_source: "TikTok",
      utm_medium: "video",
      utm_campaign: "launch_week-1",
      ref: "abc123",
    });
    expect(r.res.status, r.text).toBe(201);
    expect(r.json).toEqual({ ok: true });
    expect(await row("t@example.com")).toEqual({
      source: "hero",
      utm_source: "tiktok",
      utm_medium: "video",
      utm_campaign: "launch_week-1",
      ref: "abc123",
      age_ok: 1,
    });
  });

  it("no tracking: all NULL; an unknown form source falls back to site", async () => {
    await join({ email: "n@example.com", age_ok: true, source: "instagram" });
    expect(await row("n@example.com")).toEqual({
      source: "site",
      utm_source: null,
      utm_medium: null,
      utm_campaign: null,
      ref: null,
      age_ok: 1,
    });
  });

  it("a value that isn't short and plain is dropped, not stored and not a failed sign-up", async () => {
    const r = await join({
      email: "v@example.com",
      age_ok: true,
      utm_source: "a".repeat(41),
      utm_medium: "<script>",
      utm_campaign: "spring sale",
      ref: "",
    });
    expect(r.res.status).toBe(201);
    expect(await row("v@example.com")).toMatchObject({ utm_source: null, utm_medium: null, utm_campaign: null, ref: null });
    await join({ email: "w@example.com", age_ok: true, utm_source: "a".repeat(40) });
    expect(await row("w@example.com")).toMatchObject({ utm_source: "a".repeat(40) });
  });

  it("a non-string tracking value or an unknown field is a 400", async () => {
    expectError(await join({ email: "x@example.com", age_ok: true, utm_source: 5 }), 400, "bad_request");
    expectError(await join({ email: "x@example.com", age_ok: true, ref: ["a"] }), 400, "bad_request");
    expectError(await join({ email: "x@example.com", age_ok: true, utm_term: "a" }), 400, "bad_request");
    expect(await count()).toBe(0);
  });

  it("the no-JS form post carries the hidden fields; empty ones mean none", async () => {
    await post("email=h%40example.com&age_ok=on&source=footer&utm_source=yt&utm_medium=&utm_campaign=&ref=&company=");
    expect(await row("h@example.com")).toEqual({
      source: "footer",
      utm_source: "yt",
      utm_medium: null,
      utm_campaign: null,
      ref: null,
      age_ok: 1,
    });
  });

  it("a repeat sign-up gets the same answer and keeps the first touch", async () => {
    const a = await join({ email: "r@example.com", age_ok: true, utm_source: "tiktok", ref: "first" });
    const b = await join({ email: "R@example.com", age_ok: true, utm_source: "youtube", ref: "second" });
    expect(b.res.status).toBe(a.res.status);
    expect(b.json).toEqual(a.json);
    expect(await row("r@example.com")).toMatchObject({ utm_source: "tiktok", ref: "first" });
    expect(await count()).toBe(1);
  });
});

describe("waitlist: the daily cap is an abuse backstop", () => {
  it("defaults to 20,000 new addresses a day", async () => {
    // Pretend 19,999 new addresses joined today: one more fits, the next one is turned away.
    const day = Math.floor(Date.now() / 1000 / 86400) * 86400;
    await env.DB.prepare(`INSERT INTO rate_limits (key, window_start, count) VALUES ('waitlist:day', ?1, 19999)`)
      .bind(day)
      .run();
    expect((await join({ email: "last@example.com", age_ok: true })).res.status).toBe(201);
    const full = expectError(await join({ email: "over@example.com", age_ok: true }), 429, "rate_limited");
    expect(full.message).toMatch(/busy today/);
    expect(await row("over@example.com")).toBeNull();
  });

  it("when full: nothing is stored, the IP counter isn't touched, repeats don't count", async () => {
    const cap = { WAITLIST_DAILY_CAP: "2" };
    const h = headers();
    const j = (email: string) => call("/api/waitlist", { body: { email, age_ok: true }, headers: h, env: cap });
    expect((await j("c1@example.com")).res.status).toBe(201);
    expect((await j("c1@example.com")).res.status).toBe(201); // a repeat isn't a new address
    expect((await j("c2@example.com")).res.status).toBe(201);
    expectError(await j("c3@example.com"), 429, "rate_limited");
    expect(await count()).toBe(2);
    expect(await row("c3@example.com")).toBeNull();
    // 3 requests got through to the IP counter; the refused one didn't add a 4th.
    const ip = await env.DB.prepare(`SELECT count FROM rate_limits WHERE key LIKE 'waitlist:ip:%'`).first();
    expect(ip).toEqual({ count: 3 });
  });

  it("5 sign-ups per IP per hour still applies", async () => {
    const h = headers();
    const j = (email: string) => call("/api/waitlist", { body: { email, age_ok: true }, headers: h });
    for (let i = 0; i < 5; i++) expect((await j(`ip${i}@example.com`)).res.status).toBe(201);
    expectError(await j("ip5@example.com"), 429, "rate_limited");
  });
});
