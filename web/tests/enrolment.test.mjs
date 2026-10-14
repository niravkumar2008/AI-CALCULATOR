// Board enrolment and free-month eligibility (PLAN 4c S1/S2; openapi.yaml
// /v1/admin/hardware, register hw_secret, /api/devices/claim description).
// The daily new-trial cap (429, code not consumed) is in limits.test.mjs (quota pass).
import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  api, browser, assertError, register, registerEnrolled, enrol, login, claim, hwId, hwSecret,
  ADMIN_TOKEN, PUBLIC_HOST, solveBody, solve, mockCount, sleep, email,
} from './lib.mjs';

const skip = ADMIN_TOKEN ? false : 'ADMIN_TOKEN not given to the test run (npm run test:full sets one)';
const NO_TRIAL = () => `No free month for this calculator. Subscribe at ${PUBLIC_HOST}/account to use AI.`;
const hw = (body, token = ADMIN_TOKEN) =>
  api('POST', '/v1/admin/hardware', { body, headers: token === null ? {} : { 'x-admin-token': token } });

async function status(token) {
  const r = await api('GET', '/v1/device/status', { token });
  assert.equal(r.status, 200, r.text);
  return r.json;
}

/** The device is linked but has no free month: plan none, cannot solve, the 4c reason. */
async function assertNoTrial(dev) {
  const s = await status(dev.token);
  assert.equal(s.linked, true);
  assert.equal(s.plan, 'none', `expected plan none, got ${s.plan}`);
  assert.equal(s.can_solve, false);
  assert.equal(s.reason, NO_TRIAL());
}

test('admin/hardware: missing or wrong token -> 401 unauthorized', { skip }, async () => {
  const body = { hw_id: hwId(), hw_secret: hwSecret() };
  assertError(await hw(body, null), 401, 'unauthorized');
  assertError(await hw(body, 'x'.repeat(48)), 401, 'unauthorized');
});

test('admin/hardware: bad bodies -> 400 bad_request', { skip }, async () => {
  for (const body of [
    {},
    { hw_id: hwId() },
    { hw_id: 'calc-XYZ', hw_secret: hwSecret() },
    { hw_id: hwId(), hw_secret: 'hs_short' },
    { hw_id: hwId(), hw_secret: 'xx_' + 'A'.repeat(43) },
    { hw_id: hwId(), hw_secret: hwSecret(), trial_used: false },
  ]) {
    assertError(await hw(body), 400, 'bad_request');
  }
});

test('admin/hardware: 201 { hw_id, trial_used: false } for a new board', { skip }, async () => {
  const body = { hw_id: hwId(), hw_secret: hwSecret() };
  const r = await hw(body);
  assert.equal(r.status, 201, r.text);
  assert.deepEqual(r.json, { hw_id: body.hw_id, trial_used: false });
});

test('register: malformed hw_secret -> 400; no hw_secret still registers (dev boards)', async () => {
  for (const hw_secret of ['hs_short', 'noths_' + 'A'.repeat(43), 42]) {
    assertError(await api('POST', '/v1/device/register', { body: { hw_id: hwId(), hw_secret } }), 400, 'bad_request');
  }
  const d = await register();
  assert.ok(d.token);
});

test('unenrolled board: links with plan none and the "No free month" reason; solve 402', async () => {
  const d = await register();
  const u = await login();
  const c = await claim(u.cookie, d.pair_code);
  assert.equal(c.status, 201, c.text);
  await assertNoTrial(d);
  const me = await api('GET', '/api/me', { cookie: u.cookie });
  assert.equal(me.json.plan, 'none');
  const n0 = await mockCount();
  const s = await solve(d.token, solveBody());
  assert.equal(s.status, 402, s.text);
  assert.equal(s.json.error.type, 'subscription_inactive');
  assert.equal(s.json.error.message, NO_TRIAL());
  await sleep(100);
  assert.equal(await mockCount(), n0, 'no upstream call');
});

test('wrong hw_secret for an enrolled hw_id is not enrolled: no free month', { skip }, async () => {
  const b = await enrol();
  const r = await api('POST', '/v1/device/register', { body: { hw_id: b.hw_id, hw_secret: hwSecret() } });
  if (r.status === 400) return; // rejecting the mismatch outright is also contract-compatible
  assert.equal(r.status, 201, r.text);
  const u = await login();
  assert.equal((await claim(u.cookie, r.json.pair_code)).status, 201);
  await assertNoTrial(r.json);
});

test('enrolled board: free month once per board ever (revoke + re-register, re-enrol)', { skip }, async () => {
  const b = await enrol();
  const d1 = await register(b);
  const u1 = await login();
  assert.equal((await claim(u1.cookie, d1.pair_code)).status, 201);
  assert.equal((await status(d1.token)).plan, 'trial');

  // Owner removes it; the same board re-registers and a different, fresh account claims it.
  assert.equal((await browser('POST', `/api/devices/${d1.device_id}/revoke`, u1.cookie)).status, 204);
  const d2 = await register(b);
  const u2 = await login();
  assert.equal((await claim(u2.cookie, d2.pair_code)).status, 201);
  await assertNoTrial(d2);

  // Re-enrolling replaces the secret but never resets trial_used.
  const again = await hw({ hw_id: b.hw_id, hw_secret: hwSecret() });
  assert.equal(again.status, 201, again.text);
  assert.equal(again.json.trial_used, true);
});

test('account that already had a trial: another enrolled board links but the board keeps its trial', { skip }, async () => {
  const u = await login();
  const d1 = await registerEnrolled();
  assert.equal((await claim(u.cookie, d1.pair_code)).status, 201);
  const b2 = await enrol();
  const d2 = await register(b2);
  assert.equal((await claim(u.cookie, d2.pair_code)).status, 201);
  const s = await status(d2.token);
  assert.equal(s.plan, 'trial', 'still under the account\'s one trial');
  const re = await hw({ hw_id: b2.hw_id, hw_secret: b2.hw_secret });
  assert.equal(re.json.trial_used, false, 'board 2 trial not spent: the account was the blocker');
});

test('free month once per e-mail, even after account deletion', { skip }, async () => {
  const addr = email('once');
  const u1 = await login(addr);
  const d1 = await registerEnrolled();
  assert.equal((await claim(u1.cookie, d1.pair_code)).status, 201);
  assert.equal((await status(d1.token)).plan, 'trial');
  assert.equal((await browser('DELETE', '/api/me', u1.cookie, { confirm_email: addr })).status, 204);

  const u2 = await login(addr); // same e-mail, brand-new account
  const me = await api('GET', '/api/me', { cookie: u2.cookie });
  assert.equal(me.json.plan, 'none');
  const b = await enrol();
  const d2 = await register(b);
  assert.equal((await claim(u2.cookie, d2.pair_code)).status, 201);
  await assertNoTrial(d2);
  const re = await hw({ hw_id: b.hw_id, hw_secret: b.hw_secret });
  assert.equal(re.json.trial_used, false, 'the e-mail was the blocker, the board keeps its trial');
});
