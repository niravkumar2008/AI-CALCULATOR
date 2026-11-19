// Back office for the Proofline Farm "Business HQ" (docs/web/BACK_OFFICE.md).
//
// 1. receiveMail(): customer e-mail that Cloudflare Email Routing hands to this Worker's email()
//    handler is stored in D1 (a copy is also forwarded to Nirav's own inbox by index.ts).
// 2. /v1/admin/* (x-admin-token): the numbers for the business dashboard, the order list and
//    "mark shipped", the inbox, saving a draft reply, and sending a reply Nirav approved.
//
// Nothing here sends mail by itself: a reply leaves only through POST /v1/admin/mail/:id/send,
// which the farm app calls when Nirav presses "Approve & send".

import PostalMime from "postal-mime";
import { requireAdmin } from "./admin";
import type { Config, Env } from "./env";
import { HttpError, MSG, json } from "./http";
import { StripeError, stripeCall } from "./stripe";
import { dayOf, monthOf, onlyKeys, readJsonObject } from "./util";
import { hex, randomBytes } from "./crypto";
import { unitsLeft } from "./billing";

type ParsedEmail = Awaited<ReturnType<typeof PostalMime.parse>>;

export const MAIL_BODY_MAX = 20_000;
export const MAIL_PER_DAY_MAX = 300; // stored per UTC day; more is only forwarded (a flood can't fill D1)
const MAIL_RAW_MAX = 8 * 1024 * 1024; // bigger messages aren't parsed (still forwarded)
const MAIL_KEEP_SEC = 2 * 365 * 86400;
const DRAFT_MAX = 8000;
const SUMMARY_MAX = 200;
export const CATEGORIES = ["order", "shipping", "support", "refund", "sales", "press", "spam", "other"] as const;
const SHIPPABLE = "'paid','partially_refunded'";

// ------------------------------------------------------------------ inbound e-mail

export interface Envelope {
  from: string; // SMTP envelope sender
  to: string; // the address it was sent to (hello@your-domain)
  rawSize: number;
}

