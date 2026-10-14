// Magic-link auth and the browser CSRF rules (openapi.yaml tag "auth").
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { api, browser, assertError, login, outbox, email, sidCookie, fakeIp, ORIGIN, PUBLIC_URL } from './lib.mjs';

const loginBody = (addr) => ({ email: addr, age_confirmed: true });

test('login: always 204, for new and for known e-mails', async () => {
  const known = await login();
  const a = await api('POST', '/api/auth/login', { body: loginBody(known.email), origin: ORIGIN });
  assert.equal(a.status, 204);
  const b = await api('POST', '/api/auth/login', { body: loginBody(email('new')), origin: ORIGIN });
  assert.equal(b.status, 204);
  assert.equal(a.text, '');
  assert.equal(b.text, '');
});

test('login: e-mail link points at /login/confirm on PUBLIC_URL', async () => {
  const addr = email();
  const r = await api('POST', '/api/auth/login', { body: loginBody(addr), origin: ORIGIN });
  assert.equal(r.status, 204);
  const box = await outbox(addr);
  const link = new URL(box.link);
  assert.equal(link.origin, new URL(PUBLIC_URL).origin);
  assert.equal(link.pathname, '/login/confirm');
  assert.equal(link.searchParams.get('token'), box.token);
  assert.ok(box.token.length >= 32 && box.token.length <= 128);
});

test('login: missing or false age_confirmed, bad e-mail, extra fields -> 400 bad_request', async () => {
  for (const body of [{ email: email() }, { email: email(), age_confirmed: false }, { email: 'not-an-email', age_confirmed: true }, { ...loginBody(email()), admin: true }]) {
    const r = await api('POST', '/api/auth/login', { body, origin: ORIGIN });
    assertError(r, 400, 'bad_request');
  }
});

test('login: bad or missing Origin -> 403 forbidden', async () => {
  for (const origin of ['https://evil.example', 'null', null]) {
    const r = await api('POST', '/api/auth/login', { body: loginBody(email()), origin });
    assertError(r, 403, 'forbidden');
  }
});

test('login: wrong content type -> 403 forbidden (form posts cannot forge it)', async () => {
  for (const ct of ['text/plain', 'application/x-www-form-urlencoded']) {
    const r = await api('POST', '/api/auth/login', { rawBody: JSON.stringify(loginBody(email())), contentType: ct, origin: ORIGIN });
    assertError(r, 403, 'forbidden');
  }
});

test('login: per-IP limit 20/hour -> 429 rate_limited', async () => {
  const ip = fakeIp();
  let last;
  for (let i = 0; i < 21; i++) last = await api('POST', '/api/auth/login', { body: loginBody(email('ip')), origin: ORIGIN, ip });
  assertError(last, 429, 'rate_limited');
});

test('confirm: sets sid cookie HttpOnly, SameSite=Lax, Path=/; GET /api/me works', async () => {
  const addr = email();
  await api('POST', '/api/auth/login', { body: loginBody(addr), origin: ORIGIN });
  const { token } = await outbox(addr);
  const c = await api('POST', '/api/auth/confirm', { body: { token }, origin: ORIGIN });
  assert.equal(c.status, 204);
  const sid = sidCookie(c);
  assert.ok(sid, 'sid cookie set');
  assert.ok(sid.value.length >= 32, 'sid is long and random');
  assert.ok(sid.attrs.includes('httponly'), sid.raw);
  assert.ok(sid.attrs.includes('samesite=lax'), sid.raw);
  assert.ok(sid.attrs.includes('path=/'), sid.raw);
  assert.ok(sid.attrs.some((a) => a.startsWith('max-age=')), sid.raw);
  const me = await api('GET', '/api/me', { cookie: `sid=${sid.value}` });
  assert.equal(me.status, 200);
  assert.equal(me.json.email.toLowerCase(), addr.toLowerCase());
  assert.equal(me.json.plan, 'none', 'no device yet -> no trial');
  assert.deepEqual(me.json.devices, []);
});

test('confirm: token is single use (second confirm -> 400 invalid_token)', async () => {
  const addr = email();
  await api('POST', '/api/auth/login', { body: loginBody(addr), origin: ORIGIN });
  const { token } = await outbox(addr);
  const first = await api('POST', '/api/auth/confirm', { body: { token }, origin: ORIGIN });
  assert.equal(first.status, 204);
  const second = await api('POST', '/api/auth/confirm', { body: { token }, origin: ORIGIN });
  assertError(second, 400, 'invalid_token');
});

test('confirm: unknown token -> 400 invalid_token; bad Origin -> 403', async () => {
  const r = await api('POST', '/api/auth/confirm', { body: { token: 'x'.repeat(43) }, origin: ORIGIN });
  assertError(r, 400, 'invalid_token');
  const addr = email();
  await api('POST', '/api/auth/login', { body: loginBody(addr), origin: ORIGIN });
  const { token } = await outbox(addr);
  const f = await api('POST', '/api/auth/confirm', { body: { token }, origin: 'https://evil.example' });
  assertError(f, 403, 'forbidden');
  const ok = await api('POST', '/api/auth/confirm', { body: { token }, origin: ORIGIN });
  assert.equal(ok.status, 204, 'a rejected cross-origin attempt does not burn the token');
});

test('login e-mail is case-insensitive (same account)', async () => {
  // Assumes /__dev/outbox looks e-mails up case-insensitively, like users.email (COLLATE NOCASE).
  const u = await login(email('Case'));
  const again = await login(u.email.toUpperCase());
  const a = await api('GET', '/api/me', { cookie: u.cookie });
  const b = await api('GET', '/api/me', { cookie: again.cookie });
  assert.equal(a.json.email.toLowerCase(), b.json.email.toLowerCase());
});

test('logout: 204, clears cookie, session no longer works', async () => {
  const u = await login();
  const r = await browser('POST', '/api/auth/logout', u.cookie);
  assert.equal(r.status, 204);
  const cleared = sidCookie(r);
  assert.ok(cleared && (cleared.value === '' || cleared.attrs.includes('max-age=0') || cleared.attrs.some((a) => a.startsWith('expires='))), 'cookie cleared');
  const me = await api('GET', '/api/me', { cookie: u.cookie });
  assertError(me, 401, 'unauthorized');
});

test('logout: bad Origin -> 403 and the session survives', async () => {
  const u = await login();
  const r = await api('POST', '/api/auth/logout', { cookie: u.cookie, body: {}, origin: 'https://evil.example' });
  assertError(r, 403, 'forbidden');
  const me = await api('GET', '/api/me', { cookie: u.cookie });
  assert.equal(me.status, 200);
});

test('/api/me: 401 unauthorized without cookie or with a junk cookie', async () => {
  assertError(await api('GET', '/api/me'), 401, 'unauthorized');
  assertError(await api('GET', '/api/me', { cookie: 'sid=' + 'a'.repeat(43) }), 401, 'unauthorized');
});
