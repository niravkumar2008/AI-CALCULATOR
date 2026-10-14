import { describe, expect, it } from "vitest";
import {
  DEVICE_TOKEN_RE,
  formatPairCode,
  newDeviceToken,
  newPairCode,
  normalisePairCode,
  safeEqual,
  sha256Hex,
} from "../src/crypto";
import { call, expectError } from "./helpers";

describe("tokens and codes", () => {
  it("device tokens are dt_ + 43 base64url chars and unique", () => {
    const a = newDeviceToken();
    const b = newDeviceToken();
    expect(a).toMatch(DEVICE_TOKEN_RE);
    expect(a).toMatch(/^dt_[A-Za-z0-9_-]{43}$/);
    expect(a).not.toBe(b);
  });

  it("hashes are SHA-256 hex", async () => {
    expect(await sha256Hex("abc")).toBe("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  });

  it("pair codes use the 32-char alphabet and normalise", () => {
    for (let i = 0; i < 200; i++) {
      const c = newPairCode();
      expect(c).toMatch(/^[A-HJ-NP-Z2-9]{8}$/);
      expect(formatPairCode(c)).toMatch(/^[A-HJ-NP-Z2-9]{4}-[A-HJ-NP-Z2-9]{4}$/);
    }
    expect(normalisePairCode(" abcd-efgh ")).toBe("ABCDEFGH");
    expect(normalisePairCode("abcd efgh")).toBe("ABCDEFGH");
    expect(normalisePairCode("ABCD-EFG0")).toBeNull(); // 0 isn't in the alphabet
    expect(normalisePairCode("ABCDEFG")).toBeNull();
  });

  it("constant-time compare", async () => {
    expect(await safeEqual("secret-token", "secret-token")).toBe(true);
    expect(await safeEqual("secret-token", "secret-tokem")).toBe(false);
    expect(await safeEqual("secret", "secret-token")).toBe(false);
  });
});

describe("routing and error shape", () => {
  it("healthz reveals nothing", async () => {
    const r = await call("/healthz");
    expect(r.res.status).toBe(200);
    expect(r.json).toEqual({ ok: true });
  });

  it("404 and 405 use the error shape", async () => {
    expectError(await call("/nope"), 404, "not_found");
    const r = await call("/v1/solve", { method: "GET" });
    expectError(r, 405, "bad_request");
    expect(r.res.headers.get("allow")).toBe("POST");
  });

  it("stage-3 endpoints exist and only take POST (details in billing.test.ts)", async () => {
    for (const p of ["/api/billing/checkout", "/api/billing/portal", "/api/store/checkout", "/api/stripe/webhook", "/api/waitlist"]) {
      const r = await call(p, { method: "GET" });
      expectError(r, 405, "bad_request");
      expect(r.res.headers.get("allow")).toBe("POST");
    }
  });

  it("a bad setting fails closed with 503 and no details", async () => {
    const r = await call("/healthz", { env: { MAX_TOKENS: "lots" } });
    const e = expectError(r, 503, "unavailable");
    expect(e.message).not.toMatch(/MAX_TOKENS/);
  });
});
