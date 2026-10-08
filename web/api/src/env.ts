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
}

function num(v: string | undefined, def: number, name: string, min = 0): number {
  const s = (v ?? "").trim();
  if (s === "") return def;
  const n = Number(s);
  if (!Number.isFinite(n) || n < min) throw new ConfigError(`bad setting ${name}`);
  return n;
}

const EFFORTS = new Set(["low", "medium", "high", "xhigh", "max"]);

export function config(env: Env): Config {
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
    devMode: (env.DEV_MODE ?? "").trim() === "1",
  };
}
