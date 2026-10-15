// Limits that need a Worker started with tiny settings. run-all.mjs runs this file in
// extra passes and sets LIMITS_PASS:
//   quota  MONTHLY_SOLVES=2, MAX_NEW_TRIALS_PER_DAY=1
//   cap    GLOBAL_DAILY_CAP_USD=0.01 (below the worst case of a single solve)
// With no LIMITS_PASS (e.g. plain `npm test`) everything here is skipped.
import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  api, assertError, register, registerEnrolled, login, claim, linkedDevice, solve, solveBody,
  mockCount, sleep, ADMIN_TOKEN,
} from './lib.mjs';

const PASS = process.env.LIMITS_PASS || '';
const only = (p) => (PASS === p ? false : `runs in the "${p}" pass of npm run test:full`);

async function noUpstream(fn) {
  const before = await mockCount();
  const out = await fn();
  await sleep(100);
  assert.equal(await mockCount(), before, 'a rejected solve must not call Anthropic');
  return out;
}

test('quota: 3rd solve with MONTHLY_SOLVES=2 -> 429 quota_exceeded, no upstream call', { skip: only('quota') }, async () => {
  const { dev, user } = await linkedDevice();
  for (let i = 0; i < 2; i++) {
    const r = await solve(dev.token, solveBody());
    assert.equal(r.status, 200, `solve ${i + 1}: ${r.text.slice(0, 200)}`);
    await sleep(300); // settlement in waitUntil
  }
  const r = await noUpstream(() => solve(dev.token, solveBody()));
  const msg = assertError(r, 429, 'quota_exceeded');
  assert.equal(msg, 'Monthly AI limit reached. It resets on the 1st.');
  assert.ok(r.headers.get('retry-after'));
  const me = await api('GET', '/api/me', { cookie: user.cookie });
  assert.equal(me.json.usage.solves_this_month, 2);
  assert.equal(me.json.usage.solves_limit, 2);
  const s = await api('GET', '/v1/device/status', { token: dev.token });
  assert.equal(s.json.can_solve, false, 'status agrees once the quota is used up');
});

test('quota: the limit is per account (another account still solves)', { skip: only('quota') }, async () => {
  const { dev } = await linkedDevice();
  const r = await solve(dev.token, solveBody());
  assert.equal(r.status, 200, r.text.slice(0, 200));
});

test('trial cap: MAX_NEW_TRIALS_PER_DAY=1 -> 2nd eligible claim 429 rate_limited, code not consumed', {
  skip: only('quota') || (ADMIN_TOKEN ? false : 'needs ADMIN_TOKEN'),
}, async () => {
  const first = await registerEnrolled();
  const a = await login();
  assert.equal((await claim(a.cookie, first.pair_code)).status, 201);
  assert.equal((await api('GET', '/v1/device/status', { token: first.token })).json.plan, 'trial');

  const second = await registerEnrolled();
  const b = await login();
  const r = await claim(b.cookie, second.pair_code);
  assertError(r, 429, 'rate_limited');
  const ra = Number(r.headers.get('retry-after'));
  const toMidnight = 86400 - (Math.floor(Date.now() / 1000) % 86400);
  assert.ok(ra > 0 && ra <= toMidnight + 5, `Retry-After until UTC midnight (${ra} vs ${toMidnight})`);

  const s = await api('GET', '/v1/device/status', { token: second.token });
  assert.equal(s.json.linked, false, 'the device is still unlinked');
  // The code is still claimable: account a already had a trial, so for a the cap is not
  // the only blocker and the claim goes through (plan stays a's trial).
  const c = await claim(a.cookie, second.pair_code);
  assert.equal(c.status, 201, `code was consumed by the 429: ${c.text}`);
});

test('trial cap does not block unenrolled boards (they never get a trial anyway)', { skip: only('quota') }, async () => {
  const d = await register();
  const u = await login();
  assert.equal((await claim(u.cookie, d.pair_code)).status, 201);
});

test('spend cap: GLOBAL_DAILY_CAP_USD below one solve -> 503 spend_cap, no upstream call', { skip: only('cap') }, async () => {
  const { dev } = await linkedDevice();
  const r = await noUpstream(() => solve(dev.token, solveBody()));
  const msg = assertError(r, 503, 'spend_cap');
  assert.equal(msg, 'AI is busy right now. Please try again later.');
});