const ADDR_RE = /^[^\s@<>()",;:]{1,64}@[A-Za-z0-9.-]{1,253}$/;
const clean = (s: unknown, max: number): string =>
  typeof s === "string" ? s.replace(/[\u0000-\u0008\u000b\u000c\u000e-\u001f\u007f]/g, "").slice(0, max) : "";

function headerOf(headers: { key: string; value: string }[], key: string): string {
  return headers.find((h) => h.key.toLowerCase() === key)?.value ?? "";
}

/** Auto-replies, bounces, mailing lists and no-reply senders: shown, but never drafted or answered. */
function isAuto(headers: { key: string; value: string }[], from: string): boolean {
  const auto = headerOf(headers, "auto-submitted").trim().toLowerCase();
  if (auto && auto !== "no") return true;
  if (/^(bulk|junk|list|auto_reply)$/i.test(headerOf(headers, "precedence").trim())) return true;
  if (headerOf(headers, "list-id") || headerOf(headers, "list-unsubscribe")) return true;
  if (headerOf(headers, "x-autoreply") || headerOf(headers, "x-autorespond")) return true;
  return /^(mailer-daemon|postmaster|no-?reply|do-?not-?reply|bounces?)([+.@-]|$)/i.test(from);
}

/** Plain text from HTML-only mail (good enough to read and to draft from; never rendered). */
function htmlToText(html: string): string {
  return html
    .replace(/<(script|style|head)[\s\S]*?<\/\1>/gi, " ")
    .replace(/<br\s*\/?>|<\/(p|div|li|tr|h\d)>/gi, "\n")
    .replace(/<[^>]+>/g, " ")
    .replace(/&nbsp;/g, " ")
    .replace(/&amp;/g, "&")
    .replace(/&lt;/g, "<")
    .replace(/&gt;/g, ">")
    .replace(/&quot;/g, '"')
    .replace(/&#39;/g, "'")
    .replace(/[ \t]+/g, " ")
    .split("\n")
    .map((l) => l.trim())
    .join("\n")
    .replace(/\n{3,}/g, "\n\n")
    .trim();
}

const normSubject = (s: string) =>
  s
    .replace(/^\s*((re|fw|fwd|aw)\s*:\s*)+/i, "")
    .trim()
    .toLowerCase();

export interface ReceiveResult {
  stored: boolean;
  id?: string;
  why?: string;
}

/**
 * Parses one inbound message and stores it. Never throws for a bad message (it is skipped and
 * logged); the caller forwards the original either way.
 */
export async function receiveMail(
  env: Env,
  raw: ReadableStream<Uint8Array> | ArrayBuffer | Uint8Array | string,
  envelope: Envelope,
  now: number,
): Promise<ReceiveResult> {
  if (envelope.rawSize > MAIL_RAW_MAX) return { stored: false, why: "too_big" };
  const ownFrom = addrOf((env.MAIL_REPLY_FROM ?? "").trim());
  let parsed: ParsedEmail;
  try {
    parsed = await PostalMime.parse(raw as Parameters<typeof PostalMime.parse>[0]);
  } catch (e) {
    console.error(JSON.stringify({ event: "mail_parse_failed", detail: String(e).slice(0, 200) }));
    return { stored: false, why: "unreadable" };
  }
  const box = parsed.from && "address" in parsed.from ? parsed.from : null;
  const fromAddr = clean(box?.address || envelope.from, 254).trim().toLowerCase();
  if (!ADDR_RE.test(fromAddr)) return { stored: false, why: "bad_sender" };
  if (ownFrom && fromAddr === ownFrom) return { stored: false, why: "own_mail" }; // our own reply looping back

  const today = await env.DB.prepare(`SELECT COUNT(*) AS n FROM mail WHERE received_at >= ?1`)
    .bind(now - (now % 86400))
    .first<{ n: number }>();
  if ((today?.n ?? 0) >= MAIL_PER_DAY_MAX) {
    console.error(JSON.stringify({ event: "mail_daily_cap", n: today?.n }));
    return { stored: false, why: "daily_cap" };
  }

  const headers = parsed.headers ?? [];
  const auto = isAuto(headers, fromAddr);
  const subject = clean(parsed.subject ?? "", 300);
  let body = clean(parsed.text ?? "", MAIL_BODY_MAX * 2);
  if (!body.trim() && parsed.html) body = htmlToText(clean(parsed.html, MAIL_BODY_MAX * 4));
  body = body.slice(0, MAIL_BODY_MAX);
  const attachments = (parsed.attachments ?? []).slice(0, 20).map((a) => ({
    name: clean(a.filename ?? "", 120) || "(no name)",
    type: clean(a.mimeType ?? "", 80),
    size: typeof a.content === "string" ? a.content.length : (a.content as ArrayBuffer | undefined)?.byteLength ?? 0,
  }));
  const messageId = clean(parsed.messageId ?? "", 300);

  // Thread: the mail it answers (In-Reply-To / References), else the same sender and subject in
  // the last 60 days, else a new conversation.
  const refs = `${parsed.inReplyTo ?? ""} ${parsed.references ?? ""}`.match(/<[^<>\s]{1,300}>/g) ?? [];
  let threadId: string | null = null;
  for (const r of refs.slice(-10)) {
    const row = await env.DB.prepare(`SELECT thread_id FROM mail WHERE message_id = ?1 LIMIT 1`)
      .bind(r)
      .first<{ thread_id: string }>();
    if (row) {
      threadId = row.thread_id;
      break;
    }
  }
  if (!threadId && normSubject(subject)) {
    const rows = await env.DB.prepare(
      `SELECT thread_id, subject FROM mail WHERE from_addr = ?1 AND received_at > ?2 ORDER BY received_at DESC LIMIT 20`,
    )
      .bind(fromAddr, now - 60 * 86400)
      .all<{ thread_id: string; subject: string }>();
    threadId = rows.results.find((r) => normSubject(r.subject ?? "") === normSubject(subject))?.thread_id ?? null;
  }

  const id = `mail_${hex(randomBytes(8))}`;
  await env.DB.prepare(
    `INSERT INTO mail (id, received_at, from_addr, from_name, to_addr, subject, body, message_id, thread_id,
                       attachments, auto, status)
     VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12)`,
  )
    .bind(
      id,
      now,
      fromAddr,
      clean(box?.name ?? "", 120) || null,
      clean(envelope.to, 254) || null,
      subject || null,
      body,
      messageId || null,
      threadId ?? id,
      attachments.length ? JSON.stringify(attachments) : null,
      auto ? 1 : 0,
      auto ? "ignored" : "new",
    )
    .run();
  console.log(JSON.stringify({ event: "mail_received", id, auto }));
  return { stored: true, id };
}

/** "Proofline <hello@x.com>" or "hello@x.com" -> "hello@x.com" (lower case), else "". */
function addrOf(s: string): string {
  const m = s.match(/<([^<>]+)>\s*$/);
  const a = (m ? m[1]! : s).trim().toLowerCase();
  return ADDR_RE.test(a) ? a : "";
}

/** Old mail is deleted after two years (hourly cron). */
export function mailCleanup(env: Env, now: number): D1PreparedStatement {
  return env.DB.prepare(`DELETE FROM mail WHERE received_at < ?1`).bind(now - MAIL_KEEP_SEC);
}

// ------------------------------------------------------------------ sending (only on Nirav's say-so)

interface SendResult {
  ok: boolean;
  why?: string;
}

/** The address replies and shipping notices come from. */
function officeFrom(env: Env): string {
  return (env.MAIL_REPLY_FROM ?? "").trim() || (env.MAIL_FROM ?? "").trim();
}

async function sendOfficeMail(
  env: Env,
  to: string,
  subject: string,
  text: string,
  headers: Record<string, string> = {},
): Promise<SendResult> {
  const key = (env.RESEND_API_KEY ?? "").trim();
  const from = officeFrom(env);
  if (!key) return { ok: false, why: "RESEND_API_KEY is not set on the Worker." };
  if (!from) return { ok: false, why: "MAIL_REPLY_FROM is not set on the Worker." };
  try {
    const r = await fetch("https://api.resend.com/emails", {
      method: "POST",
      headers: { authorization: `Bearer ${key}`, "content-type": "application/json" },
      body: JSON.stringify({ from, to: [to], subject, text, reply_to: addrOf(from) || undefined, headers }),
      signal: AbortSignal.timeout(15_000),
    });
    const detail = await r.text().catch(() => "");
    if (!r.ok) {
      console.error(JSON.stringify({ event: "office_mail_failed", status: r.status, detail: detail.slice(0, 200) }));
      return { ok: false, why: `The mail service said ${r.status}. ${detail.slice(0, 160)}` };
    }
    return { ok: true };
  } catch (e) {
    console.error(JSON.stringify({ event: "office_mail_failed", detail: String(e).slice(0, 200) }));
    return { ok: false, why: "Couldn't reach the mail service. Try again in a minute." };
  }
}

// ------------------------------------------------------------------ GET /v1/admin/business

const n = (v: unknown) => (typeof v === "number" && Number.isFinite(v) ? v : 0);

/** Everything the dashboard shows, in one call. Counts and sums only (no names or addresses). */
export async function business(req: Request, env: Env, cfg: Config, now: number): Promise<Response> {
  await requireAdmin(req, env);
  const day = now - (now % 86400);
  const week = now - 7 * 86400;
  const month = now - 30 * 86400;
  const [byStatus, sales, toShip, subs, wl, wlSources, people, ai, aiMonth, mail] = await env.DB.batch([
    env.DB.prepare(
      `SELECT status, COUNT(*) AS orders, SUM(COALESCE(quantity, 1)) AS units, SUM(COALESCE(amount_total, 0)) AS cents,
              SUM(COALESCE(amount_refunded, 0)) AS refunded FROM orders GROUP BY status`,
    ),
    env.DB.prepare(
      `SELECT SUM(created_at >= ?1) AS n_day, SUM(CASE WHEN created_at >= ?1 THEN amount_total ELSE 0 END) AS c_day,
              SUM(created_at >= ?2) AS n_week, SUM(CASE WHEN created_at >= ?2 THEN amount_total ELSE 0 END) AS c_week,
              SUM(created_at >= ?3) AS n_month, SUM(CASE WHEN created_at >= ?3 THEN amount_total ELSE 0 END) AS c_month,
              COUNT(*) AS n_all, SUM(COALESCE(amount_total, 0)) AS c_all,
              SUM(COALESCE(amount_refunded, 0)) AS refunded_all
       FROM orders WHERE status IN (${SHIPPABLE}, 'refunded', 'disputed', 'dispute_won', 'dispute_lost')`,
    ).bind(day, week, month),
    env.DB.prepare(
      `SELECT COUNT(*) AS orders, SUM(COALESCE(quantity, 1)) AS units, MIN(created_at) AS oldest
       FROM orders WHERE status IN (${SHIPPABLE}) AND shipped_at IS NULL`,
    ),
    env.DB.prepare(`SELECT status, price_id, cancel_at_period_end AS ending, COUNT(*) AS n FROM subscriptions GROUP BY 1, 2, 3`),
    env.DB.prepare(
      `SELECT COUNT(*) AS total, SUM(created_at >= ?1) AS today, SUM(created_at >= ?2) AS week FROM waitlist`,
    ).bind(day, week),
    env.DB.prepare(
      `SELECT COALESCE(NULLIF(utm_source, ''), source) AS src, COUNT(*) AS n FROM waitlist
       WHERE created_at >= ?1 GROUP BY 1 ORDER BY 2 DESC LIMIT 6`,
    ).bind(month),
    env.DB.prepare(
      `SELECT (SELECT COUNT(*) FROM users WHERE deleted_at IS NULL) AS users,
              (SELECT COUNT(*) FROM devices WHERE user_id IS NOT NULL AND revoked_at IS NULL) AS linked,
              (SELECT COUNT(*) FROM devices WHERE revoked_at IS NULL AND last_seen_at >= ?1) AS active_week`,
    ).bind(week),
    env.DB.prepare(
      `SELECT COUNT(*) AS solves, SUM(cost_micro) AS micro FROM usage WHERE day = ?1 AND state = 'settled'`,
    ).bind(dayOf(now)),
    env.DB.prepare(
      `SELECT COUNT(*) AS solves, SUM(cost_micro) AS micro FROM usage WHERE month = ?1 AND state = 'settled'`,
    ).bind(monthOf(now)),
    env.DB.prepare(`SELECT status, auto, COUNT(*) AS n, MIN(received_at) AS oldest FROM mail GROUP BY 1, 2`),
  ]);
  const left = await unitsLeft(env, cfg, now);

  const s = cfg.stripe;
  const ordersByStatus: Record<string, { orders: number; units: number; cents: number; refunded: number }> = {};
  for (const r of byStatus!.results as any[]) {
    ordersByStatus[String(r.status)] = { orders: n(r.orders), units: n(r.units), cents: n(r.cents), refunded: n(r.refunded) };
  }
  const sl = (sales!.results[0] ?? {}) as any;
  const ts = (toShip!.results[0] ?? {}) as any;
  const subRows = subs!.results as any[];
  const live = (st: string) => ["active", "trialing", "past_due"].includes(st);
  const plan = {
    active: 0,
    monthly: 0,
    yearly: 0,
    past_due: 0,
    ending: 0,
    canceled: 0,
  };
  for (const r of subRows) {
    const k = n(r.n);
    if (live(r.status)) {
      plan.active += k;
      if (r.price_id && r.price_id === s.priceSubYearly) plan.yearly += k;
      else plan.monthly += k;
      if (r.ending) plan.ending += k;
    }
    if (r.status === "past_due") plan.past_due += k;
    if (r.status === "canceled") plan.canceled += k;
  }
  const w = (wl!.results[0] ?? {}) as any;
  const p = (people!.results[0] ?? {}) as any;
  const a = (ai!.results[0] ?? {}) as any;
  const am = (aiMonth!.results[0] ?? {}) as any;
  const mailCounts = { new: 0, drafted: 0, replied: 0, ignored: 0, auto: 0, oldest_open: null as number | null };
  for (const r of mail!.results as any[]) {
    if (r.auto) mailCounts.auto += n(r.n);
    if (r.status in mailCounts) (mailCounts as any)[r.status] += n(r.n);
    if ((r.status === "new" || r.status === "drafted") && !r.auto && r.oldest) {
      mailCounts.oldest_open = Math.min(mailCounts.oldest_open ?? Infinity, n(r.oldest));
    }
  }
  const key = s.secretKey;
  const mailFrom = officeFrom(env);
  const body = {
    at: now,
    store: {
      open: s.storeOpen,
      stock: s.stock,
      units_left: left,
      ship_note: s.shipNote,
      shipping_rates: s.shippingRates.length,
      promo_codes: s.promoCodes,
      tax: s.tax,
    },
    sales: {
      day: { orders: n(sl.n_day), cents: n(sl.c_day) },
      week: { orders: n(sl.n_week), cents: n(sl.c_week) },
      month: { orders: n(sl.n_month), cents: n(sl.c_month) },
      all: { orders: n(sl.n_all), cents: n(sl.c_all), refunded_cents: n(sl.refunded_all) },
    },
    orders: { by_status: ordersByStatus, to_ship: { orders: n(ts.orders), units: n(ts.units), oldest: ts.oldest ?? null } },
    plan,
    waitlist: {
      total: n(w.total),
      today: n(w.today),
      week: n(w.week),
      sources_30d: (wlSources!.results as any[]).map((r) => ({ source: String(r.src ?? "site"), n: n(r.n) })),
    },
    people: { accounts: n(p.users), calculators_linked: n(p.linked), calculators_active_7d: n(p.active_week) },
    ai: {
      solves_today: n(a.solves),
      cost_today_usd: n(a.micro) / 1e6,
      solves_month: n(am.solves),
      cost_month_usd: n(am.micro) / 1e6,
      daily_cap_usd: cfg.globalDailyCapMicro / 1e6,
    },
    mail: mailCounts,
    // What is set up on the Worker (true/false only, never the values).
    setup: {
      public_url: cfg.publicUrl,
      custom_domain: !/\.workers\.dev$|localhost|127\.0\.0\.1/.test(cfg.host),
      stripe_key: key ? (/^(sk|rk)_live_/.test(key) ? "live" : "test") : "missing",
      stripe_webhook: s.webhookSecret !== "",
      stripe_prices: { calculator: s.priceDevice !== "", monthly: s.priceSub !== "", yearly: s.priceSubYearly !== "" },
      resend: (env.RESEND_API_KEY ?? "").trim() !== "",
      mail_from_own_domain: !!mailFrom && !/resend\.dev/i.test(mailFrom),
      mail_reply_from: (env.MAIL_REPLY_FROM ?? "").trim() !== "",
      mail_forward_to: (env.MAIL_FORWARD_TO ?? "").trim() !== "",
      turnstile: (env.TURNSTILE_SECRET ?? "").trim() !== "",
      anthropic: (env.ANTHROPIC_API_KEY ?? "").trim() !== "",
    },
  };
  return json(body);
}

// ------------------------------------------------------------------ GET /v1/admin/stripe

/** Money in Stripe: balance and the last payouts to the bank. Needs "Balance: read" and "Payouts: read". */
export async function stripeMoney(req: Request, env: Env, cfg: Config): Promise<Response> {
  await requireAdmin(req, env);
  if (!cfg.stripe.secretKey) return json({ ok: false, why: "STRIPE_SECRET_KEY is not set on the Worker yet." });
  const sum = (list: any[] | undefined) =>
    (list ?? []).filter((x) => x?.currency === "usd").reduce((t, x) => t + n(x.amount), 0);
  try {
    const [bal, payouts] = await Promise.all([
      stripeCall(cfg.stripe, "GET", "/v1/balance"),
      stripeCall(cfg.stripe, "GET", "/v1/payouts", { limit: 5 }).catch((e) => ({ error: e })),
    ]);
    const list = Array.isArray((payouts as any).data) ? ((payouts as any).data as any[]) : [];
    return json({
      ok: true,
      mode: bal.livemode ? "live" : "test",
      available_cents: sum(bal.available),
      pending_cents: sum(bal.pending),
      payouts: list.map((po) => ({
        cents: n(po.amount),
        arrival: n(po.arrival_date),
        status: String(po.status ?? ""),
      })),
      payouts_error: (payouts as any).error ? "This key can't read payouts (add \"Payouts: read\")." : null,
    });
  } catch (e) {
    const code = e instanceof StripeError ? e.code : "error";
    return json({
      ok: false,
      why:
        code === "network"
          ? "Couldn't reach Stripe just now."
          : `Stripe said no (${code}). The Worker's key needs "Balance: read" and "Payouts: read".`,
    });
  }
}

// ------------------------------------------------------------------ orders

const ORDER_ID_RE = /^(cs_|hold_)[A-Za-z0-9_-]{4,255}$/;
const CARRIERS: Record<string, { name: string; url: (t: string) => string }> = {
  usps: { name: "USPS", url: (t) => `https://tools.usps.com/go/TrackConfirmAction?tLabels=${t}` },
  ups: { name: "UPS", url: (t) => `https://www.ups.com/track?tracknum=${t}` },
  fedex: { name: "FedEx", url: (t) => `https://www.fedex.com/fedextrack/?trknbr=${t}` },
  dhl: { name: "DHL", url: (t) => `https://www.dhl.com/us-en/home/tracking.html?tracking-id=${t}` },
  other: { name: "the carrier", url: () => "" },
};
const TRACKING_RE = /^[A-Za-z0-9-]{6,40}$/;

function shippingOf(raw: string | null): { name: string; address: string } | null {
  if (!raw) return null;
  try {
    const s = JSON.parse(raw);
    const ad = s.address ?? {};
    const line = [ad.line1, ad.line2, ad.city, [ad.state, ad.postal_code].filter(Boolean).join(" "), ad.country]
      .filter((x) => typeof x === "string" && x.trim())
      .join(", ");
    return { name: typeof s.name === "string" ? s.name.slice(0, 120) : "", address: line.slice(0, 300) };
  } catch {
    return null;
  }
}

function orderOut(r: any) {
  return {
    id: r.id,
    created_at: r.created_at,
    email: r.email,
    status: r.status,
    quantity: r.quantity ?? 1,
    cents: r.amount_total ?? 0,
    refunded_cents: r.amount_refunded ?? 0,
    ship_to: shippingOf(r.shipping_json),
    shipped_at: r.shipped_at ?? null,
    carrier: r.carrier ?? null,
    tracking: r.tracking ?? null,
    notified_at: r.ship_notified_at ?? null,
  };
}

/** GET /v1/admin/orders?view=to_ship|recent */
export async function listOrders(req: Request, env: Env): Promise<Response> {
  await requireAdmin(req, env);
  const view = new URL(req.url).searchParams.get("view") ?? "to_ship";
  if (!["to_ship", "recent"].includes(view)) throw new HttpError(400, "bad_request", "view must be to_ship or recent.");
  const sql =
    view === "to_ship"
      ? `SELECT * FROM orders WHERE status IN (${SHIPPABLE}) AND shipped_at IS NULL ORDER BY created_at ASC LIMIT 200`
      : `SELECT * FROM orders WHERE status NOT IN ('open', 'expired') ORDER BY created_at DESC LIMIT 100`;
  const rows = await env.DB.prepare(sql).all();
  return json({ view, orders: rows.results.map(orderOut) });
}

/** POST /v1/admin/orders/:id/ship {carrier, tracking, notify?} - and e-mails the customer the tracking. */
export async function shipOrder(req: Request, env: Env, cfg: Config, id: string, now: number): Promise<Response> {
  await requireAdmin(req, env);
  if (!ORDER_ID_RE.test(id)) throw new HttpError(404, "not_found", MSG.notFound);
  const body = await readJsonObject(req, 2048);
  onlyKeys(body, ["carrier", "tracking", "notify"]);
  const carrier = typeof body.carrier === "string" ? body.carrier.toLowerCase() : "";
  if (!CARRIERS[carrier]) throw new HttpError(400, "bad_request", "carrier must be usps, ups, fedex, dhl or other.");
  const tracking = typeof body.tracking === "string" ? body.tracking.replace(/\s+/g, "") : "";
  if (!TRACKING_RE.test(tracking)) throw new HttpError(400, "bad_request", "The tracking number looks wrong (6-40 letters or digits).");
  const notify = body.notify !== false;
  const row = await env.DB.prepare(
    `UPDATE orders SET shipped_at = COALESCE(shipped_at, ?2), carrier = ?3, tracking = ?4
     WHERE id = ?1 AND status IN (${SHIPPABLE}) RETURNING *`,
  )
    .bind(id, now, carrier, tracking)
    .first<any>();
  if (!row) throw new HttpError(404, "not_found", "No paid order with that id.");
  let mailed: SendResult = { ok: false, why: "not asked" };
  if (notify && row.email) {
    const c = CARRIERS[carrier]!;
    const link = c.url(encodeURIComponent(tracking));
    const name = shippingOf(row.shipping_json)?.name.split(/\s+/)[0] ?? "";
    const qty = row.quantity && row.quantity > 1 ? `${row.quantity} calculators are` : "calculator is";
    const text =
      `Hi${name ? " " + name : ""},\n\n` +
      `Good news: your Proofline ${qty} on the way.\n\n` +
      `Carrier: ${c.name}\nTracking number: ${tracking}\n${link ? `Track it: ${link}\n` : ""}\n` +
      `Setup takes about 5 minutes: ${cfg.publicUrl}/support/setup/\n\n` +
      `Questions? Just reply to this email.\n\nThanks for your order,\nProofline`;
    mailed = await sendOfficeMail(env, row.email, "Your Proofline calculator has shipped", text);
    if (mailed.ok) {
      await env.DB.prepare(`UPDATE orders SET ship_notified_at = ?2 WHERE id = ?1`).bind(id, now).run();
      row.ship_notified_at = now;
    }
  }
  console.log(JSON.stringify({ event: "order_shipped", id, carrier, mailed: mailed.ok }));
  return json({ order: orderOut(row), mailed: mailed.ok, mail_problem: mailed.ok || !notify ? null : mailed.why });
}

// ------------------------------------------------------------------ the inbox

const MAIL_ID_RE = /^mail_[0-9a-f]{16}$/;

function mailOut(r: any, full = false) {
  const out: Record<string, unknown> = {
    id: r.id,
    received_at: r.received_at,
    from: r.from_addr,
    from_name: r.from_name,
    subject: r.subject,
    thread_id: r.thread_id,
    auto: !!r.auto,
    status: r.status,
    category: r.category,
    summary: r.summary,
    draft: r.draft,
    drafted_at: r.drafted_at,
    replied_at: r.replied_at,
    preview: String(r.body ?? "").replace(/\s+/g, " ").slice(0, 160),
  };
  if (full) {
    out.body = r.body;
    out.to = r.to_addr;
    out.reply_text = r.reply_text;
    out.attachments = r.attachments ? JSON.parse(r.attachments) : [];
  }
  return out;
}

/** GET /v1/admin/mail?view=open|all|new */
export async function listMail(req: Request, env: Env): Promise<Response> {
  await requireAdmin(req, env);
  const view = new URL(req.url).searchParams.get("view") ?? "open";
  const where: Record<string, string> = {
    open: `status IN ('new', 'drafted')`,
    new: `status = 'new'`,
    all: `1 = 1`,
  };
  if (!where[view]) throw new HttpError(400, "bad_request", "view must be open, new or all.");
  const rows = await env.DB.prepare(
    `SELECT * FROM mail WHERE ${where[view]} ORDER BY received_at ${view === "all" ? "DESC" : "ASC"} LIMIT 100`,
  ).all();
  return json({ view, mail: rows.results.map((r) => mailOut(r)) });
}

/** GET /v1/admin/mail/:id - the mail, its conversation, and what we know about the sender. */
export async function getMail(req: Request, env: Env, id: string): Promise<Response> {
  await requireAdmin(req, env);
  if (!MAIL_ID_RE.test(id)) throw new HttpError(404, "not_found", MSG.notFound);
  const row = await env.DB.prepare(`SELECT * FROM mail WHERE id = ?1`).bind(id).first<any>();
  if (!row) throw new HttpError(404, "not_found", MSG.notFound);
  const [thread, orders, sub, wl] = await env.DB.batch([
    env.DB.prepare(`SELECT * FROM mail WHERE thread_id = ?1 AND id != ?2 ORDER BY received_at ASC LIMIT 20`).bind(
      row.thread_id,
      id,
    ),
    env.DB.prepare(
      `SELECT * FROM orders WHERE email = ?1 COLLATE NOCASE AND status NOT IN ('open', 'expired') ORDER BY created_at DESC LIMIT 5`,
    ).bind(row.from_addr),
    env.DB.prepare(
      `SELECT s.status, s.price_id, s.current_period_end, s.cancel_at_period_end FROM subscriptions s
       JOIN users u ON u.id = s.user_id WHERE u.email = ?1 COLLATE NOCASE ORDER BY s.updated_at DESC LIMIT 1`,
    ).bind(row.from_addr),
    env.DB.prepare(`SELECT created_at FROM waitlist WHERE email = ?1`).bind(row.from_addr),
  ]);
  const s = (sub!.results[0] ?? null) as any;
  return json({
    mail: mailOut(row, true),
    thread: (thread!.results as any[]).map((r) => mailOut(r, true)),
    customer: {
      orders: (orders!.results as any[]).map((o) => {
        const x = orderOut(o);
        return { ...x, ship_to: x.ship_to ? { name: x.ship_to.name } : null }; // the drafter doesn't need the address
      }),
      plan: s
        ? {
            status: s.status,
            yearly: !!s.price_id && s.price_id === (env.STRIPE_PRICE_SUB_YEARLY ?? "").trim(),
            period_end: s.current_period_end,
            ending: !!s.cancel_at_period_end,
          }
        : null,
      on_waitlist_since: (wl!.results[0] as any)?.created_at ?? null,
    },
  });
}

/** POST /v1/admin/mail/:id {action: draft|ignore|reopen, draft?, summary?, category?} */
export async function updateMail(req: Request, env: Env, id: string, now: number): Promise<Response> {
  await requireAdmin(req, env);
  if (!MAIL_ID_RE.test(id)) throw new HttpError(404, "not_found", MSG.notFound);
  const body = await readJsonObject(req, DRAFT_MAX * 2);
  onlyKeys(body, ["action", "draft", "summary", "category"]);
  const category =
    typeof body.category === "string" && (CATEGORIES as readonly string[]).includes(body.category) ? body.category : null;
  const summary = clean(body.summary, SUMMARY_MAX) || null;
  let row: any;
  if (body.action === "draft") {
    const draft = clean(body.draft, DRAFT_MAX + 1);
    if (draft.length > DRAFT_MAX) throw new HttpError(400, "bad_request", `The draft is longer than ${DRAFT_MAX} characters.`);
    // A draft never overwrites a sent reply; status goes new -> drafted (a re-draft stays drafted).
    // An empty draft is the drafter saying "no reply needed" (spam, a thank-you): Nirav ignores or answers it.
    row = await env.DB.prepare(
      `UPDATE mail SET draft = ?2, drafted_at = ?3, summary = COALESCE(?4, summary), category = COALESCE(?5, category),
         status = 'drafted'
       WHERE id = ?1 AND status IN ('new', 'drafted') RETURNING *`,
    )
      .bind(id, draft, now, summary, category)
      .first();
  } else if (body.action === "ignore") {
    row = await env.DB.prepare(
      `UPDATE mail SET status = 'ignored', category = COALESCE(?2, category) WHERE id = ?1 AND status IN ('new', 'drafted') RETURNING *`,
    )
      .bind(id, category)
      .first();
  } else if (body.action === "reopen") {
    row = await env.DB.prepare(
      `UPDATE mail SET status = CASE WHEN drafted_at IS NULL THEN 'new' ELSE 'drafted' END
       WHERE id = ?1 AND status = 'ignored' RETURNING *`,
    )
      .bind(id)
      .first();
  } else {
    throw new HttpError(400, "bad_request", "action must be draft, ignore or reopen.");
  }
  if (!row) throw new HttpError(400, "bad_request", "That mail was already answered or doesn't exist.");
  return json({ mail: mailOut(row, true) });
}

/** POST /v1/admin/mail/:id/send {text} - sends the reply Nirav approved (after his edits). */
export async function sendReply(req: Request, env: Env, id: string, now: number): Promise<Response> {
  await requireAdmin(req, env);
  if (!MAIL_ID_RE.test(id)) throw new HttpError(404, "not_found", MSG.notFound);
  const body = await readJsonObject(req, DRAFT_MAX * 2);
  onlyKeys(body, ["text"]);
  const text = clean(body.text, DRAFT_MAX + 1).trim();
  if (!text) throw new HttpError(400, "bad_request", "The reply is empty.");
  if (text.length > DRAFT_MAX) throw new HttpError(400, "bad_request", `The reply is longer than ${DRAFT_MAX} characters.`);
  // Claim the mail first, so a double click can't send two replies.
  const row = await env.DB.prepare(
    `UPDATE mail SET status = 'replied', replied_at = ?2, reply_text = ?3
     WHERE id = ?1 AND status IN ('new', 'drafted') AND auto = 0 RETURNING *`,
  )
    .bind(id, now, text)
    .first<any>();
  if (!row) throw new HttpError(400, "bad_request", "That mail was already answered, ignored, or is an automatic mail.");
  const subject = /^\s*re\s*:/i.test(row.subject ?? "") ? row.subject : `Re: ${row.subject || "your message"}`;
  const headers: Record<string, string> = {};
  if (row.message_id) {
    headers["In-Reply-To"] = row.message_id;
    headers["References"] = row.message_id;
  }
  const quoted = String(row.body ?? "")
    .split(/\r?\n/)
    .slice(0, 40)
    .map((l) => `> ${l}`)
    .join("\n");
  const when = new Date(row.received_at * 1000).toUTCString().replace(/:\d\d GMT$/, " UTC");
  const sent = await sendOfficeMail(
    env,
    row.from_addr,
    subject.slice(0, 300),
    `${text}\n\nOn ${when}, ${row.from_name || row.from_addr} wrote:\n${quoted}`,
    headers,
  );
  if (!sent.ok) {
    await env.DB.prepare(
      `UPDATE mail SET status = CASE WHEN drafted_at IS NULL THEN 'new' ELSE 'drafted' END, replied_at = NULL, reply_text = NULL
       WHERE id = ?1`,
    )
      .bind(id)
      .run();
    throw new HttpError(503, "unavailable", sent.why ?? MSG.unavailable);
  }
  console.log(JSON.stringify({ event: "mail_replied", id }));
  return json({ mail: mailOut(row, true) });
}
