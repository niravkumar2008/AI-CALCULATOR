// The farm's link to the business: the Cloudflare Worker's back office (/v1/admin/*, docs/web/BACK_OFFICE.md).
// Used by the app (Business HQ screen) and the manager (Penny's mail drafts, the business numbers Fern plans from).
//
// The admin token never goes into a farm run: the app keeps it encrypted (Windows DPAPI via Electron safeStorage)
// and hands it to the manager process in PROOFLINE_ADMIN_TOKEN; runEnv/crewEnv strip it before any `claude -p`.
'use strict';

const DEFAULT_API = 'https://ai-calc-api.ai-calc-api.workers.dev';
const ID_RE = { order: /^(cs_|hold_)[A-Za-z0-9_-]{4,255}$/, mail: /^mail_[0-9a-f]{16}$/ };

function settings(env = process.env) {
  const apiUrl = String(env.PROOFLINE_API_URL || DEFAULT_API).replace(/\/+$/, '');
  const token = String(env.PROOFLINE_ADMIN_TOKEN || '').trim();
  return { apiUrl, token, enabled: /^https?:\/\//.test(apiUrl) && token.length >= 32 };
}

/** A small client. Every call resolves to { ok, status, data, error } and never throws. */
function client({ apiUrl, token, fetchImpl = globalThis.fetch, timeoutMs = 20000 }) {
  async function req(method, path, body) {
    const t0 = Date.now();
    try {
      const r = await fetchImpl(apiUrl + path, {
        method,
        headers: { 'x-admin-token': token, ...(body ? { 'content-type': 'application/json' } : {}) },
        body: body ? JSON.stringify(body) : undefined,
        signal: AbortSignal.timeout(timeoutMs),
      });
      const text = await r.text();
      let data = null;
      try { data = JSON.parse(text); } catch { /* not JSON */ }
      const error = r.ok ? null : (data && data.error && data.error.message) || `The Worker answered ${r.status}.`;
      return { ok: r.ok, status: r.status, data, error, ms: Date.now() - t0 };
    } catch (e) {
      return { ok: false, status: 0, data: null, error: `Couldn't reach the Worker (${String(e.message || e).slice(0, 80)}).`, ms: Date.now() - t0 };
    }
  }
  const id = (kind, v) => { if (!ID_RE[kind].test(String(v))) throw new Error(`bad ${kind} id`); return encodeURIComponent(v); };
  return {
    health: () => req('GET', '/healthz'),
    business: () => req('GET', '/v1/admin/business'),
    stripe: () => req('GET', '/v1/admin/stripe'),
    orders: (view = 'to_ship') => req('GET', `/v1/admin/orders?view=${view === 'recent' ? 'recent' : 'to_ship'}`),
    ship: (orderId, carrier, tracking, notify = true) => req('POST', `/v1/admin/orders/${id('order', orderId)}/ship`, { carrier, tracking, notify }),
    mail: (view = 'open') => req('GET', `/v1/admin/mail?view=${['open', 'new', 'all'].includes(view) ? view : 'open'}`),
    mailGet: mailId => req('GET', `/v1/admin/mail/${id('mail', mailId)}`),
    mailDraft: (mailId, d) => req('POST', `/v1/admin/mail/${id('mail', mailId)}`, { action: 'draft', draft: String(d.draft || ''), summary: d.summary || undefined, category: d.category || undefined }),
    mailIgnore: mailId => req('POST', `/v1/admin/mail/${id('mail', mailId)}`, { action: 'ignore' }),
    mailReopen: mailId => req('POST', `/v1/admin/mail/${id('mail', mailId)}`, { action: 'reopen' }),
    mailSend: (mailId, text) => req('POST', `/v1/admin/mail/${id('mail', mailId)}/send`, { text: String(text || '') }),
  };
}

const usd = c => `$${(Number(c || 0) / 100).toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`;
const days = sec => Math.floor((Date.now() / 1000 - sec) / 86400);

/**
 * What needs Nirav, from the business numbers: [{ kind, level: 'urgent'|'todo'|'setup', text, go }].
 * `go` is the Business HQ tab that fixes it.
 */
function alerts(biz, health) {
  const out = [];
  if (health && !health.ok) out.push({ kind: 'worker', level: 'urgent', text: 'The website/Worker is not answering.', go: 'cloud' });
  if (!biz) return out;
  const m = biz.mail || {};
  const drafted = m.drafted || 0, fresh = m.new || 0;
  if (drafted) out.push({ kind: 'mail', level: 'urgent', text: `${drafted} customer repl${drafted > 1 ? 'ies' : 'y'} drafted: read and approve`, go: 'inbox' });
  if (fresh) out.push({ kind: 'mail', level: 'todo', text: `${fresh} new customer e-mail${fresh > 1 ? 's' : ''} (Penny is on ${fresh > 1 ? 'them' : 'it'})`, go: 'inbox' });
  const ship = (biz.orders || {}).to_ship || {};
  if (ship.orders) {
    const old = ship.oldest ? days(ship.oldest) : 0;
    // The ship note promises 3-5 weeks; warn well before the FTC "ship when promised" line.
    const level = old >= 21 ? 'urgent' : 'todo';
    out.push({ kind: 'ship', level, text: `${ship.units} calculator${ship.units > 1 ? 's' : ''} to ship in ${ship.orders} order${ship.orders > 1 ? 's' : ''}${old ? ` (oldest ${old} day${old > 1 ? 's' : ''})` : ''}${old >= 21 ? ': close to the promised ship time' : ''}`, go: 'orders' });
  }
  const st = biz.store || {};
  if (st.open && st.units_left !== null && st.units_left !== undefined && st.units_left <= 3) {
    out.push({ kind: 'stock', level: st.units_left === 0 ? 'urgent' : 'todo', text: st.units_left === 0 ? 'Sold out: the store closed itself. Raise STORE_STOCK when the next batch is ready.' : `Only ${st.units_left} calculator${st.units_left > 1 ? 's' : ''} left in this batch.`, go: 'today' });
  }
  const plan = biz.plan || {};
  if (plan.past_due) out.push({ kind: 'plan', level: 'todo', text: `${plan.past_due} AI-plan payment${plan.past_due > 1 ? 's' : ''} failed (Stripe retries by itself).`, go: 'money' });
  const ai = biz.ai || {};
  if (ai.daily_cap_usd && ai.cost_today_usd >= ai.daily_cap_usd * 0.8) out.push({ kind: 'ai', level: 'urgent', text: `AI cost today ${usd(ai.cost_today_usd * 100)} is near the ${usd(ai.daily_cap_usd * 100)} daily cap.`, go: 'cloud' });
  for (const s of setupSteps(biz).filter(x => !x.done).slice(0, 3)) out.push({ kind: 'setup', level: 'setup', text: s.text, go: 'launch' });
  return out;
}

/** The launch checklist, from what the Worker reports as set up (true/false only). */
function setupSteps(biz) {
  const s = (biz && biz.setup) || {};
  const st = (biz && biz.store) || {};
  const p = s.stripe_prices || {};
  return [
    { id: 'stripe', done: s.stripe_key === 'live', text: s.stripe_key === 'test' ? 'Stripe is in test mode: switch the Worker to the live key.' : 'Add the Stripe key to the Worker (wrangler secret put STRIPE_SECRET_KEY).' },
    { id: 'webhook', done: !!s.stripe_webhook, text: 'Add the Stripe webhook secret (wrangler secret put STRIPE_WEBHOOK_SECRET).' },
    { id: 'prices', done: !!(p.calculator && p.monthly), text: 'Paste the Stripe price ids into wrangler.toml (stripe-setup.mjs prints them).' },
    { id: 'shipping', done: (st.shipping_rates || 0) > 0, text: 'Add shipping rates (STRIPE_SHIPPING_RATE in wrangler.toml).' },
    { id: 'store', done: !!st.open, text: 'Open the store (STORE_OPEN = "1"), then deploy.' },
    { id: 'tax', done: !!st.tax, text: 'Turn on sales tax (Indiana certificate, then STRIPE_TAX = "1").' },
    { id: 'domain', done: !!s.custom_domain, text: 'Put the site on your own domain (still on workers.dev).' },
    { id: 'mail_in', done: !!s.mail_forward_to, text: 'Set up Email Routing for hello@your-domain (BACK_OFFICE guide, step 2).' },
    { id: 'mail_out', done: !!(s.mail_reply_from && s.mail_from_own_domain), text: 'Send mail from your domain: verify it in Resend, set MAIL_FROM and MAIL_REPLY_FROM.' },
    { id: 'turnstile', done: !!s.turnstile, text: 'Turn on the bot check (Turnstile) before the Thanksgiving push.' },
  ];
}

/** Short business lines for Fern's planning prompt and the morning summary (numbers only, no customers). */
function planningNotes(biz) {
  if (!biz) return '';
  const st = biz.store || {}, sa = biz.sales || {}, w = biz.waitlist || {}, pl = biz.plan || {}, ship = (biz.orders || {}).to_ship || {};
  const lines = [
    `Store: ${st.open ? 'OPEN' : 'closed'}${st.units_left != null ? `, ${st.units_left} of ${st.stock} left in this batch` : ''}${st.ship_note ? `, "${st.ship_note}"` : ''}.`,
    `Sales: today ${sa.day ? sa.day.orders : 0} (${usd(sa.day && sa.day.cents)}), 7 days ${sa.week ? sa.week.orders : 0} (${usd(sa.week && sa.week.cents)}), all time ${sa.all ? sa.all.orders : 0} (${usd(sa.all && sa.all.cents)}).`,
    `To ship: ${ship.orders || 0} order(s). AI plan: ${pl.active || 0} active (${pl.monthly || 0} monthly, ${pl.yearly || 0} yearly).`,
    `Waitlist: ${w.total || 0} total, ${w.week || 0} this week. Top sources (30 days): ${(w.sources_30d || []).map(x => `${x.source} ${x.n}`).join(', ') || 'none yet'}.`,
    `Customer mail open: ${(biz.mail || {}).new || 0} new, ${(biz.mail || {}).drafted || 0} drafted.`,
  ];
  const todo = setupSteps(biz).filter(x => !x.done).map(x => `- ${x.text}`);
  if (todo.length) lines.push(`Launch steps not done yet (only Nirav can do these; plan work that helps around them, never pretend they're done):\n${todo.join('\n')}`);
  return lines.join('\n');
}

module.exports = { DEFAULT_API, settings, client, alerts, setupSteps, planningNotes, usd };
