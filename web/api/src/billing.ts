// Stage 3: Stripe Checkout (subscription and the calculator), the Customer Portal, the signed
// webhook, and closing the Stripe customer when an account is deleted. Contract: openapi.yaml
// tags billing / store / webhooks. Plan: PLAN.md sections 2, 3 and 7 (D2, D3).
//
// Rules this file keeps:
// - Our database only learns about money from verified webhooks plus a fresh read from Stripe,
//   never from the browser coming back from Checkout.
// - Subscription state is always the subscription as Stripe has it *now* (re-read on every
//   subscription or invoice event), mapped to an account by the Stripe customer id we created.
//   Metadata is never trusted, and the order events arrive in no longer matters (S3-4).
// - Webhooks are idempotent on the event id: the event is recorded in the same transaction as
//   its effects.
// - Any database or Stripe failure answers 503, so Stripe retries the event later.
// - Deleting an account deletes its Stripe customer first, which cancels every subscription
//   and makes any still-open Checkout page for it fail (S3-2).

import { type Config, type Env, type StripeConfig, planPrices } from "./env";
import { HttpError, MSG, json } from "./http";
import { requireBrowserWrite, requireUser } from "./auth";
import type { UserRow } from "./plan";
import { devAllowed } from "./dev";
import { StripeError, idOf, invoiceSubscriptionId, periodEndOf, stripeCall, verifyWebhook } from "./stripe";
import { clientIp, hitRateLimit, onlyKeys, peekRateLimit, rateLimited, readBody, readJsonObject } from "./util";

const BODY_LIMIT = 4 * 1024;
const WEBHOOK_BODY_LIMIT = 512 * 1024;
const MAX_DEVICES_PER_ORDER = 3;
/** Stripe's minimum Checkout lifetime is 30 minutes; keep sessions just above it (S3-3). */
const CHECKOUT_TTL_SEC = 31 * 60;
/** Statuses that mean "a subscription already exists or is being paid": no second checkout. */
const BLOCKING = new Set(["trialing", "active", "past_due", "unpaid", "incomplete"]);
/** Statuses that still bill or may bill: cancelled when the account is gone. */
const LIVE = new Set(["trialing", "active", "past_due", "unpaid", "incomplete", "paused"]);

const MSG_PAY = {
  unavailable: "Payments aren't available right now. Please try again later.",
  soldOut: "Preorders open in 2027. Join the waitlist to hear first.",
  alreadySubscribed: "You already have a subscription. Use Manage billing to change it.",
  noCustomer: "There's no billing account yet. Subscribe first.",
  quantity: "Choose 1, 2 or 3 calculators.",
  interval: 'interval must be "month" or "year".',
  noYearly: "Yearly billing isn't offered right now. Choose monthly.",
  storeBusy: "The store is busy right now. Please try again in a few minutes.",
};

function needStripe(cfg: Config, price: "priceSub" | "priceDevice"): StripeConfig {
  const s = cfg.stripe;
  if (!s.secretKey || !s[price]) throw new HttpError(503, "unavailable", MSG_PAY.unavailable);
  return s;
}

/** Logs the Stripe failure (no secrets in it) and answers with a plain 503. */
function stripeFailed(e: unknown): HttpError {
  if (e instanceof HttpError) return e;
  const detail = e instanceof StripeError ? `${e.status} ${e.code}` : String(e).slice(0, 200);
  console.error(JSON.stringify({ event: "stripe_error", detail }));
  return new HttpError(503, "unavailable", MSG_PAY.unavailable);
}

/** "I'm 18+" and Terms consent shown on both Checkout pages (PLAN section 9). */
function consentParams(cfg: Config) {
  if (!cfg.stripe.requireTos) return {};
  return {
    consent_collection: { terms_of_service: "required" },
    custom_text: {
      terms_of_service_acceptance: {
        message: `I am 18 or older and agree to the [Terms of service](${cfg.publicUrl}/terms/).`,
      },
    },
  };
}

