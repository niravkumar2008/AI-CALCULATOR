// Test helpers: call the Worker directly, register/sign in/link, and mock Anthropic.

import { createExecutionContext, waitOnExecutionContext } from "cloudflare:test";
import { env } from "cloudflare:workers";
import { expect, vi } from "vitest";
import worker from "../src/index";
import type { Env } from "../src/env";

export const BASE = "http://localhost:8787";
export const ORIGIN = "http://localhost:8787";

export interface CallOpts {
  method?: string;
  body?: unknown; // object -> JSON; string/Uint8Array sent as is
  headers?: Record<string, string>;
  env?: Partial<Env>;
  base?: string;
  /** Wait for ctx.waitUntil work (e.g. settlement) before returning. Reads the body first. */
  settle?: boolean;
}

export interface CallResult {
  res: Response;
  text: string;
  json: any;
}

let ipCounter = 0;
/** A fresh client IP, so IP rate limits don't leak between helper calls. */
export function freshIp(): string {
  ipCounter++;
  return `10.0.${(ipCounter >> 8) & 255}.${ipCounter & 255}`;
}

export function testEnv(over: Partial<Env> = {}): Env {
  return { ...(env as unknown as Env), ...over };
}

export async function call(path: string, o: CallOpts = {}): Promise<CallResult> {
  const headers: Record<string, string> = { ...(o.headers ?? {}) };
  let body: BodyInit | undefined;
  if (o.body !== undefined) {
    if (typeof o.body === "string" || o.body instanceof Uint8Array) body = o.body as BodyInit;
    else {
      body = JSON.stringify(o.body);
      headers["content-type"] ??= "application/json";
    }
  }
  const req = new Request((o.base ?? BASE) + path, { method: o.method ?? (body ? "POST" : "GET"), headers, body });
  const ctx = createExecutionContext();
  const res = await worker.fetch(req, testEnv(o.env), ctx);
  const text = await res.text();
  await waitOnExecutionContext(ctx);
  let json: any = undefined;
  try {
    json = JSON.parse(text);
  } catch {
    /* not JSON */
  }
  return { res, text, json };
}

/** Asserts the contract's Error shape and returns the error object. */
export function expectError(r: CallResult, status: number, type: string) {
  expect(r.res.status, r.text).toBe(status);
  expect(r.json?.type).toBe("error");
  expect(r.json?.error?.type).toBe(type);
  expect(typeof r.json.error.message).toBe("string");
  expect(r.json.error.message.length).toBeLessThanOrEqual(110);
  return r.json.error as { type: string; message: string };
}

// ------------------------------------------------------------ devices
let hwCounter = 0;
export interface Device {
  device_id: string;
  token: string;
  pair_code: string;
  link_url: string;
  expires_at: number;
}

export async function registerDevice(over: Partial<Env> = {}): Promise<Device> {
  hwCounter++;
  const r = await call("/v1/device/register", {
    body: { hw_id: `calc-${hwCounter.toString(16).padStart(12, "0")}` },
    headers: { "x-dev-client-ip": freshIp(), "x-firmware": "stage13" },
    env: over,
  });
  expect(r.res.status, r.text).toBe(201);
  return r.json as Device;
}

export function deviceHeaders(d: Device): Record<string, string> {
  return { authorization: `Bearer ${d.token}`, "x-device-id": "calc-0123456789ab", "x-firmware": "stage13" };
}

// ------------------------------------------------------------ portal
export function browserHeaders(cookie?: string): Record<string, string> {
  const h: Record<string, string> = { origin: ORIGIN, "content-type": "application/json", "x-dev-client-ip": freshIp() };
  if (cookie) h.cookie = cookie;
  return h;
}

let emailCounter = 0;
export function freshEmail(): string {
  emailCounter++;
  return `user${emailCounter}-${Math.random().toString(36).slice(2, 8)}@example.com`;
}

/** Signs in through login -> dev outbox -> confirm. Returns the `sid=...` cookie. */
export async function signIn(email = freshEmail()): Promise<{ cookie: string; email: string }> {
  const l = await call("/api/auth/login", { body: { email, age_confirmed: true }, headers: browserHeaders() });
  expect(l.res.status, l.text).toBe(204);
  const o = await call(`/__dev/outbox?email=${encodeURIComponent(email)}`);
  expect(o.res.status, o.text).toBe(200);
  const c = await call("/api/auth/confirm", { body: { token: o.json.token }, headers: browserHeaders() });
  expect(c.res.status, c.text).toBe(204);
  const setCookie = c.res.headers.get("set-cookie") ?? "";
  const m = /sid=([^;]+)/.exec(setCookie);
  expect(m).not.toBeNull();
  return { cookie: `sid=${m![1]}`, email };
}

