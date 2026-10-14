// Firmware OTA on Workers KV: /v1/firmware, /v1/firmware/image, admin publish.
import { test } from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import { api, assertError, register, ADMIN_TOKEN, FRESH_STATE } from './lib.mjs';

const sha = (b) => crypto.createHash('sha256').update(b).digest('hex');
/** Image the publish endpoint accepts: >= 100 KB, <= 0x1E0000, first byte 0xE9. */
const image = (size = 150 * 1024) => {
  const b = crypto.randomBytes(size);
  b[0] = 0xe9;
  return b;
};
const publish = (version, body, token = ADMIN_TOKEN) =>
  api('POST', `/v1/admin/firmware?version=${encodeURIComponent(version)}`, {
    rawBody: body,
    contentType: 'application/octet-stream',
    headers: token === null ? {} : { 'x-admin-token': token },
  });

const noAdmin = ADMIN_TOKEN ? false : 'ADMIN_TOKEN not given to the test run (npm run test:full sets one)';

test('nothing published: update false and image 404 not_found', { skip: FRESH_STATE ? false : 'needs a fresh KV (FRESH_STATE=1, set by test:full)' }, async () => {
  const d = await register();
  const r = await api('GET', '/v1/firmware', { token: d.token, headers: { 'x-firmware': '0.0.1' } });
  assert.equal(r.status, 200);
  assert.equal(r.json.update, false);
  assertError(await api('GET', '/v1/firmware/image', { token: d.token }), 404, 'not_found');
});

test('firmware endpoints need a device token', async () => {
  assertError(await api('GET', '/v1/firmware'), 401, 'unauthorized');
  assertError(await api('GET', '/v1/firmware/image'), 401, 'unauthorized');
});

test('admin publish: missing or wrong token -> 401 unauthorized, nothing published', { skip: noAdmin }, async () => {
  assertError(await publish('evil-1', image(), null), 401, 'unauthorized');
  assertError(await publish('evil-2', image(), 'wrong-' + ADMIN_TOKEN), 401, 'unauthorized');
  const d = await register();
  const r = await api('GET', '/v1/firmware', { token: d.token, headers: { 'x-firmware': 'x' } });
  assert.ok(!['evil-1', 'evil-2'].includes(r.json.version), 'rejected images were not published');
});

test('admin publish: bad magic, too small, too big, bad version -> 400', { skip: noAdmin }, async () => {
  const bad = image();
  bad[0] = 0x00;
  assertError(await publish('bad-magic', bad), 400, 'bad_request');
  assertError(await publish('too-small', image(50 * 1024)), 400, 'bad_request');
  assertError(await publish('too-big', image(0x1e0000 + 1)), 400, 'bad_request');
  assertError(await publish('bad version!', image()), 400, 'bad_request');
});

test('publish, then update true when x-firmware differs, false when equal; image sha256 matches', { skip: noAdmin }, async () => {
  const version = 't' + Date.now().toString(36);
  const img = image(200 * 1024 + 17);
  const p = await publish(version, img);
  assert.equal(p.status, 201, p.text);
  assert.equal(p.json.version, version);
  assert.equal(p.json.size, img.length);
  assert.equal(p.json.sha256, sha(img), 'server computes sha256 of the raw image');

  const d = await register();
  const older = await api('GET', '/v1/firmware', { token: d.token, headers: { 'x-firmware': 'old-1' } });
  assert.equal(older.status, 200);
  assert.deepEqual(older.json, {
    update: true, version, size: img.length, sha256: sha(img), path: '/v1/firmware/image', family: null, rollback: false,
  });

  const same = await api('GET', '/v1/firmware', { token: d.token, headers: { 'x-firmware': version } });
  assert.equal(same.json.update, false);

  const dl = await api('GET', '/v1/firmware/image', { token: d.token });
  assert.equal(dl.status, 200);
  assert.match(dl.headers.get('content-type'), /^application\/octet-stream/);
  assert.equal(dl.bytes.length, img.length);
  assert.equal(sha(dl.bytes), sha(img));
});

// ---- 2026-10-11: per-family channels and the rollback mark
const fam = () => 'f' + crypto.randomBytes(5).toString('hex'); // a family no other test uses
const publishTo = (q, body = image()) =>
  api('POST', `/v1/admin/firmware?${q}`, {
    rawBody: body,
    contentType: 'application/octet-stream',
    headers: { 'x-admin-token': ADMIN_TOKEN },
  });

