// AI Calculator API: one Worker for the calculators (/v1/*) and the portal (/api/*).
// Contract: docs/web/openapi.yaml. Plan: docs/web/PLAN.md.

import { claim, deleteMe, getMe, renameDevice, revokeDevice } from "./account";
import { publishFirmware } from "./admin";
import { confirm, login, logout } from "./auth";
import { devRoute } from "./dev";
import { firmwareCheck, firmwareImage, pairStart, register, status } from "./device";
import { ConfigError, type Config, type Env, config } from "./env";
import { HttpError, MSG, error, json } from "./http";
import { solve } from "./solve";
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
  // ---- portal
  "/api/auth/login": { POST: (r, e, c, x, n) => login(r, e, c, x, n) },
  "/api/auth/confirm": { POST: (r, e, c, _x, n) => confirm(r, e, c, n) },
  "/api/auth/logout": { POST: (r, e, c) => logout(r, e, c) },
  "/api/me": { GET: (r, e, c, _x, n) => getMe(r, e, c, n), DELETE: (r, e, c, _x, n) => deleteMe(r, e, c, n) },
  "/api/devices/claim": { POST: (r, e, c, _x, n) => claim(r, e, c, n) },
  "/api/devices/:id": { PATCH: (r, e, c, _x, n, id) => renameDevice(r, e, c, id, n) },
  "/api/devices/:id/revoke": { POST: (r, e, c, _x, n, id) => revokeDevice(r, e, c, id, n) },
  // TODO(stage 3): /api/billing/checkout, /api/billing/portal, /api/store/checkout and
  // /api/stripe/webhook (Stripe). Until then they fall through to 404 `not_found`.
};

function match(path: string): { route: string; param: string } | null {
  if (ROUTES[path]) return { route: path, param: "" };
  let m = /^\/api\/devices\/([^/]+)\/revoke$/.exec(path);
  if (m) return { route: "/api/devices/:id/revoke", param: decodeURIComponent(m[1]!) };
  m = /^\/api\/devices\/([^/]+)$/.exec(path);
  if (m) return { route: "/api/devices/:id", param: decodeURIComponent(m[1]!) };
  return null;
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

/** Daily clean-up (cron in wrangler.toml). */
export async function cleanup(env: Env, now: number): Promise<void> {
  const unusedSince = now - 30 * 86400;
  await env.DB.batch([
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
  ]);
}

export default {
  fetch: handle,
  async scheduled(_c: ScheduledController, env: Env, ctx: ExecutionContext) {
    ctx.waitUntil(cleanup(env, nowSec()));
  },
} satisfies ExportedHandler<Env>;
