// /__dev/* must only answer on localhost / 127.0.0.1 (PLAN 4b). Not part of openapi.yaml,
// so no contract validation here. fetch() cannot override Host, so node:http is used.
//
// Caveat: this proves the Worker checks the Host it is given. Whether wrangler dev passes
// a foreign Host header through to request.url unchanged is a wrangler detail; if this test
// fails with 200, check by hand that production (no DEV_MODE) returns 404 for /__dev/*.
import { test } from 'node:test';
import assert from 'node:assert/strict';
import http from 'node:http';
import { BASE_URL, ORIGIN, api, email } from './lib.mjs';

function raw(method, path, host, body) {
  const u = new URL(BASE_URL);
  return new Promise((resolve, reject) => {
    const req = http.request({
      host: u.hostname, port: u.port || 80, method, path,
      headers: { host, ...(body ? { 'content-type': 'application/json', 'content-length': Buffer.byteLength(body) } : {}) },
    }, (res) => {
      const chunks = [];
      res.on('data', (c) => chunks.push(c));
      res.on('end', () => resolve({ status: res.statusCode, text: Buffer.concat(chunks).toString('utf8') }));
    });
    req.on('error', reject);
    if (body) req.write(body);
    req.end();
  });
}

test('/__dev/* answers on localhost (positive control)', async () => {
  const addr = email('dev');
  await api('POST', '/api/auth/login', { body: { email: addr, age_confirmed: true }, origin: ORIGIN });
  const port = new URL(BASE_URL).port || '80';
  const r = await raw('GET', `/__dev/outbox?email=${encodeURIComponent(addr)}`, `localhost:${port}`);
  assert.equal(r.status, 200, r.text);
});

for (const host of ['example.com', 'api.aicalc.example', 'localhost.evil.example', '127.0.0.1.nip.io']) {
  test(`/__dev/* -> 404 when Host is ${host}`, async () => {
    const addr = email('dev');
    await api('POST', '/api/auth/login', { body: { email: addr, age_confirmed: true }, origin: ORIGIN });
    const o = await raw('GET', `/__dev/outbox?email=${encodeURIComponent(addr)}`, host);
    assert.equal(o.status, 404, `outbox leaked a sign-in token via Host ${host}: ${o.text.slice(0, 200)}`);
    assert.ok(!o.text.includes('token'), 'no token in the body');
    const u = await raw('POST', '/__dev/user', host, JSON.stringify({ email: addr, plan_override: 'active' }));
    assert.equal(u.status, 404, `/__dev/user reachable via Host ${host}`);
  });
}
