// Portal account API: claim, rename, revoke, trial start, account deletion (tag "account").
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { api, browser, assertError, register, login, claim, now, ORIGIN, DEVICE_ID_RE } from './lib.mjs';

test('claim with the code as shown (XXXX-XXXX) -> 201 Device', async () => {
  const d = await register();
  const u = await login();
  const r = await claim(u.cookie, d.pair_code.includes('-') ? d.pair_code : d.pair_code.slice(0, 4) + '-' + d.pair_code.slice(4), 'Maya\'s calc');
  assert.equal(r.status, 201, r.text);
  assert.equal(r.json.id, d.device_id);
  assert.match(r.json.id, DEVICE_ID_RE);
  assert.equal(r.json.label, 'Maya\'s calc');
  assert.equal(r.json.revoked, false);
  assert.equal(r.json.hw_id, d.hw_id);
});

test('claim without dash, lower case, with spaces -> 201 (codes are normalised)', async () => {
  const plain = (c) => c.replace('-', '');
  for (const variant of [(c) => plain(c), (c) => plain(c).toLowerCase(), (c) => ` ${plain(c).slice(0, 4)} ${plain(c).slice(4)} `]) {
    const d = await register();
    const u = await login();
    const r = await claim(u.cookie, variant(d.pair_code));
    assert.equal(r.status, 201, `variant "${variant(d.pair_code)}": ${r.status} ${r.text}`);
  }
});

test('invalid code: unknown and already used give the same 400 invalid_code message', async () => {
  const u = await login();
  const unknown = await claim(u.cookie, 'ZZZZ-ZZZZ');
  const m1 = assertError(unknown, 400, 'invalid_code');
  const d = await register();
  const other = await login();
  assert.equal((await claim(other.cookie, d.pair_code)).status, 201);
  const used = await claim(u.cookie, d.pair_code);
  const m2 = assertError(used, 400, 'invalid_code');
  assert.equal(m1, m2, 'one message for unknown / expired / used (no oracle)');
});

test('claim race: two accounts claim the same code concurrently -> exactly one 201', async () => {
  const d = await register();
  const [a, b] = await Promise.all([login(), login()]);
  const results = await Promise.all([claim(a.cookie, d.pair_code), claim(b.cookie, d.pair_code)]);
  const statuses = results.map((r) => r.status).sort();
  assert.deepEqual(statuses, [201, 400], `got ${statuses}`);
  const loser = results.find((r) => r.status === 400);
  assertError(loser, 400, 'invalid_code');
});

test('device limit: 4th active device -> 409 device_limit; revoking frees a slot', async () => {
  const u = await login();
  const devs = [];
  for (let i = 0; i < 3; i++) {
    const d = await register();
    assert.equal((await claim(u.cookie, d.pair_code)).status, 201);
    devs.push(d);
  }
  const fourth = await register();
  assertError(await claim(u.cookie, fourth.pair_code), 409, 'device_limit');
  assert.equal((await browser('POST', `/api/devices/${devs[0].device_id}/revoke`, u.cookie)).status, 204);
  const retry = await claim(u.cookie, fourth.pair_code);
  assert.equal(retry.status, 201, 'the code survives a device_limit rejection and a slot is free after revoke');
});

test('rename: PATCH label -> 200 Device with the new label', async () => {
  const d = await register();
  const u = await login();
  await claim(u.cookie, d.pair_code);
  const r = await browser('PATCH', `/api/devices/${d.device_id}`, u.cookie, { label: 'Kitchen calculator' });
  assert.equal(r.status, 200, r.text);
  assert.equal(r.json.label, 'Kitchen calculator');
  const me = await api('GET', '/api/me', { cookie: u.cookie });
  assert.equal(me.json.devices.find((x) => x.id === d.device_id).label, 'Kitchen calculator');
});

test('label validation: "<script>", empty, too long, control chars rejected', async () => {
  const d = await register();
  const u = await login();
  // On claim the 400 is documented.
  assertError(await claim(u.cookie, d.pair_code, '<script>alert(1)</script>'), 400, 'bad_request');
  assert.equal((await claim(u.cookie, d.pair_code)).status, 201, 'a rejected label does not consume the code');
  // On PATCH the 400 is a known contract gap (see contract.mjs KNOWN_GAPS).
  for (const label of ['<script>', '', 'x'.repeat(41), 'a\u0001b']) {
    const r = await browser('PATCH', `/api/devices/${d.device_id}`, u.cookie, { label });
    assertError(r, 400, 'bad_request');
  }
});