function taxParams(cfg: Config, withCustomer: boolean) {
  if (!cfg.stripe.tax) return {};
  return withCustomer
    ? { automatic_tax: { enabled: true }, customer_update: { address: "auto", name: "auto" } }
    : { automatic_tax: { enabled: true } };
}

/** The account's Stripe customer, created once (an idempotency key stops double creation). */
async function ensureCustomer(env: Env, s: StripeConfig, user: UserRow): Promise<{ id: string; isNew: boolean }> {
  if (user.stripe_customer_id) return { id: user.stripe_customer_id, isNew: false };
  const c = await stripeCall<{ id: string }>(
    s,
    "POST",
    "/v1/customers",
    { email: user.email, metadata: { user_id: user.id } },
    `customer-${user.id}`,
  );
  if (!/^cus_[A-Za-z0-9]+$/.test(c.id ?? "")) throw new StripeError(502, "bad_customer", "bad customer id");
  // S4-4: never attach a customer to an account deleted while this call was at Stripe.
  await env.DB.prepare(
    `UPDATE users SET stripe_customer_id = ?1 WHERE id = ?2 AND stripe_customer_id IS NULL AND deleted_at IS NULL`,
  )
    .bind(c.id, user.id)
    .run();
  const row = await env.DB.prepare(`SELECT stripe_customer_id, deleted_at FROM users WHERE id = ?1`)
    .bind(user.id)
    .first<{ stripe_customer_id: string | null; deleted_at: number | null }>();
  if (!row || row.deleted_at !== null) {
    // The account is gone: remove the customer we just made (404 = already gone) and stop.
    try {
      await stripeCall(s, "DELETE", `/v1/customers/${encodeURIComponent(c.id)}`);
    } catch (e) {
      if (!(e instanceof StripeError && e.status === 404)) throw e;
    }
    throw new HttpError(401, "unauthorized", MSG.signIn);
  }
  if (!row.stripe_customer_id) throw new Error("customer link vanished");
  // "New" only if this call's customer is the one stored (a parallel call may have won).
  return { id: row.stripe_customer_id, isNew: row.stripe_customer_id === c.id };
}

function checkoutUrl(session: { url?: unknown }, host: RegExp, cfg: Config): string {
  const url = typeof session.url === "string" ? session.url : "";
  let u: URL;
  try {
    u = new URL(url);
  } catch {
    throw new StripeError(502, "bad_url", "Stripe returned no usable URL");
  }
  // Only ever send a browser to Stripe's own pages. Local development (DEV_MODE, which only
  // runs on localhost) may also use the fake Stripe's pages at exactly STRIPE_API_BASE.
  const stripePage = u.protocol === "https:" && host.test(u.hostname);
  const localFake = cfg.devMode && u.origin === cfg.stripe.apiBase && cfg.stripe.apiBase.startsWith("http://127.0.0.1");
  if (!stripePage && !localFake) throw new StripeError(502, "bad_url", "unexpected URL host");
  return url;
}

