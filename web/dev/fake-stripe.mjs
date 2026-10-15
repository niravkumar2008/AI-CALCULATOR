// A fake Stripe for LOCAL testing of the whole site: no account, no keys, no money.
//
// It speaks the Stripe API calls the Worker makes (customers incl. DELETE, checkout and portal
// sessions, subscriptions: list, read, cancel), and it also serves two pages that stand in for
// Stripe's hosted pages:
//   /checkout/<session>  "Pay with a test card" -> creates the subscription or order, sends the
//                        same signed webhooks Stripe would, then returns to the site. The Worker
//                        re-reads each subscription from here (GET /v1/subscriptions/<id>), so
//                        the state below is always changed before its webhook is sent.
//   /portal/<session>    "Manage billing": cancel at period end, cancel now, resume, or
//                        simulate a failed payment or a renewal that needs 3-D Secure
//                        (each sends the matching webhook)
//   /invoice/<invoice>   stands in for Stripe's hosted invoice page: "Confirm payment" makes
//                        the subscription active again and sends invoice.paid
// Started by web/dev/run-local.mjs. Never point a real deployment at it.

import http from 'node:http';
import crypto from 'node:crypto';

export const FAKE_STRIPE_KEY = 'sk_test_localFakeStripe0123456789';

const esc = (s) => String(s ?? '').replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' })[c]);
const nowSec = () => Math.floor(Date.now() / 1000);
const rid = (p) => `${p}_${crypto.randomBytes(8).toString('hex')}`;

const PAGE_CSS = `
  :root{color-scheme:light;--ink:#1a1f36;--muted:#5b6478;--line:#e3e8ee;--brand:#635bff}
  *{box-sizing:border-box}body{margin:0;font:16px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif;color:var(--ink);background:#f6f9fc}
  .banner{background:#fff3cd;color:#664d03;padding:10px 16px;text-align:center;font-weight:600;font-size:14px}
  main{max-width:460px;margin:32px auto;padding:0 16px}
  .card{background:#fff;border:1px solid var(--line);border-radius:12px;padding:24px;box-shadow:0 2px 8px rgba(0,0,0,.04)}
  h1{font-size:20px;margin:0 0 4px}.muted{color:var(--muted);font-size:14px}.price{font-size:32px;font-weight:700;margin:16px 0 4px}
  button{width:100%;min-height:46px;margin-top:12px;border:0;border-radius:8px;font:600 16px system-ui;cursor:pointer;background:var(--brand);color:#fff}
  button.secondary{background:#fff;color:var(--ink);border:1px solid var(--line)}button.danger{background:#fff;color:#b42318;border:1px solid #f1c6c2}
  .row{display:flex;justify-content:space-between;padding:8px 0;border-top:1px solid var(--line);font-size:15px}
  form{margin:0}`;

function page(title, body) {
  return `<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="robots" content="noindex"><title>${esc(title)} · Fake Stripe (local test)</title><style>${PAGE_CSS}</style></head>
<body><div class="banner">Local test only. This is not Stripe and no money moves.</div><main>${body}</main></body></html>`;
}

/**
 * opts: { port, host, workerUrl, webhookSecret, priceSub, priceSubYearly, priceDevice }
 */