test('family: nothing published for it -> update false (never the legacy image)', { skip: noAdmin }, async () => {
  assert.equal((await publish('legacy-' + Date.now().toString(36), image())).status, 201);
  const d = await register();
  const f = fam();
  const r = await api('GET', `/v1/firmware?family=${f}`, { token: d.token, headers: { 'x-firmware': `${f}-1` } });
  assert.equal(r.status, 200);
  assert.deepEqual(r.json, { update: false, family: f });
  assertError(await api('GET', `/v1/firmware/image?family=${f}`, { token: d.token }), 404, 'not_found');
});

test('family: publish, check and download its own image; other families and legacy unaffected', { skip: noAdmin }, async () => {
  const f = fam();
  const legacyVersion = 'legacy2-' + Date.now().toString(36);
  assert.equal((await publish(legacyVersion, image(120 * 1024))).status, 201);

  const version = `${f}-2026.10.11`;
  const img = image(160 * 1024 + 3);
  const p = await publishTo(`version=${version}&family=${f}`, img);
  assert.equal(p.status, 201, p.text);
  assert.deepEqual(p.json, { version, size: img.length, sha256: sha(img), family: f, rollback: false });

  const d = await register();
  const chk = await api('GET', `/v1/firmware?family=${f}`, { token: d.token, headers: { 'x-firmware': `${f}-2026.10.10` } });
  assert.deepEqual(chk.json, {
    update: true, version, size: img.length, sha256: sha(img), path: `/v1/firmware/image?family=${f}`, family: f, rollback: false,
  });
  const dl = await api('GET', chk.json.path, { token: d.token });
  assert.equal(dl.status, 200);
  assert.equal(sha(dl.bytes), sha(img));

  const same = await api('GET', `/v1/firmware?family=${f}`, { token: d.token, headers: { 'x-firmware': version } });
  assert.equal(same.json.update, false);

  // no family: still the legacy channel
  const leg = await api('GET', '/v1/firmware', { token: d.token, headers: { 'x-firmware': 'x' } });
  assert.equal(leg.json.version, legacyVersion);
  assert.equal(leg.json.family, null);
  assert.equal(leg.json.path, '/v1/firmware/image');
  // another family: nothing
  const other = await api('GET', `/v1/firmware?family=${fam()}`, { token: d.token });
  assert.equal(other.json.update, false);
});

test('family: malformed -> 400; publish needs version "<family>-..."', { skip: noAdmin }, async () => {
  const d = await register();
  for (const f of ['v15-lcd', 'a b', 'x'.repeat(25)]) {
    assertError(await api('GET', `/v1/firmware?family=${encodeURIComponent(f)}`, { token: d.token }), 400, 'bad_request');
    assertError(await api('GET', `/v1/firmware/image?family=${encodeURIComponent(f)}`, { token: d.token }), 400, 'bad_request');
  }
  const f = fam();
  assertError(await publishTo(`version=other-1&family=${f}`), 400, 'bad_request');
  assertError(await publishTo(`version=${f}-1&family=bad-family`), 400, 'bad_request');
  assertError(await publishTo(`version=${f}-1&family=${f}&rollback=maybe`), 400, 'bad_request');
});

test('rollback: manifest carries rollback true until the next normal publish', { skip: noAdmin }, async () => {
  const f = fam();
  const rb = await publishTo(`version=${f}-2026.10.09&family=${f}&rollback=1`);
  assert.equal(rb.status, 201, rb.text);
  assert.equal(rb.json.rollback, true);
  const d = await register();
  const h = { 'x-firmware': `${f}-2026.10.11` };
  let chk = await api('GET', `/v1/firmware?family=${f}`, { token: d.token, headers: h });
  assert.equal(chk.json.update, true);
  assert.equal(chk.json.version, `${f}-2026.10.09`);
  assert.equal(chk.json.rollback, true);

  assert.equal((await publishTo(`version=${f}-2026.10.12&family=${f}`)).status, 201);
  chk = await api('GET', `/v1/firmware?family=${f}`, { token: d.token, headers: h });
  assert.equal(chk.json.version, `${f}-2026.10.12`);
  assert.equal(chk.json.rollback, false);
});
