// Device protocol: register, pair/start, status, token errors (openapi.yaml tag "device").
import { test } from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {
  api, assertError, register, registerEnrolled, login, claim, hwId, fakeIp, now,
  TOKEN_RE, PAIR_RE, DEVICE_ID_RE, PUBLIC_URL, browser,
} from './lib.mjs';

test('healthz is ok and reveals nothing', async () => {
  const r = await api('GET', '/healthz');
  assert.equal(r.status, 200);
  assert.ok(!/sonnet|claude|sk-ant|anthropic/i.test(r.text), 'healthz must not reveal model or keys');
});

test('register: 201 shape, token and pair code formats', async () => {
  const hw = hwId();
  const r = await api('POST', '/v1/device/register', { body: { hw_id: hw }, headers: { 'x-firmware': 'test-1.0' } });
  assert.equal(r.status, 201);
  const j = r.json;
  assert.match(j.device_id, DEVICE_ID_RE);
  assert.match(j.token, TOKEN_RE);
  assert.match(j.pair_code, PAIR_RE);
  const link = new URL(j.link_url);
  assert.equal(link.origin, new URL(PUBLIC_URL).origin, 'link_url is built from PUBLIC_URL');
  assert.equal(link.pathname, '/link');
  assert.equal(link.searchParams.get('code')?.replace('-', ''), j.pair_code.replace('-', ''));
  assert.ok(j.expires_at > now() + 14 * 60 - 30 && j.expires_at <= now() + 15 * 60 + 30, 'pair code valid about 15 min');
});

test('register twice from the same hw_id gives two different devices and tokens', async () => {
  const hw = hwId();
  const a = await register({ hw_id: hw });
  const b = await register({ hw_id: hw });
  assert.notEqual(a.device_id, b.device_id);
  assert.notEqual(a.token, b.token);
});

test('register: hw_id validation -> 400 bad_request', async () => {
  for (const body of [{}, { hw_id: 'calc-XYZ' }, { hw_id: 'calc-0123456789ab', extra: 1 }, { hw_id: 'calc-0123456789abc' }, { hw_id: 42 }]) {
    const r = await api('POST', '/v1/device/register', { body });
    assertError(r, 400, 'bad_request');
  }
  const r = await api('POST', '/v1/device/register', { rawBody: '{not json', contentType: 'application/json' });
  assertError(r, 400, 'bad_request');
});

test('register: 5 per IP per hour, 6th -> 429 rate_limited with Retry-After', async () => {
  const ip = fakeIp();
  for (let i = 0; i < 5; i++) {
    const r = await api('POST', '/v1/device/register', { body: { hw_id: hwId() }, ip });
    assert.equal(r.status, 201, `call ${i + 1} should pass (does the Worker honour x-dev-client-ip in DEV_MODE?)`);
  }
  const r = await api('POST', '/v1/device/register', { body: { hw_id: hwId() }, ip });
  assertError(r, 429, 'rate_limited');
});

test('status before claim: not linked, cannot solve, reason has the code', async () => {
  const d = await register();
  const r = await api('GET', '/v1/device/status', { token: d.token, headers: { 'x-device-id': d.hw_id, 'x-firmware': 'test-1.0' } });
  assert.equal(r.status, 200);
  assert.equal(r.json.linked, false);
  assert.equal(r.json.can_solve, false);
  assert.equal(r.json.plan, 'none');
  assert.ok(r.json.reason, 'reason present when can_solve is false');
  assert.equal(r.json.account_hint, undefined, 'account_hint only when linked');
});

test('status after claim (enrolled board): linked, trial, can solve, masked account hint', async () => {
  const d = await registerEnrolled();
  const u = await login();
  const c = await claim(u.cookie, d.pair_code);
  assert.equal(c.status, 201);
  const r = await api('GET', '/v1/device/status', { token: d.token });
  assert.equal(r.status, 200);
  assert.equal(r.json.linked, true);
  assert.equal(r.json.plan, 'trial');
  assert.equal(r.json.can_solve, true);
  assert.ok(r.json.account_hint, 'account_hint when linked');
  assert.ok(!r.json.account_hint.includes(u.email), 'account_hint must be masked');
  assert.ok(r.json.account_hint.includes('***'), 'masked like n***@gmail.com');
  assert.equal(r.json.reason, undefined, 'no reason when can_solve');
});

test('pair/start: new code replaces the old one; linked device gets already_linked', async () => {
  const d = await register();
  const r = await api('POST', '/v1/pair/start', { token: d.token });
  assert.equal(r.status, 200);
  assert.equal(r.json.already_linked, false);
  assert.match(r.json.code, PAIR_RE);
  assert.ok(r.json.link_url && r.json.expires_at);
  assert.notEqual(r.json.code.replace('-', ''), d.pair_code.replace('-', ''));

  const u = await login();
  const old = await claim(u.cookie, d.pair_code);
  assertError(old, 400, 'invalid_code');
  const ok = await claim(u.cookie, r.json.code);
  assert.equal(ok.status, 201);

  const again = await api('POST', '/v1/pair/start', { token: d.token });
  assert.equal(again.status, 200);
  assert.equal(again.json.already_linked, true);
});

test('unknown, malformed or missing token -> 401 unauthorized', async () => {
  const fake = 'dt_' + crypto.randomBytes(32).toString('base64url');
  for (const token of [fake, 'not-a-token', undefined]) {
    for (const [m, p] of [['GET', '/v1/device/status'], ['POST', '/v1/pair/start'], ['GET', '/v1/firmware']]) {
      const r = await api(m, p, { token });
      assertError(r, 401, 'unauthorized');
    }
  }
});

test('revoked token -> 401 device_revoked on every device path', async () => {
  const d = await register();
  const u = await login();
  const c = await claim(u.cookie, d.pair_code);
  assert.equal(c.status, 201);
  const rv = await browser('POST', `/api/devices/${d.device_id}/revoke`, u.cookie);
  assert.equal(rv.status, 204);
  for (const [m, p] of [['GET', '/v1/device/status'], ['POST', '/v1/pair/start'], ['GET', '/v1/firmware']]) {
    const r = await api(m, p, { token: d.token });
    const msg = assertError(r, 401, 'device_revoked');
    assert.match(msg, /removed from its account/i);
  }
  const s = await api('POST', '/v1/solve', { token: d.token, body: { images: [{ media_type: 'image/jpeg', data: '/9j/' }] } });
  assertError(s, 401, 'device_revoked');
});

test('x-device-id is optional and never used for auth', async () => {
  const d = await register();
  const without = await api('GET', '/v1/device/status', { token: d.token });
  assert.equal(without.status, 200);
  const other = await api('GET', '/v1/device/status', { token: d.token, headers: { 'x-device-id': hwId() } });
  assert.equal(other.status, 200, 'a different x-device-id does not change auth');
  const noToken = await api('GET', '/v1/device/status', { headers: { 'x-device-id': d.hw_id } });
  assertError(noToken, 401, 'unauthorized');
});
