// Small shared helpers: time, request body reading, rate limits.

import { HttpError, MSG } from "./http";

export function nowSec(): number {
  return Math.floor(Date.now() / 1000);
}

/** 'YYYY-MM-DD' in UTC. */
export function dayOf(t: number): string {
  return new Date(t * 1000).toISOString().slice(0, 10);
}

/** 'YYYY-MM' in UTC (monthly limits reset on the 1st, UTC). */
export function monthOf(t: number): string {
  return new Date(t * 1000).toISOString().slice(0, 7);
}

/**
 * The caller's IP for rate limits. In DEV_MODE on localhost only, the black-box tests may set
 * `x-dev-client-ip` to simulate different clients; it is ignored everywhere else.
 */
export function clientIp(req: Request, devAllowed: boolean): string {
  if (devAllowed) {
    const dev = req.headers.get("x-dev-client-ip")?.trim();
    if (dev) return dev.slice(0, 64);
  }
  return req.headers.get("cf-connecting-ip")?.trim() || "unknown";
}

/**
 * Reads the request body as bytes, refusing more than `limit` bytes. Checks Content-Length
 * first, then counts the real bytes (Content-Length can lie or be missing).
 */
export async function readBody(req: Request, limit: number, tooLarge: HttpError): Promise<Uint8Array> {
  const cl = req.headers.get("content-length");
  if (cl !== null) {
    const n = Number(cl);
    if (!Number.isFinite(n) || n < 0) throw new HttpError(400, "bad_request", MSG.badRequest);
    if (n > limit) throw tooLarge;
  }
  if (!req.body) return new Uint8Array(0);
  const reader = req.body.getReader();
  const chunks: Uint8Array[] = [];
  let total = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    total += value.byteLength;
    if (total > limit) {
      await reader.cancel().catch(() => {});
      throw tooLarge;
    }
    chunks.push(value);
  }
  const out = new Uint8Array(total);
  let off = 0;
  for (const c of chunks) {
    out.set(c, off);
    off += c.byteLength;
  }
  return out;
}

/** Reads and parses a JSON body (an object). Empty body -> {} when `allowEmpty`. */
export async function readJsonObject(
  req: Request,
  limit: number,
  allowEmpty = false,
): Promise<Record<string, unknown>> {
  const bytes = await readBody(req, limit, new HttpError(413, "payload_too_large", "Request too large."));
  if (bytes.byteLength === 0 && allowEmpty) return {};
  let v: unknown;
  try {
    v = JSON.parse(new TextDecoder().decode(bytes));
  } catch {
    throw new HttpError(400, "bad_request", "The request body is not valid JSON.");
  }
  if (typeof v !== "object" || v === null || Array.isArray(v)) {
    throw new HttpError(400, "bad_request", "The request body must be a JSON object.");
  }
  return v as Record<string, unknown>;
}

/** additionalProperties: false */
export function onlyKeys(o: Record<string, unknown>, allowed: string[]): void {
  for (const k of Object.keys(o)) {
    if (!allowed.includes(k)) throw new HttpError(400, "bad_request", `Unknown field: ${k.slice(0, 40)}`);
  }
}

export interface RateResult {
  ok: boolean;
  retryAfter: number;
}

/**
 * Fixed-window counter in D1. Increments and returns whether the caller is within `limit`.
 * A DB error propagates (callers fail closed).
 */
export async function hitRateLimit(
  db: D1Database,
  key: string,
  windowSec: number,
  limit: number,
  now = nowSec(),
): Promise<RateResult> {
  const start = Math.floor(now / windowSec) * windowSec;
  const row = await db
    .prepare(
      `INSERT INTO rate_limits (key, window_start, count) VALUES (?1, ?2, 1)
       ON CONFLICT (key, window_start) DO UPDATE SET count = count + 1
       RETURNING count`,
    )
    .bind(key, start)
    .first<{ count: number }>();
  const count = row?.count ?? Number.MAX_SAFE_INTEGER;
  return { ok: count <= limit, retryAfter: Math.max(1, start + windowSec - now) };
}

export function rateLimited(r: RateResult, message: string = MSG.rateLimited): HttpError {
  return new HttpError(429, "rate_limited", message, { "retry-after": String(r.retryAfter) });
}

const EMAIL_RE = /^[^\s@<>()[\]\\,;:"]{1,64}@[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?(?:\.[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?)+$/;

export function isEmail(v: unknown): v is string {
  return typeof v === "string" && v.length <= 254 && EMAIL_RE.test(v);
}

export const LABEL_RE = /^[^<>\x00-\x1f]+$/;

export function isLabel(v: unknown): v is string {
  if (typeof v !== "string" || v.length > 200) return false;
  const n = [...v].length; // JSON Schema counts code points
  return n >= 1 && n <= 40 && LABEL_RE.test(v);
}
