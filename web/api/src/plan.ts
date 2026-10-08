// Entitlement: plan state, monthly usage, the global daily cap and the cost model
// (PLAN section 3 "Entitlement" and section 4 item 5).

import type { Config } from "./env";
import { HttpError, MSG, msgNotLinked, msgSubEnded, msgTrialOver } from "./http";
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
}

export interface PlanInfo {
  plan: PlanState;
  entitled: boolean;
  /** Why not entitled: the trial ran out, or a subscription ended. */
  hadSubscription: boolean;
  trialEndsAt?: number;
  periodEnd?: number;
  cancelAtPeriodEnd?: boolean;
}

const PAST_DUE_GRACE_SEC = 3 * 86400;

export async function planOf(db: D1Database, user: UserRow, now: number): Promise<PlanInfo> {
  const trialEndsAt = user.trial_ends_at ?? undefined;
  const sub = await db
    .prepare(
      `SELECT status, current_period_end, cancel_at_period_end FROM subscriptions
       WHERE user_id = ?1 ORDER BY updated_at DESC LIMIT 1`,
    )
    .bind(user.id)
    .first<{ status: string; current_period_end: number | null; cancel_at_period_end: number }>();

  const base = {
    hadSubscription: sub !== null,
    trialEndsAt,
    periodEnd: sub?.current_period_end ?? undefined,
    cancelAtPeriodEnd: sub ? sub.cancel_at_period_end === 1 : undefined,
  };

  // Local tests only (/__dev/user). Never set in production.
  if (user.plan_override && PLAN_STATES.has(user.plan_override as PlanState)) {
    const plan = user.plan_override as PlanState;
    // "inactive" simulates an ended subscription ("Your subscription has ended..."); an expired
    // trial without an override gives "Your free month is over...".
    return {
      ...base,
      plan,
      hadSubscription: base.hadSubscription || plan === "inactive",
      entitled: plan === "trial" || plan === "active" || plan === "past_due",
    };
  }

  if (sub) {
    if (sub.status === "active" || sub.status === "trialing") return { ...base, plan: "active", entitled: true };
    if (sub.status === "past_due") {
      const graceOk = sub.current_period_end !== null && now < sub.current_period_end + PAST_DUE_GRACE_SEC;
      return { ...base, plan: graceOk ? "past_due" : "inactive", entitled: graceOk };
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

export function notEntitledError(cfg: Config, p: PlanInfo): HttpError {
  return new HttpError(402, "subscription_inactive", p.hadSubscription ? msgSubEnded(cfg) : msgTrialOver(cfg));
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
