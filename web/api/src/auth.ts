// Magic-link sign-in and sessions for the portal (/api/auth/*).

import type { Config, Env } from "./env";
import { emailHash, newId, randomSecret, sha256Hex } from "./crypto";
import { HttpError, MSG, empty } from "./http";
import type { UserRow } from "./plan";
import { devAllowed } from "./dev";
import { clientIp, hitRateLimit, isEmail, onlyKeys, rateLimited, readJsonObject } from "./util";

export const LOGIN_TTL_SEC = 15 * 60;
export const SESSION_TTL_SEC = 30 * 86400;
const BODY_LIMIT = 16 * 1024;

/**
 * Browser CSRF guard for every state-changing /api/* call: Origin must be the site origin and
 * Content-Type must be JSON (PLAN / contract). 403 `forbidden` otherwise.
 */
export function requireBrowserWrite(req: Request, cfg: Config): void {
  const origin = req.headers.get("origin");
  const ct = (req.headers.get("content-type") ?? "").split(";")[0]!.trim().toLowerCase();
  if (origin !== cfg.publicOrigin || ct !== "application/json") {
    throw new HttpError(403, "forbidden", MSG.forbidden);
  }
}

function sessionCookie(cfg: Config, value: string, maxAge: number): string {
  return `sid=${value}; HttpOnly;${cfg.secureCookie ? " Secure;" : ""} SameSite=Lax; Path=/; Max-Age=${maxAge}`;
}

export function clearCookie(cfg: Config): string {
  return sessionCookie(cfg, "", 0);
}

function readSid(req: Request): string | null {
  const cookie = req.headers.get("cookie") ?? "";
  for (const part of cookie.split(";")) {
    const [k, ...v] = part.trim().split("=");
    if (k === "sid") {
      const val = v.join("=");
      return /^[A-Za-z0-9_-]{43}$/.test(val) ? val : null;
    }
  }
  return null;
}

/** The signed-in user, or 401 `unauthorized`. */
export async function requireUser(req: Request, env: Env, now: number): Promise<UserRow> {
  const sid = readSid(req);
  if (!sid) throw new HttpError(401, "unauthorized", MSG.signIn);
  const user = await env.DB.prepare(
    `SELECT u.* FROM sessions s JOIN users u ON u.id = s.user_id
     WHERE s.id_hash = ?1 AND s.expires_at > ?2 AND u.deleted_at IS NULL`,
  )
    .bind(await sha256Hex(sid), now)
    .first<UserRow>();
  if (!user) throw new HttpError(401, "unauthorized", MSG.signIn);
  return user;
}

// ------------------------------------------------------------ POST /api/auth/login
export async function login(req: Request, env: Env, cfg: Config, ctx: ExecutionContext, now: number) {
  requireBrowserWrite(req, cfg);
  const body = await readJsonObject(req, BODY_LIMIT);
  const turnstileSecret = (env.TURNSTILE_SECRET ?? "").trim();
  onlyKeys(body, turnstileSecret ? ["email", "age_confirmed", "turnstile_token"] : ["email", "age_confirmed"]);
  if (!isEmail(body.email)) throw new HttpError(400, "bad_request", "Please enter a valid e-mail address.");
  if (body.age_confirmed !== true) {
    throw new HttpError(400, "bad_request", "Please confirm the account holder is 18 or older.");
  }
  const email = body.email.trim().toLowerCase();

  const ipAddr = clientIp(req, devAllowed(req, cfg));
  const ip = await hitRateLimit(env.DB, `login:ip:${ipAddr}`, 3600, 20, now);
  if (!ip.ok) throw rateLimited(ip);
  // S5: optional bot check (Cloudflare Turnstile), only when TURNSTILE_SECRET is set.
  if (turnstileSecret) await verifyTurnstile(turnstileSecret, body.turnstile_token, ipAddr);
  const perEmail = await hitRateLimit(env.DB, `login:email:${await emailHash(cfg.trialPepper, email)}`, 3600, 5, now);
  if (!perEmail.ok) return empty(204); // silently dropped: same answer as success
  // S5: global cap on sign-in e-mails per UTC day (protects the Resend quota and reputation).
  const daily = await hitRateLimit(env.DB, "login:mail:day", 86400, cfg.mailDailyCap, now);
  if (!daily.ok) {
    console.error(JSON.stringify({ event: "mail_daily_cap_reached", cap: cfg.mailDailyCap }));
    return empty(204);
  }

  const token = randomSecret();
  await env.DB.prepare(
    `INSERT INTO login_tokens (token_hash, email, created_at, expires_at) VALUES (?1, ?2, ?3, ?4)`,
  )
    .bind(await sha256Hex(token), email, now, now + LOGIN_TTL_SEC)
    .run();
  const link = `${cfg.publicUrl}/login/confirm?token=${token}`;
  if (cfg.devMode) {
    await env.DB.prepare(`INSERT INTO dev_outbox (email, link, token, created_at) VALUES (?1, ?2, ?3, ?4)`)
      .bind(email, link, token, now)
      .run();
  }
  // Sent in the background so the response time doesn't depend on the mail provider.
  if ((env.RESEND_API_KEY ?? "").trim()) ctx.waitUntil(sendLoginMail(env, cfg, email, link));
  return empty(204);
}

