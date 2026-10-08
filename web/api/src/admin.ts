// POST /v1/admin/firmware: publish a firmware image to Workers KV (scripts/publish-firmware.mjs).

import type { Env } from "./env";
import { safeEqual, sha256Hex } from "./crypto";
import type { FirmwareMeta } from "./device";
import { HttpError, MSG, json } from "./http";
import { readBody } from "./util";

export const MIN_IMAGE_BYTES = 100_000; // an ESP32-S3 Arduino image is well over this
export const MAX_IMAGE_BYTES = 0x1e0000; // the app slot size in min_spiffs.csv (1.9 MB)
const VERSION_RE = /^[0-9A-Za-z.\-]{1,32}$/;

export async function publishFirmware(req: Request, env: Env): Promise<Response> {
  const adminToken = (env.ADMIN_TOKEN ?? "").trim();
  if (!adminToken) throw new HttpError(404, "not_found", MSG.notFound); // admin API off
  const given = req.headers.get("x-admin-token") ?? "";
  if (!(await safeEqual(given, adminToken))) throw new HttpError(401, "unauthorized", "Admin token not accepted.");

  const version = new URL(req.url).searchParams.get("version") ?? "";
  if (!VERSION_RE.test(version)) throw new HttpError(400, "bad_request", "version must be 1-32 of 0-9 A-Z a-z . -");
  const image = await readBody(
    req,
    MAX_IMAGE_BYTES,
    new HttpError(400, "bad_request", `Image is bigger than the ${MAX_IMAGE_BYTES} byte app slot.`),
  );
  if (image.byteLength < MIN_IMAGE_BYTES) throw new HttpError(400, "bad_request", "Image is too small to be firmware.");
  if (image[0] !== 0xe9) throw new HttpError(400, "bad_request", "Image doesn't start with the ESP32 magic byte 0xE9.");

  const meta: FirmwareMeta = { version, size: image.byteLength, sha256: await sha256Hex(image) };
  // Image first, then the manifest that points devices at it.
  await env.FIRMWARE.put("image", image, { metadata: meta });
  await env.FIRMWARE.put("meta", JSON.stringify(meta));
  console.log(JSON.stringify({ event: "firmware_published", ...meta }));
  return json(meta, 201);
}
