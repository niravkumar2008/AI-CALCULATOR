// QR linking, way 2 (docs/LINK_QR_CONTRACT.md): the signed-in phone gets a link token and
// shows it as a QR, the calculator scans and redeems it, the phone taps Approve.
//   POST /api/link-tokens               (phone)  new token, 10 minutes
//   GET  /api/link-tokens/{id}          (phone)  state, polled every 2 s
//   POST /api/link-tokens/{id}/approve  (phone)  Approve / No
//   POST /v1/pair/redeem                (calculator, Bearer dt_...)
// The link itself goes through the same claim engine as /api/devices/claim (account.ts).

import type { Config, Env } from "./env";
import { LINK_TOKEN_RE, deviceSuffix, newId, newLinkToken, sha256Hex } from "./crypto";
import { HttpError, MSG, json } from "./http";
import { requireBrowserWrite, requireUser, sendMail } from "./auth";
import { type ClaimGate, type ClaimTarget, type DeviceBrief, activeDevices, linkToUser } from "./account";
import { authDevice } from "./device";
import { devAllowed } from "./dev";
import { clientIp, hitRateLimit, onlyKeys, rateLimited, readJsonObject } from "./util";

export const LINK_TOKEN_TTL_SEC = 10 * 60;
/** After the calculator redeems a token, the phone has this long to tap Approve. */
export const APPROVE_WINDOW_SEC = 5 * 60;
/** The calculator sees link_state "declined" this long after a No. */
export const DECLINED_SHOW_SEC = 10 * 60;
const BODY_LIMIT = 4096;
const LINK_ID_RE = /^lt_[0-9a-f]{16}$/;

export type LinkState = "waiting" | "pending_approval" | "approved" | "declined" | "expired";

interface LinkTokenRow {
  id: string;
  user_id: string;
  created_at: number;
  expires_at: number;
  used_at: number | null;
  used_by_device_id: string | null;
  approve_by: number | null;
  approved_at: number | null;
  declined_at: number | null;
  cancelled_at: number | null;
}

const LT_COLS = `id, user_id, created_at, expires_at, used_at, used_by_device_id, approve_by, approved_at, declined_at, cancelled_at`;

/** A cancelled token (the user made a newer one) reads as expired: the phone shows "New code". */
export function linkStateOf(t: LinkTokenRow, now: number): LinkState {
  if (t.approved_at !== null) return "approved";
  if (t.declined_at !== null) return "declined";
  if (t.used_at !== null) return t.approve_by !== null && now <= t.approve_by ? "pending_approval" : "expired";
  if (t.cancelled_at !== null) return "expired";
  return now < t.expires_at ? "waiting" : "expired";
}

async function deviceBrief(db: D1Database, id: string | null): Promise<DeviceBrief | null> {
  if (!id) return null;
  const d = await db
    .prepare(`SELECT id, hw_id, label FROM devices WHERE id = ?1`)
    .bind(id)
    .first<{ id: string; hw_id: string | null; label: string | null }>();
  return d ? { id: d.id, suffix: deviceSuffix(d.hw_id, d.id), label: d.label ?? "Calculator" } : null;
}

async function ownedToken(env: Env, id: string, userId: string): Promise<LinkTokenRow> {
  if (!LINK_ID_RE.test(id)) throw new HttpError(404, "not_found", MSG.notFound);
  const t = await env.DB.prepare(`SELECT ${LT_COLS} FROM link_tokens WHERE id = ?1 AND user_id = ?2`)
    .bind(id, userId)
    .first<LinkTokenRow>();
  if (!t) throw new HttpError(404, "not_found", MSG.notFound); // also for other accounts' tokens
  return t;
}

// ------------------------------------------------------------ POST /api/link-tokens
export async function createLinkToken(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  requireBrowserWrite(req, cfg);
  const user = await requireUser(req, env, now);
  onlyKeys(await readJsonObject(req, BODY_LIMIT, true), []);
  const rl = await hitRateLimit(env.DB, `linktoken:user:${user.id}`, 3600, 10, now);
  if (!rl.ok) throw rateLimited(rl, "Too many new codes. Please wait a while and try again.");

  const id = newId("lt_");
  const token = newLinkToken();
  const expiresAt = now + LINK_TOKEN_TTL_SEC;
  await env.DB.batch([
    // Only the newest code works: earlier unused ones are cancelled.
    env.DB.prepare(
      `UPDATE link_tokens SET cancelled_at = ?1 WHERE user_id = ?2 AND used_at IS NULL AND cancelled_at IS NULL`,
    ).bind(now, user.id),
    env.DB.prepare(
      `INSERT INTO link_tokens (id, user_id, token_hash, created_at, expires_at, ip) VALUES (?1, ?2, ?3, ?4, ?5, ?6)`,
    ).bind(id, user.id, await sha256Hex(token), now, expiresAt, clientIp(req, devAllowed(req, cfg))),
  ]);
  return json({ id, token, qr_url: `${cfg.publicUrl}/q/${token}`, expires_at: expiresAt }, 201);
}

