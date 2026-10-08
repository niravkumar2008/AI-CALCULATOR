// Calculator endpoints except /v1/solve: register, pair/start, status, firmware.

import type { Config, Env } from "./env";
import { DEVICE_TOKEN_RE, formatPairCode, newDeviceToken, newId, sha256Hex } from "./crypto";
import { HttpError, MSG, json } from "./http";
import { issuePairCode } from "./pairing";
import {
  type DeviceRow,
  type PlanInfo,
  type UserRow,
  globalSpendToday,
  loadUser,
  monthUsage,
  notEntitledError,
  notLinkedError,
  planOf,
  quotaError,
  spendCapError,
  worstCaseMicro,
} from "./plan";
import { HW_SECRET_RE } from "./admin";
import { devAllowed } from "./dev";
import { clientIp, hitRateLimit, onlyKeys, rateLimited, readJsonObject } from "./util";

const HW_ID_RE = /^calc-[0-9a-f]{12}$/;

/** Bearer token -> device row, or 401. Also records last_seen / firmware / x-device-id. */
export async function authDevice(req: Request, env: Env, now: number): Promise<DeviceRow> {
  const auth = req.headers.get("authorization") ?? "";
  const m = /^Bearer\s+(\S+)$/i.exec(auth.trim());
  const token = m?.[1] ?? "";
  if (!DEVICE_TOKEN_RE.test(token)) throw new HttpError(401, "unauthorized", MSG.unknownDevice);
  const dev = await env.DB.prepare(
    `SELECT id, hw_id, fw_version, user_id, label, created_at, last_seen_at, revoked_at, enrolled
     FROM devices WHERE token_hash = ?1`,
  )
    .bind(await sha256Hex(token))
    .first<DeviceRow>();
  if (!dev) throw new HttpError(401, "unauthorized", MSG.unknownDevice);
  if (dev.revoked_at !== null) throw new HttpError(401, "device_revoked", MSG.revoked);

  const hw = req.headers.get("x-device-id")?.trim();
  const fw = req.headers.get("x-firmware")?.trim();
  // An enrolled board's hw_id is fixed at registration (it keys the once-per-board trial).
  const newHw = dev.enrolled !== 1 && hw && HW_ID_RE.test(hw) ? hw : dev.hw_id;
  const newFw = fw && fw.length <= 32 && /^[\x20-\x7e]+$/.test(fw) ? fw : dev.fw_version;
  // Write at most once a minute unless something changed (review item M2).
  if (newHw !== dev.hw_id || newFw !== dev.fw_version || dev.last_seen_at === null || now - dev.last_seen_at >= 60) {
    await env.DB.prepare(`UPDATE devices SET last_seen_at = ?1, hw_id = ?2, fw_version = ?3 WHERE id = ?4`)
      .bind(now, newHw, newFw, dev.id)
      .run();
    dev.last_seen_at = now;
    dev.hw_id = newHw;
    dev.fw_version = newFw;
  }
  return dev;
}

function linkUrl(cfg: Config, code: string): string {
  return `${cfg.publicUrl}/link?code=${formatPairCode(code)}`;
}

// ------------------------------------------------------------ POST /v1/device/register
export async function register(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  const body = await readJsonObject(req, 4096);
  onlyKeys(body, ["hw_id", "hw_secret"]);
  if (typeof body.hw_id !== "string" || !HW_ID_RE.test(body.hw_id)) {
    throw new HttpError(400, "bad_request", "hw_id must look like calc-0123456789ab.");
  }
  if (body.hw_secret !== undefined && (typeof body.hw_secret !== "string" || !HW_SECRET_RE.test(body.hw_secret))) {
    throw new HttpError(400, "bad_request", "hw_secret is malformed.");
  }
  const rl = await hitRateLimit(env.DB, `register:ip:${clientIp(req, devAllowed(req, cfg))}`, 3600, 5, now);
  if (!rl.ok) throw rateLimited(rl, "Too many registrations from this network. Try again in an hour.");

  const fw = req.headers.get("x-firmware")?.trim();
  const fwVersion = fw && fw.length <= 32 && /^[\x20-\x7e]+$/.test(fw) ? fw : null;
  // S1: enrolled only with the secret registered for this board at QA. A wrong or missing
  // secret gives the same response, just an unenrolled device (it can link but gets no trial).
  let enrolled = 0;
  if (typeof body.hw_secret === "string") {
    const allow = await env.DB.prepare(`SELECT 1 AS x FROM hw_allow WHERE hw_id = ?1 AND secret_hash = ?2`)
      .bind(body.hw_id, await sha256Hex(body.hw_secret))
      .first();
    enrolled = allow ? 1 : 0;
  }
  const deviceId = newId("dev_");
  const token = newDeviceToken();
  await env.DB.prepare(
    `INSERT INTO devices (id, token_hash, hw_id, fw_version, user_id, label, created_at, last_seen_at, enrolled)
     VALUES (?1, ?2, ?3, ?4, NULL, NULL, ?5, ?5, ?6)`,
  )
    .bind(deviceId, await sha256Hex(token), body.hw_id, fwVersion, now, enrolled)
    .run();
  const { code, expiresAt } = await issuePairCode(env.DB, deviceId, now);
  return json(
    {
      device_id: deviceId,
      token,
      pair_code: formatPairCode(code),
      link_url: linkUrl(cfg, code),
      expires_at: expiresAt,
    },
    201,
  );
}

