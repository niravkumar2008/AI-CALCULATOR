// Mock Stripe API for the stage-3 contract tests. No network, no Stripe account, no money.
//
// The calls the Worker makes:
//   POST   /v1/customers                  -> {id: cus_...}   (same id for the same Idempotency-Key)
//   POST   /v1/checkout/sessions          -> {id: cs_test_..., url: https://checkout.stripe.com/...}
//   POST   /v1/billing_portal/sessions    -> {url: https://billing.stripe.com/...}
//   DELETE /v1/customers/:id              -> {deleted: true}; cancels that customer's subscriptions
//                                            (404 if never created or already deleted)
//   GET    /v1/subscriptions?customer=&status=  -> {data: [...]} (status=all: every status;
//                                            no status: all but canceled/incomplete_expired)
//   GET    /v1/subscriptions/:id          -> the seeded subscription (404 if unknown)
//   DELETE /v1/subscriptions/:id          -> the subscription, now status=canceled
// Test controls:
//   POST /__subscriptions   body = subscription object  (seed or replace)
//   GET  /__calls           [{method, path, params, idempotencyKey}]
//   GET  /__customer?email= {id} of the customer created for that e-mail
//   POST /__fail            {prefix, status, method?}: the next call (of that method, if given)
//                           whose path starts with prefix fails
//   POST /__reset
// Every /v1/* call must carry `Authorization: Bearer <MOCK_STRIPE_KEY>`.

import http from 'node:http';
import crypto from 'node:crypto';
import { pathToFileURL } from 'node:url';

export const MOCK_STRIPE_KEY = 'sk_test_mockStripeKey0123456789';

export function startStripeMock({ port = 8789, host = '127.0.0.1' } = {}) {
  let calls = [];
  let failures = [];
  const subs = new Map();
  const customersByKey = new Map();
  const customersByEmail = new Map();
  const customers = new Set();
  const deletedCustomers = new Set();
  const id = (p) => `${p}_${crypto.randomBytes(7).toString('hex')}`;

  const server = http.createServer((req, res) => {
    let body = '';
    req.on('data', (c) => (body += c));
    req.on('end', () => {
      const url = new URL(req.url, `http://${host}`);
      const send = (status, obj) => {
        res.writeHead(status, { 'content-type': 'application/json' });
        res.end(JSON.stringify(obj));
      };
      // ---- controls
      if (url.pathname === '/healthz') return send(200, { ok: true });
      if (url.pathname === '/__reset') {
        calls = [];
        failures = [];
        subs.clear();
        return send(200, { ok: true });
      }
      if (url.pathname === '/__calls') return send(200, calls);
      if (url.pathname === '/__customer') {
        const c = customersByEmail.get(url.searchParams.get('email') ?? '');
        return c ? send(200, { id: c }) : send(404, { error: 'unknown' });
      }
      if (url.pathname === '/__subscriptions' && req.method === 'POST') {
        const s = JSON.parse(body);
        subs.set(s.id, s);
        return send(200, s);
      }
      if (url.pathname === '/__fail' && req.method === 'POST') {
        failures.push(JSON.parse(body));
        return send(200, { ok: true });
      }

      // ---- the Stripe API surface
      const params = Object.fromEntries(new URLSearchParams(req.method === 'GET' ? url.search.slice(1) : body));
      const idem = req.headers['idempotency-key'] ?? null;
      calls.push({ method: req.method, path: url.pathname, params, idempotencyKey: idem });
      const f = failures.findIndex((x) => url.pathname.startsWith(x.prefix) && (!x.method || x.method === req.method));
      if (f >= 0) {
        const { status = 500 } = failures.splice(f, 1)[0];
        return send(status, { error: { type: 'api_error', code: 'mock_failure', message: 'mock failure' } });
      }
      if (req.headers.authorization !== `Bearer ${MOCK_STRIPE_KEY}`) {
        return send(401, { error: { type: 'invalid_request_error', code: 'unauthorized' } });
      }
      if (req.method === 'POST' && url.pathname === '/v1/customers') {
        const key = idem ?? id('k');
        let cid = customersByKey.get(key);
        if (!cid) customersByKey.set(key, (cid = id('cus')));
        if (params.email) customersByEmail.set(params.email, cid);
        customers.add(cid);
        return send(200, { id: cid, object: 'customer', email: params.email });
      }
      const cm = /^\/v1\/customers\/([^/]+)$/.exec(url.pathname);
      if (cm && req.method === 'DELETE') {
        const cid = decodeURIComponent(cm[1]);
        if (!customers.has(cid) || deletedCustomers.has(cid)) {
          return send(404, { error: { type: 'invalid_request_error', code: 'resource_missing' } });
        }
        deletedCustomers.add(cid);
        for (const s of subs.values()) if (s.customer === cid) s.status = 'canceled';
        return send(200, { id: cid, object: 'customer', deleted: true });
      }
      if (req.method === 'POST' && url.pathname === '/v1/checkout/sessions') {
        const sid = id('cs_test');
        return send(200, {
          id: sid,
          object: 'checkout.session',
          mode: params.mode,
          expires_at: params.expires_at ? Number(params.expires_at) : undefined,
          url: `https://checkout.stripe.com/c/pay/${sid}`,
        });
      }
      if (req.method === 'GET' && url.pathname === '/v1/subscriptions') {
        const st = params.status;
        const data = [...subs.values()]
          .filter((s) => !params.customer || s.customer === params.customer)
          .filter((s) => (st === 'all' ? true : st ? s.status === st : !['canceled', 'incomplete_expired'].includes(s.status)))
          .slice(0, Math.min(100, Number(params.limit || 10)));
        return send(200, { object: 'list', url: '/v1/subscriptions', has_more: false, data });
      }
      if (req.method === 'POST' && url.pathname === '/v1/billing_portal/sessions') {
        return send(200, { id: id('bps'), object: 'billing_portal.session', url: `https://billing.stripe.com/p/session/${id('test')}` });
      }
      const m = /^\/v1\/subscriptions\/([^/]+)$/.exec(url.pathname);
      if (m) {
        const s = subs.get(decodeURIComponent(m[1]));
        if (!s) return send(404, { error: { type: 'invalid_request_error', code: 'resource_missing' } });
        if (req.method === 'DELETE') s.status = 'canceled';
        return send(200, s);
      }
      return send(404, { error: { type: 'invalid_request_error', code: 'unknown_path' } });
    });
  });

  return new Promise((resolve) => {
    server.listen(port, host, () =>
      resolve({
        url: `http://${host}:${port}`,
        close: () => new Promise((r) => server.close(() => r())),
      }),
    );
  });
}

// Run alone: node mock-stripe.mjs
if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  const m = await startStripeMock({ port: Number(process.env.STRIPE_MOCK_PORT || 8789) });
  console.log(`mock Stripe on ${m.url}`);
}