// ------------------------------------------------------------ POST /api/billing/checkout
export async function billingCheckout(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  requireBrowserWrite(req, cfg);
  const user = await requireUser(req, env, now);
  // {"interval": "month" | "year"}; an empty body (or no interval) means monthly.
  const body = await readJsonObject(req, BODY_LIMIT, true);
  onlyKeys(body, ["interval"]);
  const interval = body.interval === undefined ? "month" : body.interval;
  if (interval !== "month" && interval !== "year") throw new HttpError(400, "bad_request", MSG_PAY.interval);
  const rl = await hitRateLimit(env.DB, `checkout:user:${user.id}`, 600, 10, now);
  if (!rl.ok) throw rateLimited(rl);
  const s = needStripe(cfg, "priceSub");
  // Not a server fault: this plan simply isn't sold yearly (yet). 400, so the site shows it.
  if (interval === "year" && !s.priceSubYearly) throw new HttpError(400, "bad_request", MSG_PAY.noYearly);
  const price = interval === "year" ? s.priceSubYearly : s.priceSub;
  // Monthly and yearly are one plan: a subscription on either blocks a second one.
  const prices = planPrices(s);

  const live = await env.DB.prepare(
    `SELECT 1 AS x FROM subscriptions
     WHERE user_id = ?1 AND status IN ('trialing','active','past_due','unpaid','incomplete')
       AND price_id IN (?2, ?3) LIMIT 1`,
  )
    .bind(user.id, prices[0]!, prices[1] ?? prices[0]!)
    .first();
  if (live) throw new HttpError(409, "already_subscribed", MSG_PAY.alreadySubscribed);

  try {
    const customer = await ensureCustomer(env, s, user);
    if (!customer.isNew) {
      // S3-3: ask Stripe too. A subscription paid in another tab may not have reached us yet.
      const list = await stripeCall<{ data?: Record<string, any>[] }>(s, "GET", "/v1/subscriptions", {
        customer: customer.id,
        status: "all",
        limit: 20,
      });
      // Only our AI tutor plan counts, monthly or yearly (same rule as entitlement, S3-5).
      const ours = (x: Record<string, any>) => prices.includes(idOf(x.items?.data?.[0]?.price) ?? "");
      if ((list.data ?? []).some((x) => BLOCKING.has(String(x.status)) && ours(x))) {
        throw new HttpError(409, "already_subscribed", MSG_PAY.alreadySubscribed);
      }
    }
    const session = await stripeCall(s, "POST", "/v1/checkout/sessions", {
      mode: "subscription",
      customer: customer.id,
      client_reference_id: user.id,
      line_items: [{ price, quantity: 1 }],
      // Decision D2: no Stripe trial. Our own free month already ran (or never applied).
      subscription_data: { metadata: { user_id: user.id } },
      expires_at: now + CHECKOUT_TTL_SEC,
      success_url: `${cfg.publicUrl}/account/?checkout=done`,
      cancel_url: `${cfg.publicUrl}/account/?checkout=cancelled`,
      ...consentParams(cfg),
      ...taxParams(cfg, true),
    });
    return json({ url: checkoutUrl(session, /^checkout\.stripe\.com$/, cfg) });
  } catch (e) {
    throw stripeFailed(e);
  }
}

// ------------------------------------------------------------ POST /api/billing/portal
export async function billingPortal(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  requireBrowserWrite(req, cfg);
  const user = await requireUser(req, env, now);
  await readJsonObject(req, BODY_LIMIT, true);
  if (!cfg.stripe.secretKey) throw new HttpError(503, "unavailable", MSG_PAY.unavailable);
  if (!user.stripe_customer_id) throw new HttpError(409, "no_customer", MSG_PAY.noCustomer);
  const rl = await hitRateLimit(env.DB, `portal:user:${user.id}`, 600, 20, now);
  if (!rl.ok) throw rateLimited(rl);
  try {
    const session = await stripeCall(cfg.stripe, "POST", "/v1/billing_portal/sessions", {
      customer: user.stripe_customer_id,
      return_url: `${cfg.publicUrl}/account/`,
    });
    return json({ url: checkoutUrl(session, /^billing\.stripe\.com$/, cfg) });
  } catch (e) {
    throw stripeFailed(e);
  }
}

