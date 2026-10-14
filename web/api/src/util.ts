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
    if (dev) return ipKey(dev.slice(0, 64));
  }
  return ipKey(req.headers.get("cf-connecting-ip")?.trim() || "unknown");
}

/**
 * Rate-limit key for an address (R2-1): IPv4 as is, IPv6 by its /64 prefix (one customer
 * usually owns a whole /64, so per-address limits would be trivial to dodge).
 */
export function ipKey(ip: string): string {
  let a = ip.trim().toLowerCase();
  if (!a.includes(":")) return a;
  a = a.replace(/^\[|\]$/g, "").split("%")[0]!;
  const v4 = /^(?:0{0,4}:){0,5}(?:ffff:)?(\d{1,3}(?:\.\d{1,3}){3})$/.exec(a.replace(/^::/, ""));
  if (v4 && a.startsWith("::")) return v4[1]!; // IPv4-mapped / compatible
  const parts = a.split("::");
  if (parts.length > 2) return a;
  const head = parts[0] ? parts[0].split(":") : [];
  const tail = parts.length === 2 && parts[1] ? parts[1].split(":") : [];
  if (tail.length && tail[tail.length - 1]!.includes(".")) return a; // embedded IPv4: keep whole
  const fill = parts.length === 2 ? 8 - head.length - tail.length : 0;
  if (fill < 0) return a;
  const groups = [...head, ...new Array(fill).fill("0"), ...tail];
  if (groups.length !== 8 || !groups.every((g) => /^[0-9a-f]{1,4}$/.test(g))) return a;
  return groups.slice(0, 4).map((g) => parseInt(g, 16).toString(16)).join(":") + "::/64";
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
  const retryAfter = Math.max(1, start + windowSec - now);
  // S4: once over the limit, stop writing (a flood of refused requests costs only reads).
  const cur = await db
    .prepare(`SELECT count FROM rate_limits WHERE key = ?1 AND window_start = ?2`)
    .bind(key, start)
    .first<{ count: number }>();
  if (cur && cur.count >= limit) return { ok: false, retryAfter };
  const row = await db
    .prepare(
      `INSERT INTO rate_limits (key, window_start, count) VALUES (?1, ?2, 1)
       ON CONFLICT (key, window_start) DO UPDATE SET count = count + 1
       RETURNING count`,
    )
    .bind(key, start)
    .first<{ count: number }>();
  const count = row?.count ?? Number.MAX_SAFE_INTEGER;
  return { ok: count <= limit, retryAfter };
}

/** Reads a counter without writing (for global limits checked before any write). */
export async function peekRateLimit(
  db: D1Database,
  key: string,
  windowSec: number,
  limit: number,
  now = nowSec(),
): Promise<RateResult> {
  const start = Math.floor(now / windowSec) * windowSec;
  const cur = await db
    .prepare(`SELECT count FROM rate_limits WHERE key = ?1 AND window_start = ?2`)
    .bind(key, start)
    .first<{ count: number }>();
  return { ok: (cur?.count ?? 0) < limit, retryAfter: Math.max(1, start + windowSec - now) };
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
