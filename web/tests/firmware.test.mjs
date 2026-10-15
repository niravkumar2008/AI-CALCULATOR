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
  assert.deepEqual(older.json, { update: true, version, size: img.length, sha256: sha(img), path: '/v1/firmware/image' });

  const same = await api('GET', '/v1/firmware', { token: d.token, headers: { 'x-firmware': version } });
  assert.equal(same.json.update, false);

  const dl = await api('GET', '/v1/firmware/image', { token: d.token });
  assert.equal(dl.status, 200);
  assert.match(dl.headers.get('content-type'), /^application\/octet-stream/);
  assert.equal(dl.bytes.length, img.length);
  assert.equal(sha(dl.bytes), sha(img));
});