export async function claimCode(cookie: string, code: string, label?: string) {
  return call("/api/devices/claim", { body: label ? { code, label } : { code }, headers: browserHeaders(cookie) });
}

/** A registered device linked to a fresh signed-in account (trial started). */
export async function linkedDevice() {
  const d = await registerDevice();
  const s = await signIn();
  const c = await claimCode(s.cookie, d.pair_code);
  expect(c.res.status, c.text).toBe(201);
  return { device: d, ...s };
}

// ------------------------------------------------------------ images and mock Anthropic
function b64(bytes: number[], pad = 30): string {
  const all = new Uint8Array([...bytes, ...new Array(pad).fill(7)]);
  let s = "";
  for (const b of all) s += String.fromCharCode(b);
  return btoa(s);
}
export const JPEG_B64 = b64([0xff, 0xd8, 0xff, 0xe0, 0, 0x10, 0x4a, 0x46, 0x49, 0x46]);
export const PNG_B64 = b64([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);

export function sseEvent(type: string, data: object): string {
  return `event: ${type}\ndata: ${JSON.stringify({ type, ...data })}\n\n`;
}

export interface MockUsage {
  input: number;
  output: number;
}

/** A complete Messages API SSE stream with usage in message_start and message_delta. */
export function cannedSse(u: MockUsage = { input: 1500, output: 800 }, text = '{"answer":"42"}'): string {
  return (
    sseEvent("message_start", {
      message: {
        id: "msg_test",
        type: "message",
        role: "assistant",
        model: "claude-sonnet-5-5",
        content: [],
        stop_reason: null,
        usage: { input_tokens: u.input, output_tokens: 1 },
      },
    }) +
    sseEvent("content_block_start", { index: 0, content_block: { type: "text", text: "" } }) +
    "event: ping\ndata: {\"type\": \"ping\"}\n\n" +
    sseEvent("content_block_delta", { index: 0, delta: { type: "text_delta", text } }) +
    sseEvent("content_block_stop", { index: 0 }) +
    sseEvent("message_delta", { delta: { stop_reason: "end_turn" }, usage: { output_tokens: u.output } }) +
    sseEvent("message_stop", {})
  );
}

export interface UpstreamCall {
  url: string;
  headers: Headers;
  body: any;
}

/**
 * Replaces global fetch for upstream calls. `reply` builds the response per call. Returns the
 * recorded calls. Only requests to anthropic.test are intercepted; anything else fails the test.
 */
export function mockAnthropic(reply: (call: UpstreamCall) => Response | Promise<Response> = () => sseResponse(cannedSse())) {
  const calls: UpstreamCall[] = [];
  const spy = vi.spyOn(globalThis, "fetch").mockImplementation(async (input: RequestInfo | URL, init?: RequestInit) => {
    const req = new Request(input as RequestInfo, init);
    if (!req.url.startsWith("https://anthropic.test/")) throw new Error(`unexpected fetch ${req.url}`);
    const c: UpstreamCall = { url: req.url, headers: req.headers, body: await req.json() };
    calls.push(c);
    return reply(c);
  });
  return { calls, spy };
}

export function sseResponse(body: string | ReadableStream<Uint8Array>, status = 200): Response {
  return new Response(body, { status, headers: { "content-type": "text/event-stream" } });
}

export function solveBody(images = [{ media_type: "image/jpeg", data: JPEG_B64 }], tutor?: boolean) {
  return tutor === undefined ? { images } : { images, tutor };
}

export async function solve(d: Device, body: unknown = solveBody(), over: Partial<Env> = {}) {
  return call("/v1/solve", { body, headers: deviceHeaders(d), env: over });
}

export async function dbFirst<T = Record<string, unknown>>(sql: string, ...args: unknown[]): Promise<T | null> {
  return env.DB.prepare(sql).bind(...args).first<T>();
}

export async function dbRun(sql: string, ...args: unknown[]) {
  return env.DB.prepare(sql).bind(...args).run();
}

export function nowSec() {
  return Math.floor(Date.now() / 1000);
}
