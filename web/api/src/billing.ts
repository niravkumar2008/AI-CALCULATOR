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

/**
 * Stripe Tax. The plan checkout has an existing customer: Stripe Tax reads the address saved
 * on it, so Checkout must save what the buyer types (customer_update) and, review G7, ask for
 * the full billing address (a ZIP alone can't give US local rates). The store checkout has no
 * customer yet (customer_update is only valid with `customer`); tax uses its shipping address.
 */
function taxParams(cfg: Config, withCustomer: boolean) {
  if (!cfg.stripe.tax) return {};
  return withCustomer
    ? {
        automatic_tax: { enabled: true },
        billing_address_collection: "required",
        customer_update: { address: "auto", name: "auto" },
      }
    : { automatic_tax: { enabled: true } };
}

/**
 * A failed call that Stripe may have run, or stored, under its Idempotency-Key: a 5xx is
 * replayed for 24 hours for the same key, and after a timeout we can't know whether it ran.
 */
function mayHaveRun(e: unknown): boolean {
  return e instanceof StripeError && (e.status >= 500 || (e.status === 0 && e.code === "network"));
}

/** The idempotency key for an account's n-th customer-create attempt (review G9). */
export function customerKey(userId: string, attempt: number): string {
  return attempt > 0 ? `customer-${userId}-r${attempt}` : `customer-${userId}`;
}

/**
 * A customer an earlier, failed attempt may have created at Stripe after all (the 5xx or the
 * timeout came after the create ran). Our own customers carry metadata.user_id; listing by
 * e-mail is read-after-write consistent (search isn't). Deleted customers aren't listed.
 */
async function findOwnCustomer(s: StripeConfig, user: UserRow): Promise<{ id: string } | null> {
  const list = await stripeCall<{ data?: Record<string, any>[] }>(s, "GET", "/v1/customers", { email: user.email, limit: 20 });
  const c = (list.data ?? []).find((x) => x?.metadata?.user_id === user.id && x.deleted !== true && typeof x.id === "string");
  return c ? { id: c.id } : null;
}

/**
 * The account's Stripe customer, created once. The create carries an idempotency key, so a
 * double click or a retried request gets the same customer. Review G9: Stripe also replays a
 * 5xx for that key for 24 hours, so after a failure that may have reached Stripe we count an
 * attempt in D1 (`users.stripe_customer_attempt`); the next try first looks for a customer the
 * failed one may have created anyway, and only then creates one under a fresh key.
 */
async function ensureCustomer(env: Env, s: StripeConfig, user: UserRow): Promise<{ id: string; isNew: boolean }> {
  if (user.stripe_customer_id) return { id: user.stripe_customer_id, isNew: false };
  const attempt = Number(user.stripe_customer_attempt ?? 0) || 0;
  let c: { id: string } | null = null;
  let recovered = false;
  try {
    if (attempt > 0) recovered = (c = await findOwnCustomer(s, user)) !== null;
    c ??= await stripeCall<{ id: string }>(
      s,
      "POST",
      "/v1/customers",
      { email: user.email, metadata: { user_id: user.id } },
      customerKey(user.id, attempt),
    );
  } catch (e) {
    if (mayHaveRun(e)) {
      // Only the request that saw this attempt fail moves it on (parallel failures count once).
      await env.DB.prepare(
        `UPDATE users SET stripe_customer_attempt = stripe_customer_attempt + 1
         WHERE id = ?1 AND stripe_customer_id IS NULL AND stripe_customer_attempt = ?2`,
      )
        .bind(user.id, attempt)
        .run();
    }
    throw e;
  }
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
  // A recovered customer existed before this call, so it is checked like any existing one.
  return { id: row.stripe_customer_id, isNew: !recovered && row.stripe_customer_id === c.id };
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
      // Review G3: a dedicated configuration (only the plan's two prices, cancel at period
      // end, no e-mail edits), so later edits to the account default can't change it.
      configuration: cfg.stripe.portalConfig || undefined,
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
      // Review G7: carriers want a phone number (the contract always said "phone optional").
      phone_number_collection: { enabled: true },
      // A shipping rate (shr_..., with the shipping tax code) adds the delivery line, and lets
      // Stripe Tax decide tax on shipping. Unset: no delivery line.
      shipping_options: s.shippingRate ? [{ shipping_rate: s.shippingRate }] : undefined,
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
         payment_action_at = NULL, payment_action_url = NULL, updated_at = MAX(updated_at, ?1)
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
    case "invoice.payment_action_required":
      // Review G5: a renewal needs the customer to confirm it (3-D Secure). Same re-read, plus
      // the invoice's page where they confirm it, so the account can say "Confirm your payment".
      return refreshSubscription(env, cfg, invoiceSubscriptionId(o), now, {
        url: hostedInvoiceUrl(o.hosted_invoice_url, cfg),
      });
    case "customer.deleted":
      return customerDeleted(env, o, now);
    case "charge.refunded":
      return present(chargeRefunded(env, o));
    case "charge.dispute.created":
    case "charge.dispute.closed":
      return present(disputeChanged(env, ev.type, o));
    default:
      return []; // unknown types: recorded as seen, nothing else
  }
}

