// POST /api/waitlist: the launch site's "Join the waitlist" forms (web/site).
//
// - JSON from the site's script ({email, age_ok: true, source, utm_*, ref}) answers 201 {ok:true}.
// - A plain HTML form post (no JavaScript) answers 303 back to the site (`age_ok=on`).
// - The answer is the same whether or not the address was already on the list, so the form
//   can't be used to find out who signed up. A repeat sign-up changes nothing (first touch wins).
// - "I'm 13 or older" must be confirmed (`age_ok`), or 400: we don't knowingly collect e-mail
//   addresses from under-13s (COPPA).
// - Tracking: `source` is which form (hero, footer, site). `utm_source`, `utm_medium`,
//   `utm_campaign` and `ref` (a referral code) come from the page link; each is lower-cased and
//   kept only if it matches [a-z0-9_-]{1,40}. A value that doesn't match is dropped (stored as
//   NULL) rather than failing the sign-up; a non-string value is a 400.
// - Origin must be the site (same CSRF rule as the portal), 5 sign-ups per IP per hour, and a
//   global daily cap of new addresses (WAITLIST_DAILY_CAP, default 20,000) checked before any
//   write. The cap is only an abuse backstop (Turnstile is the real bot check, required before
//   launch); when it is hit, nothing is stored and the answer is 429 "busy, try tomorrow".
// - A filled-in `company` field (the hidden honeypot) is accepted and silently dropped.

import type { Config, Env } from "./env";
import { HttpError, MSG, empty, json } from "./http";
import { devAllowed } from "./dev";
import { clientIp, hitRateLimit, isEmail, peekRateLimit, rateLimited, readBody } from "./util";

const BODY_LIMIT = 4 * 1024;
const SOURCES = new Set(["hero", "footer", "site"]);
const TRACKING = ["utm_source", "utm_medium", "utm_campaign", "ref"] as const;
const FIELDS = new Set(["email", "source", "company", "age_ok", ...TRACKING]);
const TAG_RE = /^[a-z0-9_-]{1,40}$/;
export const AGE_MSG = "Please confirm you are 13 or older to join the waitlist.";

function parseBody(ct: string, bytes: Uint8Array): Record<string, unknown> {
  const text = new TextDecoder().decode(bytes);
  if (ct === "application/x-www-form-urlencoded") {
    return Object.fromEntries(new URLSearchParams(text));
  }
  let v: unknown;
  try {
    v = JSON.parse(text);
  } catch {
    throw new HttpError(400, "bad_request", "The request body is not valid JSON.");
  }
  if (typeof v !== "object" || v === null || Array.isArray(v)) {
    throw new HttpError(400, "bad_request", "The request body must be a JSON object.");
  }
  return v as Record<string, unknown>;
}

/** A tracking tag: lower-cased and kept only if it is short and plain; otherwise null. */
export function tag(v: unknown, name: string): string | null {
  if (v === undefined || v === null) return null;
  if (typeof v !== "string") throw new HttpError(400, "bad_request", `${name} must be a string.`);
  const s = v.trim().toLowerCase();
  return TAG_RE.test(s) ? s : null;
}

export async function joinWaitlist(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  const ct = (req.headers.get("content-type") ?? "").split(";")[0]!.trim().toLowerCase();
  const isForm = ct === "application/x-www-form-urlencoded";
  // Same-origin only (CSRF); JSON or a classic form post.
  if (req.headers.get("origin") !== cfg.publicOrigin || (ct !== "application/json" && !isForm)) {
    throw new HttpError(403, "forbidden", MSG.forbidden);
  }
  const body = parseBody(ct, await readBody(req, BODY_LIMIT, new HttpError(413, "payload_too_large", "Request too large.")));
  for (const k of Object.keys(body)) {
    if (!FIELDS.has(k)) throw new HttpError(400, "bad_request", `Unknown field: ${k.slice(0, 40)}`);
  }
  const email = typeof body.email === "string" ? body.email.trim().toLowerCase() : "";
  if (!isEmail(email)) throw new HttpError(400, "bad_request", "Enter a valid email address.");
  // JSON: exactly `true`. A form checkbox sends "on" when ticked and nothing when not.
  const ageOk = isForm ? body.age_ok === "on" : body.age_ok === true;
  if (!ageOk) throw new HttpError(400, "bad_request", AGE_MSG);
  const source = typeof body.source === "string" && SOURCES.has(body.source) ? body.source : "site";
  const [utmSource, utmMedium, utmCampaign, ref] = TRACKING.map((k) => tag(body[k], k));

  const done = () =>
    isForm ? empty(303, { location: `${cfg.publicUrl}/?waitlist=joined#waitlist` }) : json({ ok: true }, 201);

  // Honeypot: a bot filled the hidden field. Look successful, store nothing.
  if (typeof body.company === "string" && body.company.trim() !== "") return done();

  // The daily cap is checked first and records nothing (not even the per-IP counter) when full.
  const global = await peekRateLimit(env.DB, "waitlist:day", 86400, cfg.waitlistDailyCap, now);
  if (!global.ok) throw rateLimited(global, "The waitlist is busy today. Please try again tomorrow.");
  const ip = await hitRateLimit(env.DB, `waitlist:ip:${clientIp(req, devAllowed(req, cfg))}`, 3600, 5, now);
  if (!ip.ok) throw rateLimited(ip);

  const r = await env.DB.prepare(
    `INSERT INTO waitlist (email, source, created_at, utm_source, utm_medium, utm_campaign, ref, age_ok)
     VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, 1) ON CONFLICT (email) DO NOTHING`,
  )
    .bind(email, source, now, utmSource, utmMedium, utmCampaign, ref)
    .run();
  // Only new addresses count toward the daily cap.
  if (r.meta.changes === 1) await hitRateLimit(env.DB, "waitlist:day", 86400, Number.MAX_SAFE_INTEGER, now);
  return done();
}
