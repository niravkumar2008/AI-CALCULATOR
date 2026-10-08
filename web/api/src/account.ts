// Portal account endpoints: /api/me, /api/devices/*.

import type { Config, Env } from "./env";
import { normalisePairCode } from "./crypto";
import { HttpError, MSG, empty, json } from "./http";
import { clearCookie, requireBrowserWrite, requireUser } from "./auth";
import { type DeviceRow, monthUsage, planOf } from "./plan";
import { hitRateLimit, isEmail, isLabel, onlyKeys, rateLimited, readJsonObject } from "./util";

export const MAX_ACTIVE_DEVICES = 3;
const BODY_LIMIT = 16 * 1024;
const DEVICE_ID_RE = /^dev_[0-9a-f]{16}$/;

function deviceJson(d: DeviceRow) {
  const out: Record<string, unknown> = {
    id: d.id,
    label: d.label ?? "Calculator",
    created_at: d.created_at,
    revoked: d.revoked_at !== null,
  };
  if (d.hw_id) out.hw_id = d.hw_id;
  if (d.fw_version) out.fw_version = d.fw_version;
  if (d.last_seen_at !== null) out.last_seen_at = d.last_seen_at;
  return out;
}

const DEVICE_COLS = `id, hw_id, fw_version, user_id, label, created_at, last_seen_at, revoked_at`;

function labelOrBad(v: unknown): string {
  if (!isLabel(v)) {
    throw new HttpError(400, "bad_request", "Name must be 1-40 characters, without < > or control characters.");
  }
  return v;
}

// ------------------------------------------------------------ GET /api/me
export async function getMe(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  const user = await requireUser(req, env, now);
  const plan = await planOf(env.DB, user, now);
  const usage = await monthUsage(env.DB, user.id, now);
  const { results } = await env.DB.prepare(
    `SELECT ${DEVICE_COLS} FROM devices WHERE user_id = ?1 ORDER BY created_at`,
  )
    .bind(user.id)
    .all<DeviceRow>();
  const out: Record<string, unknown> = {
    email: user.email,
    plan: plan.plan,
    usage: {
      solves_this_month: usage.solves,
      solves_limit: cfg.monthlySolves,
      spend_limit_reached: usage.spendMicro >= cfg.monthlySpendMicro,
    },
    devices: results.map(deviceJson),
  };
  if (plan.trialEndsAt !== undefined) out.trial_ends_at = plan.trialEndsAt;
  if (plan.periodEnd !== undefined) out.period_end = plan.periodEnd;
  if (plan.cancelAtPeriodEnd !== undefined) out.cancel_at_period_end = plan.cancelAtPeriodEnd;
  return json(out);
}

// ------------------------------------------------------------ DELETE /api/me
export async function deleteMe(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  requireBrowserWrite(req, cfg);
  const user = await requireUser(req, env, now);
  const body = await readJsonObject(req, BODY_LIMIT);
  onlyKeys(body, ["confirm_email"]);
  if (!isEmail(body.confirm_email) || body.confirm_email.trim().toLowerCase() !== user.email.toLowerCase()) {
    throw new HttpError(400, "bad_request", "The e-mail doesn't match this account.");
  }
  // TODO(stage 3): cancel the Stripe subscription immediately before erasing the account.
  await env.DB.batch([
    env.DB.prepare(`UPDATE devices SET revoked_at = ?1 WHERE user_id = ?2 AND revoked_at IS NULL`).bind(now, user.id),
    env.DB.prepare(
      `DELETE FROM pair_codes WHERE claimed_at IS NULL AND device_id IN (SELECT id FROM devices WHERE user_id = ?1)`,
    ).bind(user.id),
    env.DB.prepare(`DELETE FROM sessions WHERE user_id = ?1`).bind(user.id),
    env.DB.prepare(`DELETE FROM login_tokens WHERE email = ?1`).bind(user.email),
    env.DB.prepare(`DELETE FROM dev_outbox WHERE email = ?1`).bind(user.email),
    // The e-mail is erased; usage numbers stay (no personal data in them).
    env.DB.prepare(`UPDATE users SET email = ?1, deleted_at = ?2, plan_override = NULL WHERE id = ?3`).bind(
      `deleted:${user.id}`,
      now,
      user.id,
    ),
  ]);
  return empty(204, { "set-cookie": clearCookie(cfg) });
}