// ------------------------------------------------------------ GET /api/link-tokens/{id}
export async function getLinkToken(req: Request, env: Env, id: string, now: number): Promise<Response> {
  const user = await requireUser(req, env, now);
  const t = await ownedToken(env, id, user.id);
  const state = linkStateOf(t, now);
  // The calculator approving would remove (1-device slot). Only while a decision is ahead.
  let replaces: DeviceBrief | null = null;
  if (state === "waiting" || state === "pending_approval") {
    replaces = (await activeDevices(env.DB, user.id, t.used_by_device_id ?? ""))[0] ?? null;
  }
  return json({
    state,
    device: await deviceBrief(env.DB, t.used_by_device_id),
    replaces,
    approve_by: t.approve_by,
  });
}

// ------------------------------------------------------------ POST /api/link-tokens/{id}/approve
export async function approveLinkToken(
  req: Request,
  env: Env,
  cfg: Config,
  ctx: ExecutionContext,
  id: string,
  now: number,
): Promise<Response> {
  requireBrowserWrite(req, cfg);
  const user = await requireUser(req, env, now);
  const body = await readJsonObject(req, BODY_LIMIT);
  onlyKeys(body, ["approve"]);
  if (typeof body.approve !== "boolean") throw new HttpError(400, "bad_request", "approve must be true or false.");
  const t = await ownedToken(env, id, user.id);
  const stateError = () => new HttpError(409, "link_token_state", MSG.linkTokenState);
  if (linkStateOf(t, now) !== "pending_approval" || !t.used_by_device_id) throw stateError();

  if (!body.approve) {
    const r = await env.DB.prepare(
      `UPDATE link_tokens SET declined_at = ?1
       WHERE id = ?2 AND approved_at IS NULL AND declined_at IS NULL AND approve_by >= ?1`,
    )
      .bind(now, t.id)
      .run();
    if (r.meta.changes !== 1) throw stateError();
    return json({ state: "declined" });
  }

  const deviceId = t.used_by_device_id;
  const target = await env.DB.prepare(
    `SELECT d.id AS device_id, d.hw_id, d.enrolled, h.hw_id AS allowed_hw, h.trial_used_at
     FROM devices d LEFT JOIN hw_allow h ON h.hw_id = d.hw_id
     WHERE d.id = ?1 AND d.user_id IS NULL AND d.revoked_at IS NULL`,
  )
    .bind(deviceId)
    .first<ClaimTarget>();
  if (!target) throw stateError(); // linked some other way (or removed) since the scan

  const gate: ClaimGate = {
    openSql: `EXISTS (SELECT 1 FROM link_tokens WHERE id = ?8 AND used_by_device_id = ?3 AND approved_at IS NULL
                AND declined_at IS NULL AND approve_by >= ?1)`,
    openBinds: [t.id],
    consume: (userId, limit, chained) =>
      env.DB.prepare(
        `UPDATE link_tokens SET approved_at = ?1
         WHERE id = ?2 AND user_id = ?3 AND used_by_device_id = ?5
           AND approved_at IS NULL AND declined_at IS NULL AND approve_by >= ?1
           AND ?5 IN (SELECT id FROM devices WHERE user_id IS NULL AND revoked_at IS NULL)
           AND (SELECT COUNT(*) FROM devices WHERE user_id = ?3 AND revoked_at IS NULL) < ?4
           ${chained ? "AND changes() = 1" : ""}`,
      ).bind(now, t.id, userId, limit, deviceId),
    failed: stateError,
  };
  // The phone showed the replace warning before the tap, so approving always replaces.
  const dev = await linkToUser(env, cfg, user, target, gate, null, true, now);
  const brief: DeviceBrief = { id: dev.id, suffix: deviceSuffix(dev.hw_id, dev.id), label: dev.label ?? "Calculator" };
  await sendLinkedMail(env, cfg, ctx, user.email, brief, now);
  return json({ state: "approved", device: brief });
}