/** Only sessions our store created (S3-6): Payment Links or other products aren't device orders. */
function isDeviceOrder(s: Obj): boolean {
  return s.mode === "payment" && s.metadata?.kind === "device" && typeof s.id === "string" && s.id.startsWith("cs_");
}

const present = (x: D1PreparedStatement | null): D1PreparedStatement[] => (x ? [x] : []);

/**
 * Order statuses after the money moved (S3-6, review G6). Once an order is in one of these, a
 * Checkout Session event never changes its status again; only refund and dispute events move
 * it, and only forward:
 *   paid -> partially_refunded -> refunded
 *   paid | partially_refunded | refunded -> disputed -> dispute_won | dispute_lost
 * `failed` is final too.
 */
const SETTLED_ORDER = "'paid','failed','partially_refunded','refunded','disputed','dispute_won','dispute_lost'";
const PI_RE = /^pi_[A-Za-z0-9]+$/;

function orderUpsert(env: Env, s: Obj, now: number, statusOverride?: string): D1PreparedStatement {
  const shipping = s.collected_information?.shipping_details ?? s.shipping_details ?? null;
  const email = typeof s.customer_details?.email === "string" ? s.customer_details.email.slice(0, 254) : null;
  const status = statusOverride ?? (typeof s.payment_status === "string" ? s.payment_status : "unknown");
  const pi = idOf(s.payment_intent);
  // S3-6: paid and failed are final; a late 'unpaid' copy can't undo them (nor a refund or a
  // dispute). The payment intent (needed to match refunds and disputes) is filled in whenever
  // a copy carries one, even on a settled order.
  return env.DB.prepare(
    `INSERT INTO orders (id, email, user_id, amount_total, currency, status, shipping_json, created_at, payment_intent)
     VALUES (?1, ?2, NULL, ?3, ?4, ?5, ?6, ?7, ?8)
     ON CONFLICT (id) DO UPDATE SET
       status = CASE WHEN orders.status IN (${SETTLED_ORDER}) THEN orders.status ELSE excluded.status END,
       email = CASE WHEN orders.status IN (${SETTLED_ORDER}) THEN orders.email
                    ELSE COALESCE(excluded.email, orders.email) END,
       shipping_json = CASE WHEN orders.status IN (${SETTLED_ORDER}) THEN orders.shipping_json
                            ELSE COALESCE(excluded.shipping_json, orders.shipping_json) END,
       payment_intent = COALESCE(orders.payment_intent, excluded.payment_intent)`,
  ).bind(
    String(s.id).slice(0, 255),
    email,
    Number.isInteger(s.amount_total) ? s.amount_total : null,
    typeof s.currency === "string" ? s.currency.slice(0, 8) : null,
    status.slice(0, 32),
    shipping ? JSON.stringify(shipping).slice(0, 4000) : null,
    now,
    pi && PI_RE.test(pi) ? pi.slice(0, 255) : null,
  );
}

/**
 * charge.refunded (review G6): a full or partial refund of a store order, matched by the
 * payment intent stored from checkout.session.completed. Refunds are made in the Dashboard.
 * Charges that aren't store orders (the plan's invoices) match nothing: the plan follows the
 * subscription's status instead. The refunded amount only grows, so a late copy of an older
 * partial refund can't undo a later one, and `refunded` never goes back to partial.
 */
function chargeRefunded(env: Env, ch: Obj): D1PreparedStatement | null {
  const pi = idOf(ch.payment_intent);
  const refunded = ch.amount_refunded;
  if (!pi || !PI_RE.test(pi) || !Number.isInteger(refunded) || refunded <= 0) return null;
  const full = ch.refunded === true || (Number.isInteger(ch.amount) && refunded >= ch.amount);
  return env.DB.prepare(
    `UPDATE orders SET
       status = CASE WHEN ?2 = 1 OR status = 'refunded' THEN 'refunded' ELSE 'partially_refunded' END,
       amount_refunded = MAX(COALESCE(amount_refunded, 0), ?3)
     WHERE payment_intent = ?1 AND status IN ('paid', 'partially_refunded', 'refunded')
       AND COALESCE(amount_refunded, 0) <= ?3`,
  ).bind(pi, full ? 1 : 0, refunded);
}

/**
 * charge.dispute.created / .closed (review G6): a chargeback on a store order. Don't ship
 * while `disputed`. Closed moves it to `dispute_won` (also an inquiry closed without a
 * chargeback, `warning_closed`) or `dispute_lost`. Never backwards: a late `created` can't
 * reopen a closed dispute.
 */
