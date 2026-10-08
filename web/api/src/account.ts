// Portal account endpoints: /api/me, /api/devices/*.

import type { Config, Env } from "./env";
import { emailHash, normalisePairCode } from "./crypto";
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

  // ---- read-only pre-checks (the batch below re-checks everything atomically)
  const target = await env.DB.prepare(
    `SELECT d.id AS device_id, d.hw_id, d.enrolled, h.hw_id AS allowed_hw, h.trial_used_at
     FROM pair_codes p JOIN devices d ON d.id = p.device_id
     LEFT JOIN hw_allow h ON h.hw_id = d.hw_id
     WHERE p.code = ?1 AND p.claimed_at IS NULL AND p.expires_at > ?2
       AND d.user_id IS NULL AND d.revoked_at IS NULL`,
  )
    .bind(code, now)
    .first<{ device_id: string; hw_id: string | null; enrolled: number; allowed_hw: string | null; trial_used_at: number | null }>();
  if (!target) throw new HttpError(400, "invalid_code", MSG.invalidCode);
  const active = await env.DB.prepare(`SELECT COUNT(*) AS n FROM devices WHERE user_id = ?1 AND revoked_at IS NULL`)
    .bind(user.id)
    .first<{ n: number }>();
  if ((active?.n ?? 0) >= MAX_ACTIVE_DEVICES) throw new HttpError(409, "device_limit", MSG.deviceLimit);

  // Free month (decision D2, security review S1): only for an enrolled board whose trial was
  // never used, an account that never had one, and an e-mail (by hash) that never had one,
  // within the daily cap of new trials.
  const emailH = await emailHash(cfg.trialPepper, user.email);
  const dayStart = Math.floor(now / 86400) * 86400;
  const emailUsed = await env.DB.prepare(`SELECT 1 AS x FROM trials_used WHERE email_hash = ?1`).bind(emailH).first();
  const eligible =
    target.enrolled === 1 &&
    target.allowed_hw !== null &&
    target.trial_used_at === null &&
    user.trial_ends_at === null &&
    !emailUsed;
  const capError = () =>
    // Only the daily cap blocks: refuse without consuming the code, so nobody loses a trial.
    new HttpError(429, "rate_limited", "Too many new free months today. Please try again tomorrow.", {
      "retry-after": String(Math.max(1, dayStart + 86400 - now)),
    });
  const trialsToday = async () =>
    (await env.DB.prepare(`SELECT COUNT(*) AS n FROM trials_used WHERE used_at >= ?1`).bind(dayStart).first<{ n: number }>())
      ?.n ?? 0;
  if (eligible && (await trialsToday()) >= cfg.maxNewTrialsPerDay) throw capError();

  // ---- one transaction. The claim statements are the same for both paths.
  const claimStmts = (chained: boolean) => [
    env.DB.prepare(
      `UPDATE pair_codes SET claimed_at = ?1
       WHERE code = ?2 AND claimed_at IS NULL AND expires_at > ?1
         AND device_id IN (SELECT id FROM devices WHERE user_id IS NULL AND revoked_at IS NULL)
         AND (SELECT COUNT(*) FROM devices WHERE user_id = ?3 AND revoked_at IS NULL) < ?4
         ${chained ? "AND changes() = 1" : ""}`,
    ).bind(now, code, user.id, MAX_ACTIVE_DEVICES),
    env.DB.prepare(
      `UPDATE devices SET user_id = ?1, label = COALESCE(?2, label, 'Calculator')
       WHERE id = (SELECT device_id FROM pair_codes WHERE code = ?3 AND claimed_at = ?4)
         AND user_id IS NULL AND revoked_at IS NULL AND changes() = 1`,
    ).bind(user.id, label, code, now),
  ];

  let results: D1Result[];
  if (eligible) {
    // R2-5: for an eligible device the trial steps come FIRST and the code is claimed and the
    // device linked only if all of them changed a row (changes() = 1 chains each statement to
    // the one before). If the daily cap (or anything else) was hit between the pre-check and
    // this batch, nothing is written at all. R2-4: every step re-checks the board and device.
    results = await env.DB.batch([
      env.DB.prepare(
        `UPDATE hw_allow SET trial_used_at = ?1
         WHERE hw_id = ?2 AND trial_used_at IS NULL
           AND EXISTS (SELECT 1 FROM devices WHERE id = ?3 AND hw_id = ?2 AND enrolled = 1
                       AND user_id IS NULL AND revoked_at IS NULL)
           AND EXISTS (SELECT 1 FROM pair_codes WHERE code = ?8 AND device_id = ?3
                       AND claimed_at IS NULL AND expires_at > ?1)
           AND EXISTS (SELECT 1 FROM users WHERE id = ?4 AND trial_ends_at IS NULL AND deleted_at IS NULL)
           AND NOT EXISTS (SELECT 1 FROM trials_used WHERE email_hash = ?5)
           AND (SELECT COUNT(*) FROM trials_used WHERE used_at >= ?6) < ?7`,
      ).bind(now, target.allowed_hw, target.device_id, user.id, emailH, dayStart, cfg.maxNewTrialsPerDay, code),
      env.DB.prepare(
        `INSERT INTO trials_used (email_hash, used_at) SELECT ?1, ?2 WHERE changes() = 1
         ON CONFLICT (email_hash) DO NOTHING`,
      ).bind(emailH, now),
      env.DB.prepare(
        `UPDATE users SET trial_ends_at = ?1
         WHERE id = ?2 AND trial_ends_at IS NULL AND changes() = 1
           AND EXISTS (SELECT 1 FROM hw_allow WHERE hw_id = ?3 AND trial_used_at = ?4)`,
      ).bind(now + Math.round(cfg.trialDays * 86400), user.id, target.allowed_hw, now),
      ...claimStmts(true),
    ]);
    if (results[0]?.meta.changes !== 1) {
      // The trial wasn't granted and nothing was written. If the cap is why: 429, code kept.
      // Otherwise the device is no longer eligible (e.g. its trial was used a moment ago):
      // link it without a trial below.
      if ((await trialsToday()) >= cfg.maxNewTrialsPerDay) throw capError();
      results = await env.DB.batch(claimStmts(false));
    } else if (results[3]?.meta.changes !== 1 || results[4]?.meta.changes !== 1) {
      throw new Error("claim batch: trial granted but link failed"); // impossible by the chain above
    } else {
      results = results.slice(3);
    }
  } else {
    results = await env.DB.batch(claimStmts(false));
  }
  if (results[0]?.meta.changes !== 1 || results[1]?.meta.changes !== 1) {
    const again = await env.DB.prepare(`SELECT COUNT(*) AS n FROM devices WHERE user_id = ?1 AND revoked_at IS NULL`)
      .bind(user.id)
      .first<{ n: number }>();
    if ((again?.n ?? 0) >= MAX_ACTIVE_DEVICES) throw new HttpError(409, "device_limit", MSG.deviceLimit);
    throw new HttpError(400, "invalid_code", MSG.invalidCode);
  }

  const dev = await env.DB.prepare(`SELECT ${DEVICE_COLS} FROM devices WHERE id = ?1`)
    .bind(target.device_id)
    .first<DeviceRow>();
  if (!dev) throw new Error("claimed device vanished");
  await env.DB.prepare(`DELETE FROM pair_codes WHERE device_id = ?1 AND claimed_at IS NULL`).bind(dev.id).run();
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
