// Entitlement: plan state, monthly usage, the global daily cap and the cost model
// (PLAN section 3 "Entitlement" and section 4 item 5).

import type { Config, StripeConfig } from "./env";
import { HttpError, MSG, msgNoTrial, msgNotLinked, msgSubEnded, msgTrialOver } from "./http";
import { dayOf, monthOf } from "./util";
import { currentPairCode } from "./pairing";
import { formatPairCode } from "./crypto";

export type PlanState = "none" | "trial" | "active" | "past_due" | "inactive";
const PLAN_STATES = new Set<PlanState>(["none", "trial", "active", "past_due", "inactive"]);

export interface UserRow {
  id: string;
  email: string;
  created_at: number;
  trial_ends_at: number | null;
  deleted_at: number | null;
  plan_override: string | null;
  stripe_customer_id?: string | null;
  /** Failed (5xx / timeout) customer creates so far: picks the idempotency key (review G9). */
  stripe_customer_attempt?: number;
}

export interface DeviceRow {
  id: string;
  hw_id: string | null;
  fw_version: string | null;
  user_id: string | null;
  label: string | null;
  created_at: number;
  last_seen_at: number | null;
  revoked_at: number | null;
  enrolled?: number;
}

export interface PlanInfo {
  plan: PlanState;
  entitled: boolean;
  /** Why not entitled: the trial ran out, or a subscription ended. */
  hadSubscription: boolean;
  trialEndsAt?: number;
  periodEnd?: number;
  cancelAtPeriodEnd?: boolean;
  /** Billing interval of the subscription behind an active / past_due plan; null otherwise. */
  interval: "month" | "year" | null;
  /**
   * Review G5: the subscription's latest payment needs the customer to confirm it (3-D Secure),
   * so the account shows "Confirm your payment". `url` is Stripe's hosted invoice page, if
   * Stripe sent one. Undefined when nothing needs confirming.
   */
  paymentAction?: { url: string | null; since: number };
}

const PAST_DUE_GRACE_SEC = 3 * 86400;

/**
 * The account's plan. The AI tutor plan's prices (STRIPE_PRICE_SUB monthly and
 * STRIPE_PRICE_SUB_YEARLY yearly) limit entitlement to subscriptions on our AI plan (S3-5): a
 * subscription to anything else in the Stripe account never unlocks AI. Both intervals are
 * the same plan, so switching between them (one subscription, its price changed) keeps AI on.
 */
export async function planOf(
  db: D1Database,
  user: UserRow,
  now: number,
  prices: Pick<StripeConfig, "priceSub" | "priceSubYearly"> = { priceSub: "", priceSubYearly: "" },
): Promise<PlanInfo> {
  const trialEndsAt = user.trial_ends_at ?? undefined;
  const monthly = prices.priceSub;
  const yearly = prices.priceSubYearly;
  const sub = await db
    .prepare(
      // A live subscription wins over an older ended one (e.g. cancelled, then re-subscribed),
      // whatever order their events arrived in. Then the most recently updated.
      `SELECT status, price_id, current_period_end, cancel_at_period_end, past_due_since, updated_at,
              payment_action_at, payment_action_url FROM subscriptions
       WHERE user_id = ?1 AND ((?2 = '' AND ?3 = '') OR price_id IN (NULLIF(?2, ''), NULLIF(?3, '')))
       ORDER BY CASE status WHEN 'active' THEN 0 WHEN 'trialing' THEN 0 WHEN 'past_due' THEN 1 ELSE 2 END,
                updated_at DESC
       LIMIT 1`,
    )
    .bind(user.id, monthly, yearly)
    .first<{
      status: string;
      price_id: string | null;
      current_period_end: number | null;
      cancel_at_period_end: number;
      past_due_since: number | null;
      updated_at: number;
      payment_action_at: number | null;
      payment_action_url: string | null;
    }>();

  const interval: PlanInfo["interval"] =
    sub && yearly && sub.price_id === yearly ? "year" : sub && sub.price_id && sub.price_id === monthly ? "month" : null;
  const base = {
    interval: null as PlanInfo["interval"],
    hadSubscription: sub !== null,
    trialEndsAt,
    periodEnd: sub?.current_period_end ?? undefined,
    cancelAtPeriodEnd: sub ? sub.cancel_at_period_end === 1 : undefined,
    // Only while the subscription still waits for that money (the webhook clears it otherwise).
    paymentAction:
      sub && sub.payment_action_at !== null && ["past_due", "incomplete", "unpaid"].includes(sub.status)
        ? { url: sub.payment_action_url, since: sub.payment_action_at }
        : undefined,
  };

  // Local tests only (/__dev/user). Never set in production.
  if (user.plan_override && PLAN_STATES.has(user.plan_override as PlanState)) {
    const plan = user.plan_override as PlanState;
    // "inactive" simulates an ended subscription ("Your subscription has ended..."); an expired
    // trial without an override gives "Your free month is over...".
    return {
      ...base,
      plan,
      interval: plan === "active" || plan === "past_due" ? interval : null,
      hadSubscription: base.hadSubscription || plan === "inactive",
      entitled: plan === "trial" || plan === "active" || plan === "past_due",
    };
  }

  if (sub) {
    if (sub.status === "active" || sub.status === "trialing") return { ...base, interval, plan: "active", entitled: true };
    if (sub.status === "past_due") {
      // S3-1: 3 days from the failed payment (first seen past_due), never from the period end,
      // which Stripe moves forward at renewal even when the charge fails.
      const since = sub.past_due_since ?? sub.updated_at;
      const graceOk = now < since + PAST_DUE_GRACE_SEC;
      return { ...base, interval: graceOk ? interval : null, plan: graceOk ? "past_due" : "inactive", entitled: graceOk };
    }
  }
  // Our own free month (decision D2) also covers a user whose subscription ended mid-trial.
  if (trialEndsAt !== undefined && trialEndsAt > now) return { ...base, plan: "trial", entitled: true };
  if (sub || trialEndsAt !== undefined) return { ...base, plan: "inactive", entitled: false };
  return { ...base, plan: "none", entitled: false };
}