const TURNSTILE_URL = "https://challenges.cloudflare.com/turnstile/v0/siteverify";

async function verifyTurnstile(secret: string, token: unknown, ip: string): Promise<void> {
  if (typeof token !== "string" || token.length < 1 || token.length > 2048) {
    throw new HttpError(403, "forbidden", "Please complete the check and try again.");
  }
  const form = new FormData();
  form.append("secret", secret);
  form.append("response", token);
  if (ip !== "unknown") form.append("remoteip", ip);
  let ok = false;
  try {
    const r = await fetch(TURNSTILE_URL, { method: "POST", body: form });
    const j = (await r.json()) as { success?: boolean };
    ok = r.ok && j.success === true;
  } catch (e) {
    console.error(JSON.stringify({ event: "turnstile_failed", detail: String(e).slice(0, 200) }));
    throw new HttpError(503, "unavailable", MSG.unavailable); // fail closed
  }
  if (!ok) throw new HttpError(403, "forbidden", "Please complete the check and try again.");
}

async function sendLoginMail(env: Env, cfg: Config, to: string, link: string): Promise<void> {
  const from = (env.MAIL_FROM ?? "").trim() || `AI Calculator <login@${new URL(cfg.publicUrl).hostname}>`;
  const text =
    `Sign in to your AI Calculator account:\n\n${link}\n\n` +
    `The link works once, for 15 minutes. If you didn't ask for it, ignore this e-mail.`;
  try {
    const r = await fetch("https://api.resend.com/emails", {
      method: "POST",
      headers: { authorization: `Bearer ${env.RESEND_API_KEY!.trim()}`, "content-type": "application/json" },
      body: JSON.stringify({ from, to: [to], subject: "Your AI Calculator sign-in link", text }),
    });
    if (!r.ok) console.error(JSON.stringify({ event: "mail_failed", status: r.status }));
    await r.body?.cancel().catch(() => {});
  } catch (e) {
    console.error(JSON.stringify({ event: "mail_failed", detail: String(e).slice(0, 200) }));
  }
}

// ------------------------------------------------------------ POST /api/auth/confirm
export async function confirm(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  requireBrowserWrite(req, cfg);
  const body = await readJsonObject(req, BODY_LIMIT);
  onlyKeys(body, ["token"]);
  const t = body.token;
  if (typeof t !== "string" || t.length < 32 || t.length > 128) {
    throw new HttpError(400, "bad_request", "The sign-in token is missing or malformed.");
  }
  // Single use: only one request can flip used_at.
  const row = await env.DB.prepare(
    `UPDATE login_tokens SET used_at = ?1 WHERE token_hash = ?2 AND used_at IS NULL AND expires_at > ?1
     RETURNING email`,
  )
    .bind(now, await sha256Hex(t))
    .first<{ email: string }>();
  if (!row) throw new HttpError(400, "invalid_token", MSG.invalidToken);

  // Creates the user on first sign-in.
  await env.DB.prepare(`INSERT INTO users (id, email, created_at) VALUES (?1, ?2, ?3) ON CONFLICT (email) DO NOTHING`)
    .bind(newId("usr_"), row.email, now)
    .run();
  const user = await env.DB.prepare(`SELECT id FROM users WHERE email = ?1 AND deleted_at IS NULL`)
    .bind(row.email)
    .first<{ id: string }>();
  if (!user) throw new HttpError(400, "invalid_token", MSG.invalidToken);

  const sid = randomSecret();
  await env.DB.prepare(`INSERT INTO sessions (id_hash, user_id, created_at, expires_at) VALUES (?1, ?2, ?3, ?4)`)
    .bind(await sha256Hex(sid), user.id, now, now + SESSION_TTL_SEC)
    .run();
  return empty(204, { "set-cookie": sessionCookie(cfg, sid, SESSION_TTL_SEC) });
}

// ------------------------------------------------------------ POST /api/auth/logout
export async function logout(req: Request, env: Env, cfg: Config): Promise<Response> {
  requireBrowserWrite(req, cfg);
  const sid = readSid(req);
  if (sid) await env.DB.prepare(`DELETE FROM sessions WHERE id_hash = ?1`).bind(await sha256Hex(sid)).run();
  return empty(204, { "set-cookie": clearCookie(cfg) });
}