// ------------------------------------------------------------ POST /api/store/checkout
export async function storeCheckout(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  requireBrowserWrite(req, cfg);
  const body = await readJsonObject(req, BODY_LIMIT);
  onlyKeys(body, ["quantity"]);
  const q = body.quantity;
  if (typeof q !== "number" || !Number.isInteger(q) || q < 1 || q > MAX_DEVICES_PER_ORDER) {
    throw new HttpError(400, "bad_request", MSG_PAY.quantity);
  }
  if (!cfg.stripe.storeOpen) throw new HttpError(503, "sold_out", MSG_PAY.soldOut);
  // S3-9: a hard global hourly cap, then 10 per IP. S4-1: the global cap is checked first with a
  // read only and counted only for requests the per-IP limit lets through, so one address can't
  // use up everyone's hourly budget (same pattern as /v1/device/register). The counted step is
  // atomic, so a burst still can't overshoot the cap.
  const peek = await peekRateLimit(env.DB, "store:global", 3600, cfg.storeGlobalPerHour, now);
  if (!peek.ok) throw rateLimited(peek, MSG_PAY.storeBusy);
  const rl = await hitRateLimit(env.DB, `store:ip:${clientIp(req, devAllowed(req, cfg))}`, 3600, 10, now);
  if (!rl.ok) throw rateLimited(rl);
  const global = await hitRateLimit(env.DB, "store:global", 3600, cfg.storeGlobalPerHour, now);
  if (!global.ok) throw rateLimited(global, MSG_PAY.storeBusy);
  const s = needStripe(cfg, "priceDevice");
  try {
    const session = await stripeCall(s, "POST", "/v1/checkout/sessions", {
      mode: "payment",
      line_items: [{ price: s.priceDevice, quantity: q }],
      shipping_address_collection: { allowed_countries: ["US"] },
      customer_creation: "always",
      metadata: { kind: "device" },
      expires_at: now + CHECKOUT_TTL_SEC,
      success_url: `${cfg.publicUrl}/order/complete/`,
      cancel_url: `${cfg.publicUrl}/cart/?checkout=cancelled`,
      ...consentParams(cfg),
      ...taxParams(cfg, false),
    });
    return json({ url: checkoutUrl(session, /^checkout\.stripe\.com$/, cfg) });
  } catch (e) {
    throw stripeFailed(e);
  }
}

// ------------------------------------------------------------ GET /api/store/status
/** Whether the shop can take orders right now (the site shows "Preorder" or "Join the waitlist"). */
export async function storeStatus(_req: Request, _env: Env, cfg: Config): Promise<Response> {
  const open = cfg.stripe.storeOpen && Boolean(cfg.stripe.secretKey && cfg.stripe.priceDevice);
  return json({ open, max_quantity: MAX_DEVICES_PER_ORDER }, 200, { "cache-control": "public, max-age=60" });
}

// ------------------------------------------------------------ account deletion
/**
 * Stops all billing before an account is erased (S3-2): deletes the Stripe customer, which
 * cancels every subscription at once and makes any still-open Checkout page fail. Throws 503
 * (nothing erased) if Stripe can't be reached, so a deleted account can never keep paying.
 * Returns the local statements to run in the same batch as the erase.
 */
export async function closeAtStripe(env: Env, cfg: Config, user: UserRow, now: number): Promise<D1PreparedStatement[]> {
  const live = await env.DB.prepare(
    `SELECT COUNT(*) AS n FROM subscriptions WHERE user_id = ?1 AND status NOT IN ('canceled','incomplete_expired')`,
  )
    .bind(user.id)
    .first<{ n: number }>();
  if (user.stripe_customer_id) {
    if (!cfg.stripe.secretKey) {
      // Payments switched off: only safe to continue if nothing could be billing.
      if ((live?.n ?? 0) > 0) throw new HttpError(503, "unavailable", MSG_PAY.unavailable);
      console.warn(JSON.stringify({ event: "stripe_customer_left_open", user: user.id }));
    } else {
      try {
        await stripeCall(cfg.stripe, "DELETE", `/v1/customers/${encodeURIComponent(user.stripe_customer_id)}`);
      } catch (e) {
        // Already gone at Stripe is fine; anything else stops the deletion.
        if (!(e instanceof StripeError && e.status === 404)) throw stripeFailed(e);
      }
    }
  }
  return [
    env.DB.prepare(
      `UPDATE subscriptions SET status = 'canceled', cancel_at_period_end = 0, past_due_since = NULL,
         updated_at = MAX(updated_at, ?1)
       WHERE user_id = ?2 AND status NOT IN ('canceled','incomplete_expired')`,
    ).bind(now, user.id),
  ];
}