// ------------------------------------------------------------ POST /api/devices/claim
export async function claim(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  requireBrowserWrite(req, cfg);
  const user = await requireUser(req, env, now);
  const body = await readJsonObject(req, BODY_LIMIT);
  onlyKeys(body, ["code", "label"]);
  if (typeof body.code !== "string" || body.code.length < 8 || body.code.length > 12) {
    throw new HttpError(400, "bad_request", "Enter the 8-character code shown on the calculator.");
  }
  const label = body.label === undefined ? null : labelOrBad(body.label);

  const rl = await hitRateLimit(env.DB, `claim:user:${user.id}`, 600, 10, now);
  if (!rl.ok) throw rateLimited(rl, "Too many attempts. Please wait 10 minutes.");

  const code = normalisePairCode(body.code);
  if (!code) throw new HttpError(400, "invalid_code", MSG.invalidCode);

  // Atomic: the code flips to claimed only if it is unexpired, unclaimed, its device is still
  // unlinked and not revoked, and the account has fewer than 3 active calculators. The device is
  // linked in the same transaction.
  const [upd] = await env.DB.batch([
    env.DB.prepare(
      `UPDATE pair_codes SET claimed_at = ?1
       WHERE code = ?2 AND claimed_at IS NULL AND expires_at > ?1
         AND device_id IN (SELECT id FROM devices WHERE user_id IS NULL AND revoked_at IS NULL)
         AND (SELECT COUNT(*) FROM devices WHERE user_id = ?3 AND revoked_at IS NULL) < ?4`,
    ).bind(now, code, user.id, MAX_ACTIVE_DEVICES),
    env.DB.prepare(
      `UPDATE devices SET user_id = ?1, label = COALESCE(?2, label, 'Calculator')
       WHERE id = (SELECT device_id FROM pair_codes WHERE code = ?3 AND claimed_at = ?4)
         AND user_id IS NULL AND revoked_at IS NULL`,
    ).bind(user.id, label, code, now),
  ]);
  if (upd?.meta.changes !== 1) {
    const active = await env.DB.prepare(
      `SELECT COUNT(*) AS n FROM devices WHERE user_id = ?1 AND revoked_at IS NULL`,
    )
      .bind(user.id)
      .first<{ n: number }>();
    if ((active?.n ?? 0) >= MAX_ACTIVE_DEVICES) throw new HttpError(409, "device_limit", MSG.deviceLimit);
    throw new HttpError(400, "invalid_code", MSG.invalidCode);
  }

  const dev = await env.DB.prepare(
    `SELECT ${DEVICE_COLS} FROM devices WHERE id = (SELECT device_id FROM pair_codes WHERE code = ?1)`,
  )
    .bind(code)
    .first<DeviceRow>();
  if (!dev) throw new Error("claimed device vanished");
  // The free month starts on the account's first calculator, once per account (decision D2).
  await env.DB.batch([
    env.DB.prepare(`UPDATE users SET trial_ends_at = ?1 WHERE id = ?2 AND trial_ends_at IS NULL`).bind(
      now + Math.round(cfg.trialDays * 86400),
      user.id,
    ),
    env.DB.prepare(`DELETE FROM pair_codes WHERE device_id = ?1 AND claimed_at IS NULL`).bind(dev.id),
  ]);
  return json(deviceJson(dev), 201);
}

async function ownedDeviceId(req: Request, id: string, userId: string, env: Env): Promise<DeviceRow> {
  if (!DEVICE_ID_RE.test(id)) throw new HttpError(404, "not_found", MSG.notFound);
  const d = await env.DB.prepare(`SELECT ${DEVICE_COLS} FROM devices WHERE id = ?1 AND user_id = ?2`)
    .bind(id, userId)
    .first<DeviceRow>();
  if (!d) throw new HttpError(404, "not_found", MSG.notFound); // also for other accounts' devices
  return d;
}

// ------------------------------------------------------------ PATCH /api/devices/{id}
export async function renameDevice(req: Request, env: Env, cfg: Config, id: string, now: number) {
  requireBrowserWrite(req, cfg);
  const user = await requireUser(req, env, now);
  const body = await readJsonObject(req, BODY_LIMIT);
  onlyKeys(body, ["label"]);
  const label = labelOrBad(body.label);
  const d = await ownedDeviceId(req, id, user.id, env);
  await env.DB.prepare(`UPDATE devices SET label = ?1 WHERE id = ?2 AND user_id = ?3`).bind(label, d.id, user.id).run();
  d.label = label;
  return json(deviceJson(d));
}

// ------------------------------------------------------------ POST /api/devices/{id}/revoke
export async function revokeDevice(req: Request, env: Env, cfg: Config, id: string, now: number) {
  requireBrowserWrite(req, cfg);
  const user = await requireUser(req, env, now);
  await readJsonObject(req, BODY_LIMIT, true); // body optional; nothing in it is read
  const d = await ownedDeviceId(req, id, user.id, env);
  await env.DB.batch([
    env.DB.prepare(`UPDATE devices SET revoked_at = ?1 WHERE id = ?2 AND revoked_at IS NULL`).bind(now, d.id),
    env.DB.prepare(`DELETE FROM pair_codes WHERE device_id = ?1 AND claimed_at IS NULL`).bind(d.id),
  ]);
  return empty(204);
}