function disputeChanged(env: Env, type: string, d: Obj): D1PreparedStatement | null {
  const pi = idOf(d.payment_intent);
  if (!pi || !PI_RE.test(pi)) return null;
  if (type === "charge.dispute.created") {
    return env.DB.prepare(
      `UPDATE orders SET status = 'disputed'
       WHERE payment_intent = ?1 AND status IN ('paid', 'partially_refunded', 'refunded')`,
    ).bind(pi);
  }
  const outcome =
    d.status === "won" || d.status === "warning_closed" ? "dispute_won" : d.status === "lost" ? "dispute_lost" : null;
  if (!outcome) return null;
  return env.DB.prepare(
    `UPDATE orders SET status = ?2
     WHERE payment_intent = ?1 AND status IN ('paid', 'partially_refunded', 'refunded', 'disputed')`,
  ).bind(pi, outcome);
}

/**
 * customer.deleted (review G5): the customer was deleted at Stripe (in the Dashboard, or by our
 * own account deletion). Unlink it, so the next checkout makes a new customer instead of
 * failing on the deleted one for good, and mark that account's subscriptions canceled
 * (deleting a customer cancels them at once). The customer-create attempt moves on, so the new
 * customer isn't answered with the deleted one from the old idempotency key.
 */
function customerDeleted(env: Env, c: Obj, now: number): D1PreparedStatement[] {
  const id = idOf(c);
  if (!id || !/^cus_[A-Za-z0-9]+$/.test(id)) return [];
  return [
    env.DB.prepare(
      `UPDATE subscriptions SET status = 'canceled', cancel_at_period_end = 0, past_due_since = NULL,
         payment_action_at = NULL, payment_action_url = NULL, updated_at = MAX(updated_at, ?2)
       WHERE user_id IN (SELECT id FROM users WHERE stripe_customer_id = ?1)
         AND status NOT IN ('canceled', 'incomplete_expired')`,
    ).bind(id, now),
    env.DB.prepare(
      `UPDATE users SET stripe_customer_id = NULL, stripe_customer_attempt = stripe_customer_attempt + 1
       WHERE stripe_customer_id = ?1`,
    ).bind(id),
  ];
}

/**
 * The page where the customer confirms a payment: only Stripe's hosted invoice page (or, in
 * local development, the fake Stripe at exactly STRIPE_API_BASE). Anything else is dropped.
 */
function hostedInvoiceUrl(v: unknown, cfg: Config): string | null {
  if (typeof v !== "string" || v.length > 2048) return null;
  let u: URL;
  try {
    u = new URL(v);
  } catch {
    return null;
  }
  if (u.protocol === "https:" && u.hostname === "invoice.stripe.com") return u.href;
  const localFake = cfg.devMode && u.origin === cfg.stripe.apiBase && cfg.stripe.apiBase.startsWith("http://127.0.0.1");
  return localFake ? u.href : null;
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
async function refreshSubscription(
  env: Env,
  cfg: Config,
  subId: string | null,
  now: number,
  action?: { url: string | null },
): Promise<D1PreparedStatement[]> {
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
  // Review G5: "confirm your payment" only means something while the subscription still waits
  // for money. Any other state Stripe reports (paid, canceled...) clears it.
  const awaiting = status === "past_due" || status === "incomplete" || status === "unpaid";
  const actionAt = action && awaiting ? now : null;
  const actionUrl = action && awaiting ? action.url : null;
  return [
    env.DB.prepare(
      `INSERT INTO subscriptions (id, user_id, status, price_id, current_period_end, cancel_at_period_end, updated_at,
                                  past_due_since, payment_action_at, payment_action_url)
       VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, CASE WHEN ?3 = 'past_due' THEN ?7 END, ?8, ?9)
       ON CONFLICT (id) DO UPDATE SET status = excluded.status, price_id = excluded.price_id,
         current_period_end = excluded.current_period_end, cancel_at_period_end = excluded.cancel_at_period_end,
         updated_at = excluded.updated_at,
         past_due_since = CASE WHEN excluded.status = 'past_due'
                               THEN COALESCE(subscriptions.past_due_since, excluded.past_due_since) END,
         payment_action_at = CASE WHEN excluded.status IN ('past_due', 'incomplete', 'unpaid')
                                  THEN COALESCE(excluded.payment_action_at, subscriptions.payment_action_at) END,
         payment_action_url = CASE WHEN excluded.status IN ('past_due', 'incomplete', 'unpaid')
                                   THEN CASE WHEN excluded.payment_action_at IS NOT NULL THEN excluded.payment_action_url
                                             ELSE subscriptions.payment_action_url END END
       WHERE subscriptions.user_id = excluded.user_id`,
    ).bind(id, user.id, status, price, periodEndOf(sub), cancelAtEnd, now, actionAt, actionUrl),
  ];
}