// ------------------------------------------------------------ POST /api/stripe/webhook
export async function stripeWebhook(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  const secret = cfg.stripe.webhookSecret;
  // Not configured yet: 503 makes Stripe keep retrying (for up to 3 days) instead of dropping.
  if (!secret) throw new HttpError(503, "unavailable", MSG.unavailable);
  const bytes = await readBody(req, WEBHOOK_BODY_LIMIT, new HttpError(413, "payload_too_large", "Request too large."));
  const raw = new TextDecoder().decode(bytes);
  const ev = await verifyWebhook(raw, req.headers.get("stripe-signature"), secret, now);
  if (!ev) throw new HttpError(400, "bad_signature", "Bad signature.");
  // S3-11: a live event on a test key (or the reverse) means a misconfigured endpoint.
  const livemode = (ev as { livemode?: unknown }).livemode;
  if (cfg.stripe.secretKey && typeof livemode === "boolean" && livemode !== /_live_/.test(cfg.stripe.secretKey)) {
    console.error(JSON.stringify({ event: "stripe_mode_mismatch", type: ev.type }));
    throw new HttpError(400, "bad_signature", "Bad signature.");
  }

  const seen = await env.DB.prepare(`SELECT 1 AS x FROM stripe_events WHERE id = ?1`).bind(ev.id).first();
  if (seen) return json({ received: true, duplicate: true });

  let writes: D1PreparedStatement[];
  try {
    writes = await writesFor(env, cfg, ev, now);
  } catch (e) {
    throw stripeFailed(e); // 503: Stripe retries
  }
  const mark = env.DB.prepare(`INSERT INTO stripe_events (id, type, received_at) VALUES (?1, ?2, ?3)`).bind(
    ev.id,
    ev.type.slice(0, 80),
    now,
  );
  try {
    // One transaction: the event is marked done only together with its effects.
    await env.DB.batch([mark, ...writes]);
  } catch (e) {
    // A concurrent delivery of the same event wins the insert: that's a duplicate, not an error.
    const again = await env.DB.prepare(`SELECT 1 AS x FROM stripe_events WHERE id = ?1`).bind(ev.id).first();
    if (again) return json({ received: true, duplicate: true });
    throw e; // -> 503 in index.ts; Stripe retries
  }
  return json({ received: true });
}

type Obj = Record<string, any>;

async function writesFor(env: Env, cfg: Config, ev: { type: string; data: { object: Obj } }, now: number) {
  const o = ev.data.object;
  switch (ev.type) {
    case "checkout.session.completed":
    case "checkout.session.async_payment_succeeded":
      return checkoutCompleted(env, cfg, o, now);
    case "checkout.session.async_payment_failed":
      return isDeviceOrder(o) ? [orderUpsert(env, o, now, "failed")] : [];
    case "customer.subscription.created":
    case "customer.subscription.updated":
    case "customer.subscription.deleted":
    case "customer.subscription.paused":
    case "customer.subscription.resumed":
      return refreshSubscription(env, cfg, idOf(o), now);
    case "invoice.paid":
    case "invoice.payment_succeeded":
    case "invoice.payment_failed":
      // The invoice says *that* something changed; the subscription says *what*.
      return refreshSubscription(env, cfg, invoiceSubscriptionId(o), now);
    default:
      return []; // unknown types: recorded as seen, nothing else
  }
}

/** Only sessions our store created (S3-6): Payment Links or other products aren't device orders. */
function isDeviceOrder(s: Obj): boolean {
  return s.mode === "payment" && s.metadata?.kind === "device" && typeof s.id === "string" && s.id.startsWith("cs_");
}