test('rename / revoke someone else\'s device -> 404 not_found (no existence oracle)', async () => {
  const d = await register();
  const owner = await login();
  const stranger = await login();
  await claim(owner.cookie, d.pair_code);
  assertError(await browser('PATCH', `/api/devices/${d.device_id}`, stranger.cookie, { label: 'mine now' }), 404, 'not_found');
  assertError(await browser('POST', `/api/devices/${d.device_id}/revoke`, stranger.cookie), 404, 'not_found');
  assertError(await browser('POST', '/api/devices/dev_0000000000000000/revoke', stranger.cookie), 404, 'not_found');
  const s = await api('GET', '/v1/device/status', { token: d.token });
  assert.equal(s.status, 200, 'the device still works for its owner');
});

test('revoke -> 204; the device then gets 401 device_revoked; portal shows revoked', async () => {
  const d = await register();
  const u = await login();
  await claim(u.cookie, d.pair_code);
  assert.equal((await browser('POST', `/api/devices/${d.device_id}/revoke`, u.cookie)).status, 204);
  assertError(await api('GET', '/v1/device/status', { token: d.token }), 401, 'device_revoked');
  const me = await api('GET', '/api/me', { cookie: u.cookie });
  const dev = me.json.devices.find((x) => x.id === d.device_id);
  assert.ok(!dev || dev.revoked === true, 'revoked device is hidden or marked revoked');
  // Re-registering gives a fresh, unlinked token.
  const again = await register({ hw_id: d.hw_id });
  const s = await api('GET', '/v1/device/status', { token: again.token });
  assert.equal(s.json.linked, false);
});

test('account endpoints need a session and the right Origin / Content-Type', async () => {
  const d = await register();
  assertError(await api('POST', '/api/devices/claim', { body: { code: d.pair_code }, origin: ORIGIN }), 401, 'unauthorized');
  const u = await login();
  assertError(await api('POST', '/api/devices/claim', { cookie: u.cookie, body: { code: d.pair_code }, origin: 'https://evil.example' }), 403, 'forbidden');
  assertError(await api('POST', '/api/devices/claim', { cookie: u.cookie, rawBody: JSON.stringify({ code: d.pair_code }), contentType: 'text/plain', origin: ORIGIN }), 403, 'forbidden');
  assert.equal((await claim(u.cookie, d.pair_code)).status, 201, 'rejected CSRF attempts did not consume the code');
});

test('first claim starts the 30-day trial; a second device does not restart it', async () => {
  const u = await login();
  const before = await api('GET', '/api/me', { cookie: u.cookie });
  assert.equal(before.json.plan, 'none');
  const d1 = await register();
  await claim(u.cookie, d1.pair_code);
  const s = await api('GET', '/v1/device/status', { token: d1.token });
  assert.equal(s.json.plan, 'trial');
  const days = (s.json.trial_ends_at - now()) / 86400;
  assert.ok(days > 29.9 && days <= 30.01, `trial_ends_at ~30 days out, got ${days.toFixed(2)} days`);
  const me = await api('GET', '/api/me', { cookie: u.cookie });
  assert.equal(me.json.plan, 'trial');
  assert.equal(me.json.trial_ends_at, s.json.trial_ends_at);
  assert.equal(me.json.usage.solves_limit, Number(process.env.EXPECT_MONTHLY_SOLVES || 300));
  assert.equal(me.json.usage.solves_this_month, 0);

  await new Promise((r) => setTimeout(r, 1100));
  const d2 = await register();
  await claim(u.cookie, d2.pair_code);
  const me2 = await api('GET', '/api/me', { cookie: u.cookie });
  assert.equal(me2.json.trial_ends_at, me.json.trial_ends_at, 'trial not restarted by another device');
});

test('DELETE /api/me requires the matching confirm_email; then revokes devices and the session', async () => {
  const d = await register();
  const u = await login();
  await claim(u.cookie, d.pair_code);
  assertError(await browser('DELETE', '/api/me', u.cookie, { confirm_email: 'someone-else@example.test' }), 400, 'bad_request');
  assertError(await browser('DELETE', '/api/me', u.cookie, {}), 400, 'bad_request');
  assertError(await api('DELETE', '/api/me', { cookie: u.cookie, body: { confirm_email: u.email }, origin: 'https://evil.example' }), 403, 'forbidden');
  assert.equal((await api('GET', '/api/me', { cookie: u.cookie })).status, 200, 'still there after the rejected attempts');

  const ok = await browser('DELETE', '/api/me', u.cookie, { confirm_email: u.email.toUpperCase() });
  assert.equal(ok.status, 204, ok.text);
  assertError(await api('GET', '/api/me', { cookie: u.cookie }), 401, 'unauthorized');
  const s = await api('GET', '/v1/device/status', { token: d.token });
  assert.equal(s.status, 401);
  assert.ok(['device_revoked', 'unauthorized'].includes(s.json.error.type));
});
