// Bindings and settings (PLAN section 4b). Every setting arrives as a string; config() parses
// them once per request and throws ConfigError on a bad value, which the caller turns into
// 503 `unavailable` (fail closed).

export interface Env {
  DB: D1Database;
  FIRMWARE: KVNamespace;
  ANTHROPIC_API_KEY?: string;
  ANTHROPIC_BASE_URL?: string;
  MODEL?: string;
  MAX_TOKENS?: string;
  EFFORT?: string;
  REFUSAL_FALLBACK?: string;
  UPSTREAM_TIMEOUT_MS?: string;
  PUBLIC_URL?: string;
  TRIAL_DAYS?: string;
  MONTHLY_SOLVES?: string;
  MONTHLY_SPEND_USD?: string;
  GLOBAL_DAILY_CAP_USD?: string;
  PRICE_IN_PER_MTOK?: string;
  PRICE_OUT_PER_MTOK?: string;
  RESEND_API_KEY?: string;
  MAIL_FROM?: string;
  ADMIN_TOKEN?: string;
  DEV_MODE?: string;
  MAX_NEW_TRIALS_PER_DAY?: string;
  MAIL_DAILY_CAP?: string;
  TURNSTILE_SECRET?: string;
  STREAM_MAX_MS?: string;
  TRIAL_PEPPER?: string;
  REGISTER_GLOBAL_PER_HOUR?: string;
}

export class ConfigError extends Error {}

export interface Config {
  publicUrl: string; // origin + optional path, no trailing slash
  publicOrigin: string;
  host: string; // shown on the calculator, e.g. "aicalc.example"
  secureCookie: boolean;
  trialDays: number;
  monthlySolves: number;
  monthlySpendMicro: number;
  globalDailyCapMicro: number;
  priceInMicroPerToken: number; // USD per MTok == micro-USD per token
  priceOutMicroPerToken: number;
  model: string;
  maxTokens: number;
  effort: string;
  refusalFallback: boolean;
  upstreamTimeoutMs: number;
  anthropicBaseUrl: string;
  devMode: boolean;
  maxNewTrialsPerDay: number;
  mailDailyCap: number;
  streamMaxMs: number;
  /** HMAC key for e-mail hashes (trials_used, login rate-limit keys). */
  trialPepper: string;
  registerGlobalPerHour: number;
}

/** A reservation older than this no longer blocks the device and is settled by the cron. */
export const STALE_INFLIGHT_SEC = 600;
/** Used only with DEV_MODE=1 (local). Production must set the TRIAL_PEPPER secret. */
export const DEV_TRIAL_PEPPER = "dev-only-trial-pepper-not-secret-0123456789";

function num(v: string | undefined, def: number, name: string, min = 0): number {
  const s = (v ?? "").trim();
  if (s === "") return def;
  const n = Number(s);
  if (!Number.isFinite(n) || n < min) throw new ConfigError(`bad setting ${name}`);
  return n;
}

const EFFORTS = new Set(["low", "medium", "high", "xhigh", "max"]);

export function config(env: Env): Config {
  const c = parseConfig(env);
  // R2-6: a solve must always finish (or be cut) before its reservation counts as stale.
  if (c.streamMaxMs + c.upstreamTimeoutMs >= STALE_INFLIGHT_SEC * 1000) {
    throw new ConfigError("STREAM_MAX_MS + UPSTREAM_TIMEOUT_MS must be under the 600 s stale limit");
  }
  return c;
}

function parseConfig(env: Env): Config {
  let url: URL;
  try {
    url = new URL((env.PUBLIC_URL ?? "").trim() || "http://localhost:8787");
  } catch {
    throw new ConfigError("bad setting PUBLIC_URL");
  }
  if (url.protocol !== "https:" && url.protocol !== "http:") throw new ConfigError("bad PUBLIC_URL");
  const publicUrl = (url.origin + url.pathname).replace(/\/+$/, "");
  const effort = (env.EFFORT ?? "").trim() || "high";
  if (!EFFORTS.has(effort)) throw new ConfigError("bad setting EFFORT");
  const devMode = (env.DEV_MODE ?? "").trim() === "1";
  // S8: production must be https (cookies, links on the calculator); http only for local dev.
  if (url.protocol !== "https:" && !devMode) throw new ConfigError("PUBLIC_URL must be https unless DEV_MODE=1");
  // R2-7: DEV_MODE only makes sense for a local PUBLIC_URL; refuse it anywhere else.
  if (devMode && url.hostname !== "localhost" && url.hostname !== "127.0.0.1") {
    throw new ConfigError("DEV_MODE=1 is only allowed with a localhost PUBLIC_URL");
  }
  // R2-3: e-mail hashes are HMACs with a secret pepper.
  const pepper = (env.TRIAL_PEPPER ?? "").trim();
  if (!devMode && pepper.length < 32) throw new ConfigError("TRIAL_PEPPER (32+ characters) is required");
  const local = url.protocol === "http:" && (url.hostname === "localhost" || url.hostname === "127.0.0.1");
  const base = ((env.ANTHROPIC_BASE_URL ?? "").trim() || "https://api.anthropic.com").replace(/\/+$/, "");
  return {
    publicUrl,
    publicOrigin: url.origin,
    host: url.host + url.pathname.replace(/\/+$/, ""),
    secureCookie: !local,
    trialDays: num(env.TRIAL_DAYS, 30, "TRIAL_DAYS"),
    monthlySolves: Math.floor(num(env.MONTHLY_SOLVES, 300, "MONTHLY_SOLVES")),
    monthlySpendMicro: Math.round(num(env.MONTHLY_SPEND_USD, 8, "MONTHLY_SPEND_USD") * 1e6),
    globalDailyCapMicro: Math.round(num(env.GLOBAL_DAILY_CAP_USD, 50, "GLOBAL_DAILY_CAP_USD") * 1e6),
    priceInMicroPerToken: num(env.PRICE_IN_PER_MTOK, 2, "PRICE_IN_PER_MTOK"),
    priceOutMicroPerToken: num(env.PRICE_OUT_PER_MTOK, 10, "PRICE_OUT_PER_MTOK"),
    model: (env.MODEL ?? "").trim() || "claude-sonnet-5-5",
    maxTokens: Math.floor(num(env.MAX_TOKENS, 16000, "MAX_TOKENS", 1)),
    effort,
    refusalFallback: (env.REFUSAL_FALLBACK ?? "").trim() === "1",
    upstreamTimeoutMs: num(env.UPSTREAM_TIMEOUT_MS, 60000, "UPSTREAM_TIMEOUT_MS", 1000),
    anthropicBaseUrl: base,
    devMode,
    maxNewTrialsPerDay: Math.floor(num(env.MAX_NEW_TRIALS_PER_DAY, 20, "MAX_NEW_TRIALS_PER_DAY")),
    mailDailyCap: Math.floor(num(env.MAIL_DAILY_CAP, 80, "MAIL_DAILY_CAP")),
    // S12: total time one solve may stream (the calculator gives up long before).
    streamMaxMs: num(env.STREAM_MAX_MS, 290_000, "STREAM_MAX_MS", 50),
    trialPepper: pepper.length >= 32 ? pepper : DEV_TRIAL_PEPPER,
    registerGlobalPerHour: Math.floor(num(env.REGISTER_GLOBAL_PER_HOUR, 200, "REGISTER_GLOBAL_PER_HOUR")),
  };
}
