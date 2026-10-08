// Random ids and secrets, hashing and constant-time comparison (Web Crypto only).

const enc = new TextEncoder();

export function randomBytes(n: number): Uint8Array {
  const b = new Uint8Array(n);
  crypto.getRandomValues(b);
  return b;
}

export function base64url(bytes: Uint8Array): string {
  let s = "";
  for (const b of bytes) s += String.fromCharCode(b);
  return btoa(s).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");
}

export function hex(bytes: Uint8Array): string {
  let s = "";
  for (const b of bytes) s += b.toString(16).padStart(2, "0");
  return s;
}

/** 32 random bytes as base64url: 43 characters. */
export function randomSecret(): string {
  return base64url(randomBytes(32));
}

/** Device token: 'dt_' + 43 base64url characters. Only its SHA-256 is stored. */
export function newDeviceToken(): string {
  return "dt_" + randomSecret();
}

export const DEVICE_TOKEN_RE = /^dt_[A-Za-z0-9_-]{43}$/;

export function newId(prefix: "dev_" | "usr_" | "use_"): string {
  return prefix + hex(randomBytes(8));
}

export async function sha256Hex(data: string | Uint8Array | ArrayBuffer): Promise<string> {
  const buf = typeof data === "string" ? enc.encode(data) : data;
  return hex(new Uint8Array(await crypto.subtle.digest("SHA-256", buf)));
}

/** Constant-time string comparison (hashes both sides so lengths don't leak either). */
export async function safeEqual(a: string, b: string): Promise<boolean> {
  const [ha, hb] = await Promise.all([
    crypto.subtle.digest("SHA-256", enc.encode(a)),
    crypto.subtle.digest("SHA-256", enc.encode(b)),
  ]);
  const x = new Uint8Array(ha);
  const y = new Uint8Array(hb);
  let diff = 0;
  for (let i = 0; i < x.length; i++) diff |= x[i]! ^ y[i]!;
  return diff === 0 && a.length === b.length;
}

// ------------------------------------------------------------ pairing codes
export const PAIR_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // 32 chars: no O/0/I/1

/** 8 characters, stored without the dash. 256 % 32 == 0, so `b & 31` is unbiased. */
export function newPairCode(): string {
  let s = "";
  for (const b of randomBytes(8)) s += PAIR_ALPHABET[b & 31];
  return s;
}

export function formatPairCode(code: string): string {
  return code.slice(0, 4) + "-" + code.slice(4);
}

/** Upper-case, remove dashes and spaces. Returns null when it can't be a valid code. */
export function normalisePairCode(input: string): string | null {
  const s = input.toUpperCase().replace(/[\s-]/g, "");
  return /^[A-HJ-NP-Z2-9]{8}$/.test(s) ? s : null;
}