export function startFakeStripe(opts) {
  const { port = 8789, host = '127.0.0.1', workerUrl, webhookSecret, priceSubYearly = '' } = opts;
  const base = `http://${host}:${port}`;
  const customers = new Map(); // id -> {id, email}
  const customerByKey = new Map();
  const sessions = new Map(); // checkout sessions
  const portals = new Map(); // portal sessions
  const invoices = new Map(); // id -> {id, subscription, return_url} (3-D Secure simulation)
  const subs = new Map();
  const log = (...a) => console.log('[fake-stripe]', ...a);

  async function sendEvent(type, object) {
    const ev = { id: rid('evt'), object: 'event', type, created: nowSec(), livemode: false, data: { object } };
    const body = JSON.stringify(ev);
    const t = nowSec();
    const v1 = crypto.createHmac('sha256', webhookSecret).update(`${t}.${body}`).digest('hex');
    try {
      const r = await fetch(`${workerUrl}/api/stripe/webhook`, {
        method: 'POST',
        headers: { 'content-type': 'application/json', 'stripe-signature': `t=${t},v1=${v1}` },
        body,
      });
      log(`webhook ${type} -> ${r.status}`);
      return r.status;
    } catch (e) {
      log(`webhook ${type} failed: ${e.message}`);
      return 0;
    }
  }

  function newSubscription(customer, priceId) {
    const s = {
      id: rid('sub'),
      object: 'subscription',
      customer,
      status: 'active',
      cancel_at_period_end: false,
      cancel_at: null,
      items: { object: 'list', data: [{ id: rid('si'), price: { id: priceId, recurring: { interval: priceId === priceSubYearly ? 'year' : 'month' } }, current_period_end: nowSec() + (priceId === priceSubYearly ? 365 : 30) * 86400 }] },
      metadata: {},
    };
    subs.set(s.id, s);
    return s;
  }

  const server = http.createServer((req, res) => {
    let raw = '';
    req.on('data', (c) => (raw += c));
    req.on('end', async () => {
      const url = new URL(req.url, base);
      const json = (status, obj) => {
        res.writeHead(status, { 'content-type': 'application/json' });
        res.end(JSON.stringify(obj));
      };
      const html = (status, body) => {
        res.writeHead(status, { 'content-type': 'text/html; charset=utf-8', 'cache-control': 'no-store' });
        res.end(body);
      };
      const redirect = (to) => {
        res.writeHead(303, { location: to });
        res.end();
      };
      const params = Object.fromEntries(new URLSearchParams(req.method === 'GET' ? url.search.slice(1) : raw));

      // ------------------------------------------------ hosted pages
      let m = /^\/checkout\/([\w-]+)(\/(pay|cancel))?$/.exec(url.pathname);
      if (m) {
        const s = sessions.get(m[1]);
        if (!s) return html(404, page('Not found', '<div class="card"><h1>This checkout has expired.</h1></div>'));
        if (req.method === 'POST' && m[3] === 'cancel') return redirect(s.cancel_url);
        if (req.method === 'POST' && m[3] === 'pay') {
          if (s.completed) return redirect(s.success_url);
          s.completed = true;
          if (s.mode === 'subscription') {
            const sub = newSubscription(s.customer, s.price);
            await sendEvent('customer.subscription.created', sub);
            await sendEvent('checkout.session.completed', {
              id: s.id, object: 'checkout.session', mode: 'subscription', customer: s.customer,
              subscription: sub.id, client_reference_id: s.client_reference_id, payment_status: 'paid', status: 'complete',
            });
          } else {
            await sendEvent('checkout.session.completed', {
              id: s.id, object: 'checkout.session', mode: 'payment', payment_status: 'paid', status: 'complete',
              metadata: s.metadata, amount_total: 22500 * s.quantity, currency: 'usd', payment_intent: rid('pi'),
              customer_details: { email: 'test.buyer@example.com', name: 'Test Buyer' },
              collected_information: { shipping_details: { name: 'Test Buyer', address: { line1: '1 Test Street', city: 'Testville', state: 'CA', postal_code: '94000', country: 'US' } } },
            });
          }
          return redirect(s.success_url);
        }
        const what = s.mode === 'subscription'
          ? (priceSubYearly && s.price === priceSubYearly
            ? `<h1>AI tutor plan, yearly</h1><p class="muted">Billed yearly. Cancel anytime.</p><div class="price">$100.00 <span class="muted">/ year</span></div>`
            : `<h1>AI tutor plan, monthly</h1><p class="muted">Billed monthly. Cancel anytime.</p><div class="price">$10.00 <span class="muted">/ month</span></div>`)
          : `<h1>Calculator × ${esc(s.quantity)}</h1><p class="muted">Ships to the US.</p><div class="price">$${(225 * s.quantity).toFixed(2)}</div>`;
        const consent = s.consent ? `<p class="muted">${esc(s.consent.replace(/\[([^\]]+)\]\([^)]+\)/g, '$1'))}</p>` : '';
        return html(200, page('Checkout', `<div class="card">${what}${consent}
          <form method="post" action="/checkout/${esc(s.id)}/pay"><button type="submit">Pay with test card 4242</button></form>
          <form method="post" action="/checkout/${esc(s.id)}/cancel"><button class="secondary" type="submit">Back to the site</button></form></div>`));
      }

      m = /^\/invoice\/([\w-]+)(\/confirm)?$/.exec(url.pathname);
      if (m) {
        const inv = invoices.get(m[1]);
        if (!inv) return html(404, page('Not found', '<div class="card"><h1>This invoice is gone.</h1></div>'));
        const sub = subs.get(inv.subscription);
        if (req.method === 'POST' && m[2] && sub) {
          sub.status = 'active';
          await sendEvent('invoice.paid', { id: inv.id, object: 'invoice', parent: { subscription_details: { subscription: sub.id } } });
          return redirect(inv.return_url);
        }
        return html(200, page('Confirm your payment', `<div class="card"><h1>Confirm your payment</h1>
          <p class="muted">Your bank wants you to confirm this renewal (3-D Secure).</p>
          <form method="post" action="/invoice/${esc(inv.id)}/confirm"><button>Confirm payment</button></form></div>`));
      }

      m = /^\/portal\/([\w-]+)(\/(cancel-end|cancel-now|resume|fail|recover|sca))?$/.exec(url.pathname);
      if (m) {
        const p = portals.get(m[1]);
        if (!p) return html(404, page('Not found', '<div class="card"><h1>This billing session has expired.</h1></div>'));
        const mine = [...subs.values()].filter((s) => s.customer === p.customer && s.status !== 'canceled');
        const sub = mine[0];
        if (req.method === 'POST' && m[3] && sub) {
          const a = m[3];
          if (a === 'cancel-end') {
            sub.cancel_at_period_end = true;
            await sendEvent('customer.subscription.updated', sub);
          } else if (a === 'resume') {
            sub.cancel_at_period_end = false;
            await sendEvent('customer.subscription.updated', sub);
          } else if (a === 'cancel-now') {
            sub.status = 'canceled';
            await sendEvent('customer.subscription.deleted', sub);
          } else if (a === 'fail') {
            sub.status = 'past_due';
            await sendEvent('invoice.payment_failed', { id: rid('in'), object: 'invoice', parent: { subscription_details: { subscription: sub.id } } });
          } else if (a === 'sca') {
            sub.status = 'past_due';
            const inv = { id: rid('in'), subscription: sub.id, return_url: p.return_url };
            invoices.set(inv.id, inv);
            await sendEvent('invoice.payment_action_required', {
              id: inv.id, object: 'invoice', hosted_invoice_url: `${base}/invoice/${inv.id}`,
              parent: { subscription_details: { subscription: sub.id } },
            });
          } else if (a === 'recover') {
            sub.status = 'active';
            await sendEvent('invoice.paid', { id: rid('in'), object: 'invoice', parent: { subscription_details: { subscription: sub.id } } });
          }
          return redirect(`/portal/${p.id}`);
        }
        const rows = sub
          ? `<div class="row"><span>Status</span><strong>${esc(sub.status)}${sub.cancel_at_period_end ? ' (ends at period end)' : ''}</strong></div>
             <div class="row"><span>Renews</span><span>${new Date(sub.items.data[0].current_period_end * 1000).toDateString()}</span></div>
             ${sub.cancel_at_period_end
               ? `<form method="post" action="/portal/${p.id}/resume"><button>Don't cancel</button></form>`
               : `<form method="post" action="/portal/${p.id}/cancel-end"><button class="secondary">Cancel at period end</button></form>`}
             <form method="post" action="/portal/${p.id}/cancel-now"><button class="danger">Cancel now</button></form>
             ${sub.status === 'past_due'
               ? `<form method="post" action="/portal/${p.id}/recover"><button class="secondary">Simulate: card fixed, invoice paid</button></form>`
               : `<form method="post" action="/portal/${p.id}/fail"><button class="secondary">Simulate: next payment fails</button></form>
                  <form method="post" action="/portal/${p.id}/sca"><button class="secondary">Simulate: renewal needs 3-D Secure</button></form>`}`
          : '<p class="muted">No active subscription.</p>';
        return html(200, page('Manage billing', `<div class="card"><h1>Manage billing</h1><p class="muted">${esc(customers.get(p.customer)?.email)}</p>${rows}
          <form method="get" action="${esc(p.return_url)}"><button class="secondary">Back to your account</button></form></div>`));
      }

      // ------------------------------------------------ the API
      if (!url.pathname.startsWith('/v1/')) return json(404, { error: { type: 'invalid_request_error', code: 'unknown_path' } });
      if (req.headers.authorization !== `Bearer ${FAKE_STRIPE_KEY}`) {
        return json(401, { error: { type: 'invalid_request_error', code: 'unauthorized' } });
      }
      if (req.method === 'POST' && url.pathname === '/v1/customers') {
        const key = req.headers['idempotency-key'] ?? rid('k');
        let id = customerByKey.get(key);
        if (!id) {
          id = rid('cus');
          customerByKey.set(key, id);
          customers.set(id, { id, email: params.email, metadata: { user_id: params['metadata[user_id]'] } });
        }
        return json(200, { id, object: 'customer', email: params.email });
      }
      if (req.method === 'GET' && url.pathname === '/v1/customers') {
        // The Worker looks here after a failed customer create (review G9). Deleted ones are gone.
        const data = [...customers.values()].filter((c) => !params.email || c.email === params.email)
          .map((c) => ({ ...c, object: 'customer' }));
        return json(200, { object: 'list', url: '/v1/customers', has_more: false, data });
      }
      if (req.method === 'POST' && url.pathname === '/v1/checkout/sessions') {
        const id = rid('cs_test');
        sessions.set(id, {
          id,
          mode: params.mode,
          customer: params.customer ?? null,
          client_reference_id: params.client_reference_id ?? null,
          price: params['line_items[0][price]'],
          quantity: Number(params['line_items[0][quantity]'] ?? 1),
          success_url: params.success_url,
          cancel_url: params.cancel_url,
          consent: params['custom_text[terms_of_service_acceptance][message]'] ?? '',
          metadata: params['metadata[kind]'] ? { kind: params['metadata[kind]'] } : {},
        });
        log(`checkout session ${params.mode}`);
        return json(200, { id, object: 'checkout.session', mode: params.mode, url: `${base}/checkout/${id}` });
      }
      if (req.method === 'POST' && url.pathname === '/v1/billing_portal/sessions') {
        const id = rid('bps');
        portals.set(id, { id, customer: params.customer, return_url: params.return_url });
        return json(200, { id, object: 'billing_portal.session', url: `${base}/portal/${id}` });
      }
      m = /^\/v1\/customers\/([^/]+)$/.exec(url.pathname);
      if (m && req.method === 'DELETE') {
        // Deleting a customer cancels all of its subscriptions immediately (account deletion).
        const id = decodeURIComponent(m[1]);
        if (!customers.has(id)) return json(404, { error: { type: 'invalid_request_error', code: 'resource_missing' } });
        customers.delete(id);
        for (const s of subs.values()) if (s.customer === id) s.status = 'canceled';
        log(`customer ${id} deleted`);
        return json(200, { id, object: 'customer', deleted: true });
      }
      if (req.method === 'GET' && url.pathname === '/v1/subscriptions') {
        // The Worker asks before a new checkout whether the customer already subscribes.
        const st = params.status;
        const data = [...subs.values()]
          .filter((s) => !params.customer || s.customer === params.customer)
          .filter((s) => (st === 'all' ? true : st ? s.status === st : !['canceled', 'incomplete_expired'].includes(s.status)))
          .slice(0, Math.min(100, Number(params.limit || 10)));
        return json(200, { object: 'list', url: '/v1/subscriptions', has_more: false, data });
      }
      m = /^\/v1\/subscriptions\/([^/]+)$/.exec(url.pathname);
      if (m) {
        const s = subs.get(decodeURIComponent(m[1]));
        if (!s) return json(404, { error: { type: 'invalid_request_error', code: 'resource_missing' } });
        if (req.method === 'DELETE') s.status = 'canceled';
        return json(200, s);
      }
      return json(404, { error: { type: 'invalid_request_error', code: 'unknown_path' } });
    });
  });

  return new Promise((resolve, reject) => {
    server.on('error', reject);
    server.listen(port, host, () => resolve({ url: base, close: () => new Promise((r) => server.close(() => r())) }));
  });
}
