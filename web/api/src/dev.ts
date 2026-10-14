// /__dev/* helpers for local tests. Honoured ONLY when DEV_MODE=1 AND the request host is
// localhost or 127.0.0.1; otherwise they don't exist (404). Production never sets DEV_MODE.

import type { Config, Env } from "./env";
import { newId } from "./crypto";
import { HttpError, MSG, json } from "./http";
import { isEmail, onlyKeys, readJsonObject } from "./util";

const PLAN_OVERRIDES = new Set(["none", "trial", "active", "past_due", "inactive"]);

export function devAllowed(req: Request, cfg: Config): boolean {
  const host = new URL(req.url).hostname;
  return cfg.devMode && (host === "localhost" || host === "127.0.0.1");
}

export async function devRoute(req: Request, env: Env, cfg: Config, path: string, now: number) {
  if (!devAllowed(req, cfg)) throw new HttpError(404, "not_found", MSG.notFound);

  if (path === "/__dev/outbox" && req.method === "GET") {
    const email = new URL(req.url).searchParams.get("email") ?? "";
    const row = await env.DB.prepare(
      `SELECT link, token FROM dev_outbox WHERE email = ?1 ORDER BY id DESC LIMIT 1`,
    )
      .bind(email.trim().toLowerCase())
      .first<{ link: string; token: string }>();
    if (!row) throw new HttpError(404, "not_found", "No e-mail for that address.");
    return json(row);
  }

  if (path === "/__dev/user" && req.method === "POST") {
    const body = await readJsonObject(req, 4096);
    onlyKeys(body, ["email", "trial_ends_at", "plan_override"]);
    if (!isEmail(body.email)) throw new HttpError(400, "bad_request", "email is required.");
    const email = body.email.trim().toLowerCase();
    const t = body.trial_ends_at;
    if (t !== undefined && t !== null && !(typeof t === "number" && Number.isInteger(t))) {
      throw new HttpError(400, "bad_request", "trial_ends_at must be an integer or null.");
    }
    const p = body.plan_override;
    if (p !== undefined && p !== null && !(typeof p === "string" && PLAN_OVERRIDES.has(p))) {
      throw new HttpError(400, "bad_request", "plan_override must be none, trial, active, past_due, inactive or null.");
    }
    await env.DB.prepare(`INSERT INTO users (id, email, created_at) VALUES (?1, ?2, ?3) ON CONFLICT (email) DO NOTHING`)
      .bind(newId("usr_"), email, now)
      .run();
    if (t !== undefined) await env.DB.prepare(`UPDATE users SET trial_ends_at = ?1 WHERE email = ?2`).bind(t, email).run();
    if (p !== undefined) await env.DB.prepare(`UPDATE users SET plan_override = ?1 WHERE email = ?2`).bind(p, email).run();
    const u = await env.DB.prepare(`SELECT id, email, trial_ends_at, plan_override FROM users WHERE email = ?1`)
      .bind(email)
      .first();
    return json(u);
  }

  if (path === "/__dev/outbox" || path === "/__dev/user") {
    throw new HttpError(405, "bad_request", MSG.methodNotAllowed);
  }
  throw new HttpError(404, "not_found", MSG.notFound);
}
