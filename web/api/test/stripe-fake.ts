// A tiny in-memory Stripe for the unit tests: intercepts fetch to https://stripe.test and
// answers the calls the Worker makes. It records every call so tests can check the
// parameters (form-encoded, decoded here back into a flat key -> value map).
//
// The Worker re-reads every subscription from Stripe when a webhook arrives (S3-4), so the
// fake keeps a subscription store: `seed()` puts one in, and the tests' webhook helper seeds
// `data.object` automatically when it is a subscription (see seedFromEvent).

import { vi } from "vitest";
import { signWebhook } from "../src/stripe";

export const STRIPE_BASE = "https://stripe.test";
export const WEBHOOK_SECRET = "whsec_unitTestSecret0123456789abcdef";

export interface StripeCall {
  method: string;
  path: string;
  params: Record<string, string>;
  headers: Record<string, string>;
}

export interface FakeStripe {
  calls: StripeCall[];
  subscriptions: Map<string, Record<string, any>>;
  /** Customer ids deleted with DELETE /v1/customers/{id}. */
  deletedCustomers: Set<string>;
  /** Store (a copy of) a subscription as Stripe has it now; returns the stored object. */
  seed: (sub: Record<string, any>) => Record<string, any>;
  /** Make the next matching call (optionally only this HTTP method) fail with this status. */
  failNext: (pathPrefix: string, status?: number, method?: string) => void;
  /** Runs before each Stripe call is answered (to simulate something happening meanwhile). */
  beforeAnswer?: (call: StripeCall) => void | Promise<void>;
}

let seq = 0;
const nextId = (p: string) => `${p}_${(++seq).toString(36).padStart(8, "0")}`;
const missing = () => Response.json({ error: { type: "invalid_request_error", code: "resource_missing" } }, { status: 404 });

/** Stripe's list default leaves out canceled subscriptions; status=all includes them. */
function listMatches(sub: Record<string, any>, status: string | undefined): boolean {
  if (!status) return sub.status !== "canceled" && sub.status !== "incomplete_expired";
  return status === "all" || sub.status === status;
}

let current: FakeStripe | null = null;

