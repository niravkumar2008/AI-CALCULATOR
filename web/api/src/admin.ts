// POST /v1/admin/firmware: publish a firmware image to Workers KV (scripts/publish-firmware.mjs).

import type { Env } from "./env";
import { safeEqual, sha256Hex } from "./crypto";
import { FAMILY_RE, type FirmwareMeta, firmwareKeys } from "./device";
import { HttpError, MSG, json } from "./http";
import { onlyKeys, readBody, readJsonObject } from "./util";

export const MIN_IMAGE_BYTES = 100_000; // an ESP32-S3 Arduino image is well over this
export const MAX_IMAGE_BYTES = 0x1e0000; // the app slot size in min_spiffs.csv (1.9 MB)
const VERSION_RE = /^[0-9A-Za-z.\-]{1,32}$/;

export const MIN_ADMIN_TOKEN_CHARS = 32;

/** Admin API: off (404) unless ADMIN_TOKEN is at least 32 characters (S10); constant-time check. */
async function requireAdmin(req: Request, env: Env): Promise<void> {
  const adminToken = (env.ADMIN_TOKEN ?? "").trim();
  if (adminToken.length < MIN_ADMIN_TOKEN_CHARS) throw new HttpError(404, "not_found", MSG.notFound);
  const given = req.headers.get("x-admin-token") ?? "";
  if (!(await safeEqual(given, adminToken))) throw new HttpError(401, "unauthorized", "Admin token not accepted.");
}

const HW_ID_RE = /^calc-[0-9a-f]{12}$/;
export const HW_SECRET_RE = /^hs_[A-Za-z0-9_-]{43}$/;

/** POST /v1/admin/hardware: enrol one board at QA (scripts/enroll-board.mjs). */
export async function enrolHardware(req: Request, env: Env, now: number): Promise<Response> {
  await requireAdmin(req, env);
  const body = await readJsonObject(req, 4096);
  onlyKeys(body, ["hw_id", "hw_secret"]);
  if (typeof body.hw_id !== "string" || !HW_ID_RE.test(body.hw_id)) {
    throw new HttpError(400, "bad_request", "hw_id must look like calc-0123456789ab.");
  }
  if (typeof body.hw_secret !== "string" || !HW_SECRET_RE.test(body.hw_secret)) {
    throw new HttpError(400, "bad_request", "hw_secret must be hs_ followed by 43 base64url characters.");
  }
  // Re-enrolling replaces the secret but never resets trial_used_at.
  const row = await env.DB.prepare(
    `INSERT INTO hw_allow (hw_id, secret_hash, enrolled_at) VALUES (?1, ?2, ?3)
     ON CONFLICT (hw_id) DO UPDATE SET secret_hash = excluded.secret_hash, enrolled_at = excluded.enrolled_at
     RETURNING trial_used_at`,
  )
    .bind(body.hw_id, await sha256Hex(body.hw_secret), now)
    .first<{ trial_used_at: number | null }>();
  console.log(JSON.stringify({ event: "board_enrolled", hw_id: body.hw_id }));
  return json({ hw_id: body.hw_id, trial_used: row?.trial_used_at != null }, 201);
}

export async function publishFirmware(req: Request, env: Env): Promise<Response> {
  await requireAdmin(req, env);

  const q = new URL(req.url).searchParams;
  const version = q.get("version") ?? "";
  if (!VERSION_RE.test(version)) throw new HttpError(400, "bad_request", "version must be 1-32 of 0-9 A-Z a-z . -");
  // Optional channel: ?family=v15lcd publishes to that family only (devices ask with the
  // same ?family=). The version must then belong to it ("v15lcd-..."), as the firmware checks.
  const familyRaw = q.get("family");
  const family = familyRaw === null || familyRaw === "" ? null : familyRaw;
  if (family !== null && !FAMILY_RE.test(family)) throw new HttpError(400, "bad_request", "family must be 1-24 letters or digits.");
  if (family !== null && !version.startsWith(`${family}-`)) {
    throw new HttpError(400, "bad_request", `version must start with "${family}-" for family ${family}.`);
  }
  // Optional ?rollback=1: this (older) image replaces a bad newer one; devices may downgrade.
  const rb = q.get("rollback");
  if (rb !== null && !["", "0", "1", "true", "false"].includes(rb)) {
    throw new HttpError(400, "bad_request", "rollback must be 1, 0, true or false.");
  }
  const rollback = rb === "1" || rb === "true";
  const image = await readBody(
    req,
    MAX_IMAGE_BYTES,
    new HttpError(400, "bad_request", `Image is bigger than the ${MAX_IMAGE_BYTES} byte app slot.`),
  );
  if (image.byteLength < MIN_IMAGE_BYTES) throw new HttpError(400, "bad_request", "Image is too small to be firmware.");
  if (image[0] !== 0xe9) throw new HttpError(400, "bad_request", "Image doesn't start with the ESP32 magic byte 0xE9.");

  const meta: FirmwareMeta = { version, size: image.byteLength, sha256: await sha256Hex(image) };
  if (family !== null) meta.family = family;
  if (rollback) meta.rollback = true;
  const keys = firmwareKeys(family);
  // Image first, then the manifest that points devices at it.
  await env.FIRMWARE.put(keys.image, image, { metadata: meta });
  await env.FIRMWARE.put(keys.meta, JSON.stringify(meta));
  console.log(JSON.stringify({ event: "firmware_published", ...meta }));
  return json({ ...meta, family: family, rollback }, 201);
}
