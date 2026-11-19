// QR linking, way 2 (docs/LINK_QR_CONTRACT.md): link tokens, /v1/pair/redeem, Approve / No,
// link_state (tag "link"). Every call is checked against openapi.yaml by api().
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { api, browser, assertError, register, registerEnrolled, login, claim, now, BASE_URL, PUBLIC_URL } from './lib.mjs';

const LT_ID_RE = /^lt_[0-9a-f]{16}$/;
const LT_RE = /^L1\.[A-Za-z0-9_-]{22}$/;

async function newToken(cookie) {
  const r = await browser('POST', '/api/link-tokens', cookie);
  assert.equal(r.status, 201, r.text);
  return r.json;
}
const redeem = (d, link_token, ip) => api('POST', '/v1/pair/redeem', { token: d.token, body: { link_token }, ip });
const poll = (cookie, id) => api('GET', `/api/link-tokens/${id}`, { cookie });
const approve = (cookie, id, yes) => browser('POST', `/api/link-tokens/${id}/approve`, cookie, { approve: yes });
const linkState = async (d) => (await api('GET', '/v1/device/status', { token: d.token })).json.link_state;
const suffix = (d) => d.hw_id.slice(-4).toUpperCase();

test('camera link: token -> redeem -> pending -> Approve -> linked, with a notice e-mail', async () => {
  const u = await login();
  const t = await newToken(u.cookie);
  assert.match(t.id, LT_ID_RE);
  assert.match(t.token, LT_RE);
  assert.equal(t.qr_url, `${PUBLIC_URL}/q/${t.token}`);
  assert.ok(Math.abs(t.expires_at - (now() + 600)) <= 5);
  assert.equal((await poll(u.cookie, t.id)).json.state, 'waiting');

  const d = await registerEnrolled();
  assert.equal(await linkState(d), '');
  const r = await redeem(d, t.token);
  assert.equal(r.status, 200, r.text);
  assert.equal(r.json.state, 'pending_approval');
  assert.equal(r.json.suffix, suffix(d));
  assert.equal(await linkState(d), 'pending_approval');
  const p = await poll(u.cookie, t.id);
  assert.equal(p.json.state, 'pending_approval');
  assert.deepEqual(p.json.device, { id: d.device_id, suffix: suffix(d), label: 'Calculator' });

  const a = await approve(u.cookie, t.id, true);
  assert.equal(a.status, 200, a.text);
  assert.equal(a.json.state, 'approved');
  const s = await api('GET', '/v1/device/status', { token: d.token });
  assert.equal(s.json.linked, true);
  assert.equal(s.json.plan, 'trial');
  assert.equal(s.json.link_state, '');
  const mail = await fetch(`${BASE_URL}/__dev/outbox?email=${encodeURIComponent(u.email)}&kind=linked`);
  assert.equal(mail.status, 200, 'the "calculator linked" notice is in the dev outbox');
  assertError(await approve(u.cookie, t.id, true), 409, 'link_token_state');
});

test('redeem errors: invalid 404, expired (replaced) 410, used 409, other account 409 already_linked', async () => {
  const d = await register();
  assertError(await redeem(d, 'L1.short'), 404, 'link_token_invalid');
  assertError(await redeem(d, 'L1.' + 'a'.repeat(22)), 404, 'link_token_invalid');

  const u = await login();
  const old = await newToken(u.cookie);
  const t = await newToken(u.cookie); // cancels `old`
  assertError(await redeem(d, old.token), 410, 'link_token_expired');
  assert.equal((await redeem(d, t.token)).status, 200);
  assertError(await redeem(await register(), t.token), 409, 'link_token_used');

  const owner = await login();
  const mine = await register();
  assert.equal((await claim(owner.cookie, mine.pair_code)).status, 201);
  const t2 = await newToken(u.cookie);
  assertError(await redeem(mine, t2.token), 409, 'already_linked');
});

test('same-account relink -> 200 linked; No -> declined; 1-device slot replaces on Approve', async () => {
  const u = await login();
  const d1 = await register();
  assert.equal((await claim(u.cookie, d1.pair_code)).status, 201);
  const t1 = await newToken(u.cookie);
  const same = await redeem(d1, t1.token);
  assert.equal(same.status, 200, same.text);
  assert.equal(same.json.state, 'linked');

  const d2 = await register();
  const t2 = await newToken(u.cookie);
  await redeem(d2, t2.token);
  assert.equal((await poll(u.cookie, t2.id)).json.replaces.id, d1.device_id);
  const no = await approve(u.cookie, t2.id, false);
  assert.equal(no.json.state, 'declined');
  assert.equal(await linkState(d2), 'declined');

  const t3 = await newToken(u.cookie);
  await redeem(d2, t3.token);
  assert.equal((await approve(u.cookie, t3.id, true)).status, 200);
  assertError(await api('GET', '/v1/device/status', { token: d1.token }), 401, 'device_revoked');
  assert.equal((await api('GET', '/v1/device/status', { token: d2.token })).json.linked, true);
});

test('owner only and rate limits', async () => {
  const a = await login();
  const b = await login();
  const t = await newToken(a.cookie);
  assertError(await poll(b.cookie, t.id), 404, 'not_found');
  const d = await register();
  const ip = '10.250.0.1';
  let last;
  for (let i = 0; i < 11; i++) last = await redeem(d, 'L1.' + 'z'.repeat(22), ip);
  assertError(last, 429, 'rate_limited');
});