export interface MonthUsage {
  solves: number;
  spendMicro: number;
}

/** Solves (reserved or settled) and spend (reservations at worst case) this UTC month. */
export async function monthUsage(db: D1Database, userId: string, now: number): Promise<MonthUsage> {
  const r = await db
    .prepare(
      `SELECT
         COALESCE(SUM(CASE WHEN state IN ('reserved','settled') THEN 1 ELSE 0 END), 0) AS solves,
         COALESCE(SUM(CASE state WHEN 'reserved' THEN reserved_micro WHEN 'settled' THEN cost_micro ELSE 0 END), 0) AS spend
       FROM usage WHERE user_id = ?1 AND month = ?2`,
    )
    .bind(userId, monthOf(now))
    .first<{ solves: number; spend: number }>();
  return { solves: r?.solves ?? 0, spendMicro: r?.spend ?? 0 };
}

export async function globalSpendToday(db: D1Database, now: number): Promise<number> {
  const r = await db
    .prepare(`SELECT reserved_micro + settled_micro AS total FROM spend_daily WHERE day = ?1`)
    .bind(dayOf(now))
    .first<{ total: number }>();
  return r?.total ?? 0;
}

// ------------------------------------------------------------ cost model
/** Upper bounds used for the reservation: system prompt + schema + text, and per photo. */
export const INPUT_BASE_TOKENS = 3000;
export const INPUT_PER_IMAGE_TOKENS = 5000;

export function worstCaseMicro(cfg: Config, images: number): number {
  const input = INPUT_BASE_TOKENS + INPUT_PER_IMAGE_TOKENS * images;
  return Math.ceil(input * cfg.priceInMicroPerToken + cfg.maxTokens * cfg.priceOutMicroPerToken);
}

export interface TokenUsage {
  input: number;
  cacheWrite: number;
  cacheRead: number;
  output: number;
}

export function actualCostMicro(cfg: Config, u: TokenUsage): number {
  return Math.ceil(
    u.input * cfg.priceInMicroPerToken +
      u.cacheWrite * cfg.priceInMicroPerToken * 1.25 +
      u.cacheRead * cfg.priceInMicroPerToken * 0.1 +
      u.output * cfg.priceOutMicroPerToken,
  );
}

// ------------------------------------------------------------ the shared "can it solve?" check
export async function loadUser(db: D1Database, userId: string): Promise<UserRow | null> {
  return db
    .prepare(`SELECT * FROM users WHERE id = ?1 AND deleted_at IS NULL`)
    .bind(userId)
    .first<UserRow>();
}

/**
 * DeviceStatus.plan_reason: the plan state as a short code for the calculator's Link screen.
 * Same decision as notEntitledError, so it always agrees with `reason`.
 */
export type PlanReason = "not_linked" | "trial" | "active" | "past_due" | "free_month_over" | "inactive" | "no_trial";

export function planReason(p: PlanInfo): PlanReason {
  if (p.entitled) return p.plan === "past_due" ? "past_due" : p.plan === "active" ? "active" : "trial";
  if (p.hadSubscription) return "inactive";
  return p.plan === "none" ? "no_trial" : "free_month_over";
}

export function notEntitledError(cfg: Config, p: PlanInfo): HttpError {
  const msg = p.hadSubscription ? msgSubEnded(cfg) : p.plan === "none" ? msgNoTrial(cfg) : msgTrialOver(cfg);
  return new HttpError(402, "subscription_inactive", msg);
}

export async function notLinkedError(db: D1Database, cfg: Config, deviceId: string, now: number): Promise<HttpError> {
  const code = await currentPairCode(db, deviceId, now);
  return new HttpError(402, "device_not_linked", msgNotLinked(cfg, formatPairCode(code)));
}

/** 429 with Retry-After = seconds until the 1st of next month (UTC). */
export function quotaError(now: number): HttpError {
  const d = new Date(now * 1000);
  const next = Date.UTC(d.getUTCFullYear(), d.getUTCMonth() + 1, 1) / 1000;
  return new HttpError(429, "quota_exceeded", MSG.quota, { "retry-after": String(Math.max(1, next - now)) });
}

/** 503 with Retry-After = seconds until the next UTC day. */
export function spendCapError(now: number): HttpError {
  const next = (Math.floor(now / 86400) + 1) * 86400;
  return new HttpError(503, "spend_cap", MSG.spendCap, { "retry-after": String(Math.max(1, next - now)) });
}
