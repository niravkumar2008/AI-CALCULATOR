// AI Calculator API: one Worker for the calculators (/v1/*) and the portal (/api/*).
// Contract: docs/web/openapi.yaml. Plan: docs/web/PLAN.md.

import { claim, deleteMe, getMe, renameDevice, revokeDevice } from "./account";
import { enrolHardware, publishFirmware } from "./admin";
import { confirm, login, logout } from "./auth";
import { billingCheckout, billingPortal, storeCheckout, storeStatus, stripeWebhook } from "./billing";
import { joinWaitlist } from "./waitlist";
import { devRoute } from "./dev";
import { firmwareCheck, firmwareImage, pairStart, register, status } from "./device";
import { ConfigError, type Config, type Env, config } from "./env";
import { HttpError, MSG, error, json } from "./http";
import { STALE_INFLIGHT_SEC, solve } from "./solve";
import { nowSec } from "./util";

type Handler = (req: Request, env: Env, cfg: Config, ctx: ExecutionContext, now: number, param: string) => Promise<Response>;

const ROUTES: Record<string, Partial<Record<string, Handler>>> = {
  "/healthz": { GET: async () => json({ ok: true }) },
  // ---- calculators
  "/v1/device/register": { POST: (r, e, c, _x, n) => register(r, e, c, n) },
  "/v1/pair/start": { POST: (r, e, c, _x, n) => pairStart(r, e, c, n) },
  "/v1/device/status": { GET: (r, e, c, _x, n) => status(r, e, c, n) },
  "/v1/solve": { POST: (r, e, c, x, n) => solve(r, e, c, x, n) },
  "/v1/firmware": { GET: (r, e, _c, _x, n) => firmwareCheck(r, e, n) },
  "/v1/firmware/image": { GET: (r, e, _c, _x, n) => firmwareImage(r, e, n) },
  "/v1/admin/firmware": { POST: (r, e) => publishFirmware(r, e) },
  "/v1/admin/hardware": { POST: (r, e, _c, _x, n) => enrolHardware(r, e, n) },
  // ---- portal
  "/api/auth/login": { POST: (r, e, c, x, n) => login(r, e, c, x, n) },
  "/api/auth/confirm": { POST: (r, e, c, _x, n) => confirm(r, e, c, n) },
  "/api/auth/logout": { POST: (r, e, c) => logout(r, e, c) },
  "/api/me": { GET: (r, e, c, _x, n) => getMe(r, e, c, n), DELETE: (r, e, c, _x, n) => deleteMe(r, e, c, n) },
  "/api/devices/claim": { POST: (r, e, c, _x, n) => claim(r, e, c, n) },
  "/api/devices/:id": { PATCH: (r, e, c, _x, n, id) => renameDevice(r, e, c, id, n) },
  "/api/devices/:id/revoke": { POST: (r, e, c, _x, n, id) => revokeDevice(r, e, c, id, n) },
  // ---- stage 3: Stripe and the launch-site waitlist
  "/api/billing/checkout": { POST: (r, e, c, _x, n) => billingCheckout(r, e, c, n) },
  "/api/billing/portal": { POST: (r, e, c, _x, n) => billingPortal(r, e, c, n) },
  "/api/store/checkout": { POST: (r, e, c, _x, n) => storeCheckout(r, e, c, n) },
  "/api/store/status": { GET: (r, e, c) => storeStatus(r, e, c) },
  "/api/stripe/webhook": { POST: (r, e, c, _x, n) => stripeWebhook(r, e, c, n) },
  "/api/waitlist": { POST: (r, e, c, _x, n) => joinWaitlist(r, e, c, n) },
};

/** S4-2: a malformed %-escape is just an unknown id (404), never an internal error. */
function decodeParam(s: string): string | null {
  try {
    return decodeURIComponent(s);
  } catch {
    return null;
  }
}

function match(path: string): { route: string; param: string } | null {
  if (ROUTES[path]) return { route: path, param: "" };
  let m = /^\/api\/devices\/([^/]+)\/revoke$/.exec(path);
  let route = "/api/devices/:id/revoke";
  if (!m) {
    m = /^\/api\/devices\/([^/]+)$/.exec(path);
    route = "/api/devices/:id";
  }
  if (!m) return null;
  const param = decodeParam(m[1]!);
  return param === null ? null : { route, param };
}

