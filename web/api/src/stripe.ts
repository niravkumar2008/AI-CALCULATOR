// A small Stripe client: form-encoded REST calls with fetch, and webhook signature checks.
//
// Why not the official `stripe` package (PLAN section 2 lists it): the Worker runs on the free
// plan's 10 ms CPU budget, and this layer needs only five calls plus the signature check. A
// plain fetch client starts instantly, is easy to point at a local mock in tests, and can't
// drift with SDK upgrades. The signature check is the documented Stripe scheme (HMAC-SHA256
// of "<timestamp>.<raw body>", constant-time compare, 5-minute tolerance), i.e. what the SDK's
// constructEventAsync does.

import type { StripeConfig } from "./env";
import { hmacHex, safeEqual } from "./crypto";

export const WEBHOOK_TOLERANCE_SEC = 300;
const TIMEOUT_MS = 10_000;

export class StripeError extends Error {
  constructor(
    readonly status: number,
    readonly code: string,
    message: string,
  ) {
    super(message);
  }
}

type Param = string | number | boolean | null | undefined | Param[] | { [k: string]: Param };

/** Stripe's bracket encoding: {a: {b: [1]}} -> a[b][0]=1. Undefined/null values are skipped. */
export function formEncode(params: Record<string, Param>): string {
  const out: string[] = [];
  const walk = (key: string, v: Param) => {
    if (v === undefined || v === null) return;
    if (Array.isArray(v)) v.forEach((x, i) => walk(`${key}[${i}]`, x));
    else if (typeof v === "object") for (const [k, x] of Object.entries(v)) walk(`${key}[${k}]`, x);
    else out.push(`${encodeURIComponent(key)}=${encodeURIComponent(String(v))}`);
  };
  for (const [k, v] of Object.entries(params)) walk(k, v);
  return out.join("&");
}

/**
 * One Stripe API call. Throws StripeError for any non-2xx answer or network failure; the
 * message is for logs only (callers answer users with their own fixed text).
 */
export async function stripeCall<T = Record<string, any>>(
  s: StripeConfig,
  method: "GET" | "POST" | "DELETE",
  path: string,
  params: Record<string, Param> = {},
  idempotencyKey?: string,
): Promise<T> {
  if (!s.secretKey) throw new StripeError(0, "not_configured", "STRIPE_SECRET_KEY is not set");
  const headers: Record<string, string> = { authorization: `Bearer ${s.secretKey}` };
  // Always set in production (config refuses a key without STRIPE_API_VERSION unless DEV_MODE).
  if (s.apiVersion) headers["stripe-version"] = s.apiVersion;
  let url = s.apiBase + path;
  let body: string | undefined;
  const encoded = formEncode(params);
  if (method === "GET") {
    if (encoded) url += `?${encoded}`;
  } else {
    body = encoded;
    headers["content-type"] = "application/x-www-form-urlencoded";
    if (idempotencyKey) headers["idempotency-key"] = idempotencyKey;
  }
  let res: Response;
  try {
    res = await fetch(url, { method, headers, body, signal: AbortSignal.timeout(TIMEOUT_MS) });
  } catch (e) {
    throw new StripeError(0, "network", `stripe ${method} ${path}: ${String(e).slice(0, 120)}`);
  }
  const text = await res.text();
  let json: any = null;
  try {
    json = JSON.parse(text);
  } catch {
    /* handled below */
  }
  if (!res.ok || json === null || typeof json !== "object") {
    const code = String(json?.error?.code ?? json?.error?.type ?? "http_" + res.status);
    throw new StripeError(res.status, code, `stripe ${method} ${path} -> ${res.status} ${code}`);
  }
  return json as T;
}

/** "cus_123" or an expanded {id: "cus_123"} -> "cus_123". */
export function idOf(v: unknown): string | null {
  if (typeof v === "string" && v) return v;
  if (v && typeof v === "object" && typeof (v as { id?: unknown }).id === "string") return (v as { id: string }).id;
  return null;
}

/**
 * The period end of a subscription. API versions before 2025-03-31 ("basil") put it on the
 * subscription; newer ones only on its items. Read both, take the latest item period.
 */
export function periodEndOf(sub: Record<string, any>): number | null {
  if (typeof sub.current_period_end === "number") return sub.current_period_end;
  const items: unknown[] = Array.isArray(sub.items?.data) ? sub.items.data : [];
  let best: number | null = null;
  for (const it of items) {
    const v = (it as { current_period_end?: unknown })?.current_period_end;
    if (typeof v === "number" && (best === null || v > best)) best = v;
  }
  return best;
}

/** The subscription id on an invoice, old shape (`subscription`) or new (`parent...`). */
export function invoiceSubscriptionId(inv: Record<string, any>): string | null {
  return idOf(inv.subscription) ?? idOf(inv.parent?.subscription_details?.subscription);
}

// ------------------------------------------------------------ webhook signatures

export interface VerifiedEvent {
  id: string;
  type: string;
  created: number;
  data: { object: Record<string, any> };
}

/**
 * Checks the Stripe-Signature header against the raw body and returns the parsed event, or
 * null when anything is wrong (missing header, bad format, no matching v1 signature, or a
 * timestamp more than 5 minutes from now). Never parses the body before the check passes.
 */
export async function verifyWebhook(
  rawBody: string,
  header: string | null,
  secret: string,
  now: number,
): Promise<VerifiedEvent | null> {
  if (!header || !secret || header.length > 4096) return null;
  let t: number | null = null;
  const sigs: string[] = [];
  for (const part of header.split(",")) {
    const i = part.indexOf("=");
    if (i < 1) continue;
    const k = part.slice(0, i).trim();
    const v = part.slice(i + 1).trim();
    if (k === "t" && /^\d{1,12}$/.test(v)) t = Number(v);
    else if (k === "v1" && /^[0-9a-f]{64}$/.test(v)) sigs.push(v);
  }
  if (t === null || sigs.length === 0 || sigs.length > 8) return null;
  if (Math.abs(now - t) > WEBHOOK_TOLERANCE_SEC) return null;
  const expected = await hmacHex(secret, `${t}.${rawBody}`);
  let ok = false;
  for (const s of sigs) if (await safeEqual(s, expected)) ok = true; // check all: no early exit
  if (!ok) return null;

  let ev: any;
  try {
    ev = JSON.parse(rawBody);
  } catch {
    return null;
  }
  if (
    !ev ||
    typeof ev.id !== "string" ||
    !/^evt_[A-Za-z0-9]+$/.test(ev.id) ||
    typeof ev.type !== "string" ||
    typeof ev.created !== "number" ||
    typeof ev.data?.object !== "object" ||
    ev.data.object === null
  ) {
    return null;
  }
  return ev as VerifiedEvent;
}

/** Test helper and documentation of the scheme: the header Stripe would send. */
export async function signWebhook(rawBody: string, secret: string, t: number): Promise<string> {
  return `t=${t},v1=${await hmacHex(secret, `${t}.${rawBody}`)}`;
}