export function fakeStripe(passthrough?: (input: RequestInfo | URL, init?: RequestInit) => Promise<Response>): FakeStripe {
  const calls: StripeCall[] = [];
  const subscriptions = new Map<string, Record<string, any>>();
  const deletedCustomers = new Set<string>();
  const customersByKey = new Map<string, string>();
  const failures: { prefix: string; status: number; method?: string }[] = [];
  const realFetch = globalThis.fetch;

  vi.spyOn(globalThis, "fetch").mockImplementation(async (input: RequestInfo | URL, init?: RequestInit) => {
    const url = new URL(typeof input === "string" ? input : input instanceof URL ? input.href : input.url);
    if (url.origin !== STRIPE_BASE) {
      if (passthrough) return passthrough(input, init);
      return realFetch(input as RequestInfo, init);
    }
    const method = (init?.method ?? "GET").toUpperCase();
    const headers: Record<string, string> = {};
    new Headers(init?.headers).forEach((v, k) => (headers[k] = v));
    const raw = method === "GET" ? url.search.slice(1) : String(init?.body ?? "");
    const params = Object.fromEntries(new URLSearchParams(raw));
    const thisCall = { method, path: url.pathname, params, headers };
    calls.push(thisCall);
    if (fake.beforeAnswer) await fake.beforeAnswer(thisCall);

    const fail = failures.findIndex((f) => url.pathname.startsWith(f.prefix) && (!f.method || f.method === method));
    if (fail >= 0) {
      const { status } = failures.splice(fail, 1)[0]!;
      return Response.json({ error: { type: "api_error", code: "fake_failure", message: "boom" } }, { status });
    }
    if (headers.authorization !== "Bearer sk_test_unitTestKey0123456789") {
      return Response.json({ error: { type: "invalid_request_error", code: "unauthorized" } }, { status: 401 });
    }

    if (method === "POST" && url.pathname === "/v1/customers") {
      const key = headers["idempotency-key"] ?? nextId("k");
      let id = customersByKey.get(key);
      if (!id) customersByKey.set(key, (id = nextId("cus")));
      return Response.json({ id, object: "customer", email: params.email });
    }
    const cusMatch = /^\/v1\/customers\/([^/]+)$/.exec(url.pathname);
    if (cusMatch && method === "DELETE") {
      const id = decodeURIComponent(cusMatch[1]!);
      if (deletedCustomers.has(id) || !/^cus_/.test(id)) return missing();
      deletedCustomers.add(id);
      // Deleting a customer cancels all of its subscriptions immediately.
      for (const sub of subscriptions.values()) if (sub.customer === id) sub.status = "canceled";
      return Response.json({ id, object: "customer", deleted: true });
    }
    if (method === "POST" && url.pathname === "/v1/checkout/sessions") {
      const id = nextId("cs_test");
      return Response.json({
        id,
        object: "checkout.session",
        mode: params.mode,
        expires_at: params.expires_at ? Number(params.expires_at) : undefined,
        url: `https://checkout.stripe.com/c/pay/${id}`,
      });
    }
    if (method === "POST" && url.pathname === "/v1/billing_portal/sessions") {
      return Response.json({ id: nextId("bps"), url: `https://billing.stripe.com/p/session/${nextId("test")}` });
    }
    if (method === "GET" && url.pathname === "/v1/subscriptions") {
      const limit = Math.min(100, Number(params.limit ?? 10));
      const data = [...subscriptions.values()]
        .filter((s) => (!params.customer || s.customer === params.customer) && listMatches(s, params.status))
        .slice(0, limit);
      return Response.json({ object: "list", url: "/v1/subscriptions", has_more: false, data });
    }
    const subMatch = /^\/v1\/subscriptions\/([^/]+)$/.exec(url.pathname);
    if (subMatch) {
      const sub = subscriptions.get(decodeURIComponent(subMatch[1]!));
      if (!sub) return missing();
      if (method === "DELETE") sub.status = "canceled";
      return Response.json(sub);
    }
    return Response.json({ error: { type: "invalid_request_error", code: "unknown_path" } }, { status: 404 });
  });

  const fake: FakeStripe = {
    calls,
    subscriptions,
    deletedCustomers,
    seed: (sub) => {
      const copy = structuredClone(sub);
      subscriptions.set(copy.id, copy);
      return copy;
    },
    failNext: (prefix, status = 500, method) => failures.push({ prefix, status, method: method?.toUpperCase() }),
  };
  current = fake;
  return fake;
}

/**
 * Makes Stripe's current state match a subscription event's object, the way it would be on
 * the real Stripe when the event is sent. No-op without an active fake or for other objects.
 */
export function seedFromEvent(ev: { data?: { object?: Record<string, any> } }) {
  const o = ev.data?.object;
  if (current && o?.object === "subscription" && typeof o.id === "string") current.seed(o);
}

/** A subscription object in the newer ("basil" and later) shape: period end on the items. */
export function subscription(customer: string, status: string, over: Record<string, any> = {}) {
  const now = Math.floor(Date.now() / 1000);
  return {
    id: over.id ?? nextId("sub"),
    object: "subscription",
    customer,
    status,
    cancel_at_period_end: false,
    cancel_at: null,
    items: { data: [{ id: nextId("si"), price: { id: "price_subMonthly15" }, current_period_end: now + 30 * 86400 }] },
    metadata: {},
    ...over,
  };
}

let evSeq = 0;
export function event(type: string, object: Record<string, any>, created = Math.floor(Date.now() / 1000), extra: Record<string, any> = {}) {
  return { id: `evt_test${(++evSeq).toString(36).padStart(10, "0")}`, object: "event", type, created, livemode: false, ...extra, data: { object } };
}

/** Body + Stripe-Signature header for an event, signed like Stripe signs it. */
export async function signed(ev: object, t = Math.floor(Date.now() / 1000), secret = WEBHOOK_SECRET) {
  const body = JSON.stringify(ev);
  return { body, signature: await signWebhook(body, secret, t) };
}