export async function handle(req: Request, env: Env, ctx: ExecutionContext): Promise<Response> {
  try {
    const cfg = config(env);
    const now = nowSec();
    const path = new URL(req.url).pathname;
    if (path.startsWith("/__dev/")) return await devRoute(req, env, cfg, path, now);
    const m = match(path);
    if (!m) return error(404, "not_found", MSG.notFound);
    const methods = ROUTES[m.route]!;
    const h = methods[req.method];
    if (!h) {
      return error(405, "bad_request", MSG.methodNotAllowed, { allow: Object.keys(methods).join(", ") });
    }
    return await h(req, env, cfg, ctx, now, m.param);
  } catch (e) {
    if (e instanceof HttpError) return e.response();
    // Never leak details: log server-side, answer with a plain message (fail closed).
    const kind = e instanceof ConfigError ? "config_error" : "internal_error";
    console.error(JSON.stringify({ event: kind, detail: String(e).slice(0, 300) }));
    return error(503, "unavailable", MSG.unavailable);
  }
}

/**
 * How long processed Stripe event ids are kept for de-duplication. (Not exported: workerd
 * only accepts handlers and functions as named exports of the entry module.)
 */
const STRIPE_EVENTS_KEEP_SEC = 30 * 86400;

/** Hourly clean-up (cron in wrangler.toml). */
export async function cleanup(env: Env, now: number): Promise<void> {
  const unusedSince = now - 30 * 86400;
  const stale = now - STALE_INFLIGHT_SEC;
  await env.DB.batch([
    // S12: reservations whose Worker died are settled at their worst case (safe for the caps).
    env.DB.prepare(
      `UPDATE spend_daily SET
         reserved_micro = MAX(0, reserved_micro - (SELECT COALESCE(SUM(reserved_micro), 0) FROM usage
                                                   WHERE state = 'reserved' AND started_at < ?1 AND day = spend_daily.day)),
         settled_micro = settled_micro + (SELECT COALESCE(SUM(reserved_micro), 0) FROM usage
                                          WHERE state = 'reserved' AND started_at < ?1 AND day = spend_daily.day)
       WHERE day IN (SELECT day FROM usage WHERE state = 'reserved' AND started_at < ?1)`,
    ).bind(stale),
    env.DB.prepare(
      `UPDATE usage SET state = 'settled', cost_micro = reserved_micro WHERE state = 'reserved' AND started_at < ?1`,
    ).bind(stale),
    // Unlinked calculators unused for 30 days (their token can't spend money anyway).
    env.DB.prepare(
      `DELETE FROM pair_codes WHERE device_id IN
         (SELECT id FROM devices WHERE user_id IS NULL AND COALESCE(last_seen_at, created_at) < ?1)`,
    ).bind(unusedSince),
    env.DB.prepare(`DELETE FROM devices WHERE user_id IS NULL AND COALESCE(last_seen_at, created_at) < ?1`).bind(
      unusedSince,
    ),
    env.DB.prepare(`DELETE FROM pair_codes WHERE expires_at < ?1`).bind(now - 86400),
    env.DB.prepare(`DELETE FROM login_tokens WHERE expires_at < ?1`).bind(now - 86400),
    env.DB.prepare(`DELETE FROM sessions WHERE expires_at < ?1`).bind(now),
    env.DB.prepare(`DELETE FROM rate_limits WHERE window_start < ?1`).bind(now - 2 * 86400),
    env.DB.prepare(`DELETE FROM dev_outbox WHERE created_at < ?1`).bind(now - 86400),
    // Review G9: processed Stripe event ids. Stripe retries an event for at most 3 days, so
    // after 30 days the id can't come back; without this the table only grows.
    env.DB.prepare(`DELETE FROM stripe_events WHERE received_at < ?1`).bind(now - STRIPE_EVENTS_KEEP_SEC),
  ]);
}

export default {
  fetch: handle,
  async scheduled(_c: ScheduledController, env: Env, ctx: ExecutionContext) {
    ctx.waitUntil(cleanup(env, nowSec()));
  },
} satisfies ExportedHandler<Env>;