/** "A calculator was linked to your account": a security notice after every camera link. */
async function sendLinkedMail(env: Env, cfg: Config, ctx: ExecutionContext, email: string, d: DeviceBrief, now: number) {
  const accountUrl = `${cfg.publicUrl}/account/`;
  if (cfg.devMode) {
    await env.DB.prepare(
      `INSERT INTO dev_outbox (email, link, token, created_at, kind) VALUES (?1, ?2, '', ?3, 'linked')`,
    )
      .bind(email, accountUrl, now)
      .run();
  }
  if (!(env.RESEND_API_KEY ?? "").trim()) return;
  // Shares the daily cap of sign-in e-mails (protects the mail quota and reputation).
  const daily = await hitRateLimit(env.DB, "login:mail:day", 86400, cfg.mailDailyCap, now);
  if (!daily.ok) return;
  const text =
    `A calculator (Calculator ID ending ${d.suffix}) was linked to your Proofline account.\n\n` +
    `If that was you, there is nothing to do. If it wasn't, remove it on your account page and sign out ` +
    `everywhere:\n\n${accountUrl}`;
  ctx.waitUntil(sendMail(env, cfg, email, "A calculator was linked to your account", text));
}

// ------------------------------------------------------------ POST /v1/pair/redeem
export async function pairRedeem(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  const dev = await authDevice(req, env, now);
  const devLimit = await hitRateLimit(env.DB, `redeem:dev:${dev.id}`, 60, 10, now);
  if (!devLimit.ok) throw rateLimited(devLimit, "Too many tries. Please wait a minute.");
  const ipLimit = await hitRateLimit(env.DB, `redeem:ip:${clientIp(req, devAllowed(req, cfg))}`, 60, 30, now);
  if (!ipLimit.ok) throw rateLimited(ipLimit, "Too many tries. Please wait a minute.");

  const body = await readJsonObject(req, BODY_LIMIT);
  onlyKeys(body, ["link_token"]);
  const invalid = () => new HttpError(404, "link_token_invalid", MSG.linkTokenInvalid);
  if (typeof body.link_token !== "string" || !LINK_TOKEN_RE.test(body.link_token)) throw invalid();
  const t = await env.DB.prepare(`SELECT ${LT_COLS} FROM link_tokens WHERE token_hash = ?1`)
    .bind(await sha256Hex(body.link_token))
    .first<LinkTokenRow>();
  if (!t) throw invalid();
  const suffix = deviceSuffix(dev.hw_id, dev.id);

  if (t.used_at !== null) {
    // The same calculator asking again (e.g. a lost reply) gets the same answer.
    if (t.used_by_device_id === dev.id) {
      if (dev.user_id === t.user_id) return json({ state: "linked" });
      if (linkStateOf(t, now) === "pending_approval") {
        return json({ state: "pending_approval", approve_by: t.approve_by, suffix });
      }
    }
    throw new HttpError(409, "link_token_used", MSG.linkTokenUsed);
  }
  if (linkStateOf(t, now) !== "waiting") throw new HttpError(410, "link_token_expired", MSG.linkTokenExpired);

  if (dev.user_id !== null) {
    if (dev.user_id !== t.user_id) throw new HttpError(409, "already_linked", MSG.alreadyLinked);
    // Already on this account: nothing to approve. The token is used up all the same, and the
    // phone's poll sees "approved" (approved_at is set too, no tap needed).
    await env.DB.prepare(
      `UPDATE link_tokens SET used_at = ?1, used_by_device_id = ?2, approved_at = ?1 WHERE id = ?3 AND used_at IS NULL`,
    )
      .bind(now, dev.id, t.id)
      .run();
    return json({ state: "linked" });
  }

  const approveBy = now + APPROVE_WINDOW_SEC;
  const r = await env.DB.prepare(
    `UPDATE link_tokens SET used_at = ?1, used_by_device_id = ?2, approve_by = ?3
     WHERE id = ?4 AND used_at IS NULL AND cancelled_at IS NULL AND expires_at > ?1`,
  )
    .bind(now, dev.id, approveBy, t.id)
    .run();
  if (r.meta.changes !== 1) throw new HttpError(409, "link_token_used", MSG.linkTokenUsed); // lost a race
  return json({ state: "pending_approval", approve_by: approveBy, suffix });
}

// ------------------------------------------------------------ link_state for /v1/device/status
/**
 * "pending_approval" while a token this calculator redeemed waits for the tap (until
 * approve_by), "declined" for 10 minutes after a No, else "".
 */
export async function deviceLinkState(db: D1Database, deviceId: string, now: number): Promise<"pending_approval" | "declined" | ""> {
  const t = await db
    .prepare(`SELECT ${LT_COLS} FROM link_tokens WHERE used_by_device_id = ?1 ORDER BY used_at DESC LIMIT 1`)
    .bind(deviceId)
    .first<LinkTokenRow>();
  if (!t) return "";
  const s = linkStateOf(t, now);
  if (s === "pending_approval") return s;
  if (s === "declined" && t.declined_at !== null && now < t.declined_at + DECLINED_SHOW_SEC) return "declined";
  return "";
}