// ------------------------------------------------------------ POST /v1/pair/start
export async function pairStart(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  const dev = await authDevice(req, env, now);
  if (dev.user_id !== null) return json({ already_linked: true });
  const rl = await hitRateLimit(env.DB, `pair:dev:${dev.id}`, 3600, 20, now);
  if (!rl.ok) throw rateLimited(rl);
  const { code, expiresAt } = await issuePairCode(env.DB, dev.id, now);
  return json({ already_linked: false, code: formatPairCode(code), link_url: linkUrl(cfg, code), expires_at: expiresAt });
}

// ------------------------------------------------------------ "can this device solve?"
export interface SolveContext {
  user: UserRow;
  plan: PlanInfo;
}

/**
 * The checks before the atomic reservation, in contract order: linked -> entitled.
 * Returns the user and plan, or throws the error the calculator should show.
 */
export async function checkLinkedAndEntitled(
  env: Env,
  cfg: Config,
  dev: DeviceRow,
  now: number,
): Promise<SolveContext> {
  const user = dev.user_id ? await loadUser(env.DB, dev.user_id) : null;
  if (!user) throw await notLinkedError(env.DB, cfg, dev.id, now);
  const plan = await planOf(env.DB, user, now);
  if (!plan.entitled) throw notEntitledError(cfg, plan);
  return { user, plan };
}

/** Monthly limits and the global cap (non-atomic view, for status and error messages). */
export async function checkLimits(env: Env, cfg: Config, userId: string, now: number, images = 1): Promise<void> {
  const worst = worstCaseMicro(cfg, images);
  const u = await monthUsage(env.DB, userId, now);
  if (u.solves >= cfg.monthlySolves || u.spendMicro + worst > cfg.monthlySpendMicro) throw quotaError(now);
  if ((await globalSpendToday(env.DB, now)) + worst > cfg.globalDailyCapMicro) throw spendCapError(now);
}

function maskEmail(email: string): string {
  const at = email.lastIndexOf("@");
  if (at < 1) return "***";
  const masked = `${email[0]}***@${email.slice(at + 1)}`;
  return masked.length <= 40 ? masked : masked.slice(0, 37) + "...";
}

// ------------------------------------------------------------ GET /v1/device/status
export async function status(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  const dev = await authDevice(req, env, now);
  const user = dev.user_id ? await loadUser(env.DB, dev.user_id) : null;
  if (!user) {
    const e = await notLinkedError(env.DB, cfg, dev.id, now);
    return json({ linked: false, plan: "none", can_solve: false, reason: e.userMessage });
  }
  const plan = await planOf(env.DB, user, now);
  const usage = await monthUsage(env.DB, user.id, now);
  const out: Record<string, unknown> = {
    linked: true,
    plan: plan.plan,
    can_solve: true,
    account_hint: maskEmail(user.email),
    solves_this_month: usage.solves,
    solves_limit: cfg.monthlySolves,
  };
  if (plan.trialEndsAt !== undefined) out.trial_ends_at = plan.trialEndsAt;
  if (plan.periodEnd !== undefined) out.period_end = plan.periodEnd;
  try {
    if (!plan.entitled) throw notEntitledError(cfg, plan);
    await checkLimits(env, cfg, user.id, now);
  } catch (e) {
    if (!(e instanceof HttpError)) throw e;
    out.can_solve = false;
    out.reason = e.userMessage;
  }
  return json(out);
}

// ------------------------------------------------------------ firmware (Workers KV)
export interface FirmwareMeta {
  version: string;
  size: number;
  sha256: string;
}

export const FIRMWARE_PATH = "/v1/firmware/image";

export async function firmwareCheck(req: Request, env: Env, now: number): Promise<Response> {
  await authDevice(req, env, now);
  const meta = await env.FIRMWARE.get<FirmwareMeta>("meta", "json");
  if (!meta) return json({ update: false });
  const current = (req.headers.get("x-firmware") ?? "").trim();
  return json({
    update: meta.version !== current,
    version: meta.version,
    size: meta.size,
    sha256: meta.sha256,
    path: FIRMWARE_PATH,
  });
}

export async function firmwareImage(req: Request, env: Env, now: number): Promise<Response> {
  await authDevice(req, env, now);
  // The image carries its own version and hash as KV metadata, so a download never mixes a
  // new manifest with an old image while KV propagates.
  const { value, metadata } = await env.FIRMWARE.getWithMetadata<FirmwareMeta>("image", "stream");
  if (!value || !metadata) throw new HttpError(404, "not_found", "No firmware is published.");
  return new Response(value, {
    headers: {
      "content-type": "application/octet-stream",
      "content-length": String(metadata.size),
      "cache-control": "no-store",
      "x-firmware-version": metadata.version,
      "x-firmware-sha256": metadata.sha256,
    },
  });
}