function orderUpsert(env: Env, s: Obj, now: number, statusOverride?: string): D1PreparedStatement {
  const shipping = s.collected_information?.shipping_details ?? s.shipping_details ?? null;
  const email = typeof s.customer_details?.email === "string" ? s.customer_details.email.slice(0, 254) : null;
  const status = statusOverride ?? (typeof s.payment_status === "string" ? s.payment_status : "unknown");
  // S3-6: paid and failed are final; a late 'unpaid' copy can't undo them.
  return env.DB.prepare(
    `INSERT INTO orders (id, email, user_id, amount_total, currency, status, shipping_json, created_at)
     VALUES (?1, ?2, NULL, ?3, ?4, ?5, ?6, ?7)
     ON CONFLICT (id) DO UPDATE SET status = excluded.status, email = COALESCE(excluded.email, orders.email),
       shipping_json = COALESCE(excluded.shipping_json, orders.shipping_json)
     WHERE orders.status NOT IN ('paid', 'failed')`,
  ).bind(
    String(s.id).slice(0, 255),
    email,
    Number.isInteger(s.amount_total) ? s.amount_total : null,
    typeof s.currency === "string" ? s.currency.slice(0, 8) : null,
    status.slice(0, 32),
    shipping ? JSON.stringify(shipping).slice(0, 4000) : null,
    now,
  );
}

async function checkoutCompleted(env: Env, cfg: Config, s: Obj, now: number): Promise<D1PreparedStatement[]> {
  if (isDeviceOrder(s)) return [orderUpsert(env, s, now)];
  if (s.mode !== "subscription") return [];
  return refreshSubscription(env, cfg, idOf(s.subscription), now);
}

/**
 * Re-reads one subscription from Stripe and stores it as it is now (S3-4). Maps it to an
 * account only by the Stripe customer id we created. If that account was deleted while the
 * subscription was still being set up, cancels it at Stripe (S3-2).
 */
async function refreshSubscription(env: Env, cfg: Config, subId: string | null, now: number): Promise<D1PreparedStatement[]> {
  if (!subId || !/^sub_[A-Za-z0-9]+$/.test(subId)) return [];
  const sub = await stripeCall(cfg.stripe, "GET", `/v1/subscriptions/${encodeURIComponent(subId)}`);
  const id = idOf(sub);
  const customer = idOf(sub.customer);
  if (id !== subId || !customer) return [];
  const user = await env.DB.prepare(`SELECT id, deleted_at FROM users WHERE stripe_customer_id = ?1`)
    .bind(customer)
    .first<{ id: string; deleted_at: number | null }>();
  if (!user) {
    console.warn(JSON.stringify({ event: "stripe_unknown_customer", sub: id }));
    return [];
  }
  let status = typeof sub.status === "string" ? sub.status.slice(0, 32) : "unknown";
  if (user.deleted_at !== null && LIVE.has(status)) {
    try {
      await stripeCall(cfg.stripe, "DELETE", `/v1/subscriptions/${encodeURIComponent(id)}`, {}, `cancel-${id}`);
    } catch (e) {
      if (!(e instanceof StripeError && e.status === 404)) throw e; // 503: retry until cancelled
    }
    status = "canceled";
  }
  const price = idOf(sub.items?.data?.[0]?.price) ?? null;
  const cancelAtEnd = sub.cancel_at_period_end === true || typeof sub.cancel_at === "number" ? 1 : 0;
  return [
    env.DB.prepare(
      `INSERT INTO subscriptions (id, user_id, status, price_id, current_period_end, cancel_at_period_end, updated_at, past_due_since)
       VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, CASE WHEN ?3 = 'past_due' THEN ?7 END)
       ON CONFLICT (id) DO UPDATE SET status = excluded.status, price_id = excluded.price_id,
         current_period_end = excluded.current_period_end, cancel_at_period_end = excluded.cancel_at_period_end,
         updated_at = excluded.updated_at,
         past_due_since = CASE WHEN excluded.status = 'past_due'
                               THEN COALESCE(subscriptions.past_due_since, excluded.past_due_since) END
       WHERE subscriptions.user_id = excluded.user_id`,
    ).bind(id, user.id, status, price, periodEndOf(sub), cancelAtEnd, now),
  ];
}
