// Pairing codes: 8 characters, 15 minutes, claimed atomically (see account.ts claim).

import { newPairCode } from "./crypto";

export const PAIR_CODE_TTL_SEC = 15 * 60;

/** Replaces any older unclaimed code of this device with a fresh one. Returns {code, expiresAt}. */
export async function issuePairCode(
  db: D1Database,
  deviceId: string,
  now: number,
): Promise<{ code: string; expiresAt: number }> {
  const expiresAt = now + PAIR_CODE_TTL_SEC;
  for (let attempt = 0; attempt < 5; attempt++) {
    const code = newPairCode();
    try {
      await db.batch([
        db.prepare(`DELETE FROM pair_codes WHERE device_id = ?1 AND claimed_at IS NULL`).bind(deviceId),
        db
          .prepare(`INSERT INTO pair_codes (code, device_id, expires_at) VALUES (?1, ?2, ?3)`)
          .bind(code, deviceId, expiresAt),
      ]);
      return { code, expiresAt };
    } catch (e) {
      // 32^8 codes: a collision is astronomically unlikely, but retry on a UNIQUE violation.
      if (!String(e).includes("UNIQUE")) throw e;
    }
  }
  throw new Error("could not allocate a pairing code");
}

/** The device's current unexpired, unclaimed code, or a new one. */
export async function currentPairCode(db: D1Database, deviceId: string, now: number): Promise<string> {
  const row = await db
    .prepare(
      `SELECT code FROM pair_codes WHERE device_id = ?1 AND claimed_at IS NULL AND expires_at > ?2
       ORDER BY expires_at DESC LIMIT 1`,
    )
    .bind(deviceId, now + 60) // at least a minute left to type it
    .first<{ code: string }>();
  if (row) return row.code;
  return (await issuePairCode(db, deviceId, now)).code;
}
