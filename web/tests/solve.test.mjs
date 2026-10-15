// POST /v1/solve: SSE passthrough, body shapes, entitlement, validation, limits, upstream
// failures. Needs the mock Anthropic server (MOCK_URL) wired as the Worker's ANTHROPIC_BASE_URL.
import { test, before } from 'node:test';
import assert from 'node:assert/strict';
import {
  api, solve, solveBody, legacyBody, jpeg, png, register, linkedDevice, devUser, now, sleep,
  assertError, mockCount, mockLast, waitMockCount, EXPECT_MODEL, EXPECT_MAX_TOKENS, PUBLIC_HOST, PAIR_RE, MOCK_URL,
} from './lib.mjs';
import { assertSchema } from './contract.mjs';
import { MOCK_ANSWER } from './mock-anthropic.mjs';

before(async () => {
  const r = await fetch(MOCK_URL + '/healthz').catch(() => null);
  assert.ok(r?.ok, `mock Anthropic not reachable at ${MOCK_URL} (start it with "npm run mock" or use "npm run test:full")`);
});

/** Run fn (which should be rejected by the Worker) and assert the mock saw no new call. */
async function noUpstream(fn) {
  const before = await mockCount();
  const out = await fn();
  await sleep(100);
  assert.equal(await mockCount(), before, 'a rejected solve must not call Anthropic');
  return out;
}

/** Concatenate the text deltas of an Anthropic SSE body. */
function sseText(body) {
  let text = '';
  for (const block of body.split(/\r?\n\r?\n/)) {
    const data = block.split(/\r?\n/).filter((l) => l.startsWith('data:')).map((l) => l.slice(5).trim()).join('');
    if (!data) continue;
    try {
      const j = JSON.parse(data);
      if (j.type === 'content_block_delta' && j.delta?.type === 'text_delta') text += j.delta.text;
    } catch { /* ignore */ }
  }
  return text;
}

let shared; // one linked device for the simple checks
before(async () => {
  shared = await linkedDevice();
});

test('happy path (new body): SSE passed through, server owns model/system/max_tokens', async () => {
  const { dev } = await linkedDevice();
  const n0 = await mockCount();
  const r = await solve(dev.token, solveBody('OK', { tutor: true }), { headers: { 'x-device-id': dev.hw_id, 'x-firmware': 'test-1.0' } });
  assert.equal(r.status, 200, r.text.slice(0, 300));
  assert.match(r.headers.get('content-type'), /^text\/event-stream/);
  assert.match(r.text, /event: message_start/);
  assert.match(r.text, /event: message_delta/);
  assert.match(r.text, /event: message_stop/);
  assert.deepEqual(JSON.parse(sseText(r.text)), MOCK_ANSWER, 'answer JSON arrives intact');
  assert.equal(await mockCount(), n0 + 1);

  const last = await mockLast();
  assert.equal(last.headers.has_api_key, true, 'Worker sends x-api-key upstream');
  assert.ok(last.headers['anthropic-version'], 'Worker sends anthropic-version');
  assert.equal(last.body.model, EXPECT_MODEL);
  assert.equal(last.body.max_tokens, EXPECT_MAX_TOKENS);
  assert.equal(last.body.stream, true);
  assert.ok(last.body.system && JSON.stringify(last.body.system).length > 50, 'server system prompt present');
  const imgs = last.body.messages.flatMap((m) => m.content).filter((c) => c.type === 'image');
  assert.equal(imgs.length, 1);
  assert.equal(imgs[0].source.media_type, 'image/jpeg');
});

test('PNG and two images are accepted', async () => {
  const { dev } = await linkedDevice();
  const r = await solve(dev.token, { images: [{ media_type: 'image/png', data: png() }, { media_type: 'image/jpeg', data: jpeg() }] });
  assert.equal(r.status, 200, r.text.slice(0, 300));
  const last = await mockLast();
  const imgs = last.body.messages.flatMap((m) => m.content).filter((c) => c.type === 'image');
  assert.deepEqual(imgs.map((i) => i.source.media_type), ['image/png', 'image/jpeg']);
});

test('legacy Messages-shaped body works and its foreign fields are NOT forwarded', async () => {
  const { dev } = await linkedDevice();
  const body = legacyBody();
  const r = await solve(dev.token, body);
  assert.equal(r.status, 200, r.text.slice(0, 300));
  assert.match(r.text, /event: message_stop/);
  const last = await mockLast();
  const sent = JSON.stringify(last.body);
  assert.equal(last.body.model, EXPECT_MODEL, 'model is the server\'s');
  assert.equal(last.body.max_tokens, EXPECT_MAX_TOKENS, 'max_tokens is the server\'s');
  assert.ok(!sent.includes('LEGACY_SYSTEM_MARKER'), 'legacy system prompt not forwarded');
  assert.ok(!sent.includes('LEGACY_TEXT_MARKER'), 'legacy text parts not forwarded');
  assert.ok(!sent.includes('LEGACY_TOOL_MARKER'), 'legacy tools not forwarded');
  assert.ok(!sent.includes('pwned'), 'legacy output_config not forwarded');
  assert.ok(sent.includes(body.messages[0].content[0].source.data), 'the image itself is forwarded unchanged');
});

test('not linked -> 402 device_not_linked with link URL and code, no upstream call', async () => {
  const dev = await register();
  const r = await noUpstream(() => solve(dev.token, solveBody()));
  const msg = assertError(r, 402, 'device_not_linked');
  assert.ok(msg.includes('/link'), msg);
  assert.match(msg, /[A-HJ-NP-Z2-9]{4}-[A-HJ-NP-Z2-9]{4}/, 'message carries a code');
  assert.match(msg.match(/[A-HJ-NP-Z2-9]{4}-[A-HJ-NP-Z2-9]{4}/)[0], PAIR_RE);
});

test('trial over -> 402 subscription_inactive with the exact D2 message; status agrees', async () => {
  const { dev, user } = await linkedDevice({ trial: true });
  await devUser({ email: user.email, trial_ends_at: now() - 60 });
  const r = await noUpstream(() => solve(dev.token, solveBody()));
  const msg = assertError(r, 402, 'subscription_inactive');
  assert.ok(msg.includes('free month is over'), msg);
  assert.ok(msg.includes('/account'), msg);
  assert.ok(msg.length <= 110);
  assert.equal(msg, `Your free month is over. Subscribe at ${PUBLIC_HOST}/account to keep using AI.`);

  const s = await api('GET', '/v1/device/status', { token: dev.token });
  assert.equal(s.json.linked, true);
  assert.equal(s.json.plan, 'inactive');
  assert.equal(s.json.can_solve, false);
  assert.equal(s.json.reason, msg, 'status reason matches the solve message');
});

test('plan inactive (subscription ended) -> 402 subscription_inactive, renew message', async () => {
  const { dev, user } = await linkedDevice();
  // Assumed convention: plan_override takes a PlanState; 'inactive' with an expired trial
  // models "subscription ended".
  await devUser({ email: user.email, trial_ends_at: now() - 86400 * 40, plan_override: 'inactive' });
  const r = await noUpstream(() => solve(dev.token, solveBody()));
  const msg = assertError(r, 402, 'subscription_inactive');
  assert.ok(msg.includes('/account'), msg);
});

test('trial (enrolled board, never expired) solves without any override', async () => {
  const { dev } = await linkedDevice({ trial: true });
  const r = await solve(dev.token, solveBody());
  assert.equal(r.status, 200, r.text.slice(0, 300));
});

// The 6/min burst limit runs before the body is read (security review S6), so each
// rejected-body case uses its own calculator.
const freshToken = async () => (await linkedDevice()).dev.token;

test('bad bodies -> 400 bad_request, no upstream call', async () => {
  const bad = [
    { images: [] },
    { images: [1, 2, 3].map(() => ({ media_type: 'image/jpeg', data: jpeg() })) },
    { images: [{ media_type: 'image/gif', data: jpeg() }] },
    { images: [{ media_type: 'image/jpeg', data: '***not base64***' }] },
    { images: [{ media_type: 'image/jpeg' }] },
    { tutor: true },
    { messages: [{ role: 'user', content: [{ type: 'text', text: 'no image' }] }] },
  ];
  for (const body of bad) {
    const token = await freshToken();
    const r = await noUpstream(() => solve(token, body));
    assertError(r, 400, 'bad_request');
  }
  const token = await freshToken();
  const r = await noUpstream(() => api('POST', '/v1/solve', { token, rawBody: '{"images":', contentType: 'application/json' }));
  assertError(r, 400, 'bad_request');
});

test('more than 5 MiB decoded in total is rejected (400), no upstream call', async () => {
  // 2 x 3.1 MB = 6.2 MB decoded (over 5 MiB) but ~8.27 MB of base64 (under the 8 MiB body
  // limit); 413 is still accepted in case JSON overhead tips it over.
  const token = await freshToken();
  const part = Buffer.alloc(3_100_000, 7);
  part[0] = 0xff; part[1] = 0xd8; part[2] = 0xff;
  const data = part.toString('base64');
  const body = { images: [{ media_type: 'image/jpeg', data }, { media_type: 'image/jpeg', data }] };
  const r = await noUpstream(() => solve(token, body));
  assert.ok([400, 413].includes(r.status), `expected 400 or 413, got ${r.status}`);
  assertError(r, r.status, r.status === 400 ? 'bad_request' : 'payload_too_large');
});

test('raw body over 8 MiB -> 413 payload_too_large, no upstream call', async () => {
  const token = await freshToken();
  const huge = JSON.stringify({ images: [{ media_type: 'image/jpeg', data: 'A'.repeat(8 * 1024 * 1024 + 1024) }] });
  const r = await noUpstream(() => api('POST', '/v1/solve', { token, rawBody: huge }));
  assertError(r, 413, 'payload_too_large');
});

test('Anthropic 500 before streaming -> 502 upstream_error', async () => {
  const { dev } = await linkedDevice();
  const n0 = await mockCount();
  const r = await solve(dev.token, solveBody('FAIL500'));
  assertError(r, 502, 'upstream_error');
  assert.equal(await mockCount(), n0 + 1, 'exactly one upstream attempt (no retry storm)');
});

test('mid-stream failure ends the stream with event: error carrying an Error payload', async () => {
  const { dev } = await linkedDevice();
  const r = await solve(dev.token, solveBody('MIDFAIL'));
  assert.equal(r.status, 200);
  assert.match(r.text, /event: message_start/);
  assert.ok(!/event: message_stop/.test(r.text), 'no message_stop after a broken upstream');
  const blocks = r.text.split(/\r?\n\r?\n/).filter(Boolean);
  const last = blocks[blocks.length - 1];
  assert.match(last, /^event: error/m, `last SSE event must be "error", got: ${last.slice(0, 200)}`);
  const data = last.split(/\r?\n/).filter((l) => l.startsWith('data:')).map((l) => l.slice(5).trim()).join('');
  assertSchema('Error', JSON.parse(data), 'event: error payload');
});

test('one solve in flight per device -> 429 rate_limited, no second upstream call', async () => {
  const { dev } = await linkedDevice();
  const n0 = await mockCount();
  const first = solve(dev.token, solveBody('SLOW'));
  await waitMockCount(n0 + 1);
  const second = await solve(dev.token, solveBody());
  assertError(second, 429, 'rate_limited');
  assert.ok(second.headers.get('retry-after'), 'Retry-After on 429');
  assert.equal(await mockCount(), n0 + 1, 'second request did not reach Anthropic');
  const r1 = await first;
  assert.equal(r1.status, 200);
  // After the first finishes, the device may solve again.
  await sleep(200);
  const third = await solve(dev.token, solveBody());
  assert.equal(third.status, 200, third.text.slice(0, 200));
});

test('burst limit: 7th solve within a minute -> 429 rate_limited, not forwarded', async () => {
  const { dev } = await linkedDevice();
  const n0 = await mockCount();
  for (let i = 0; i < 6; i++) {
    const r = await solve(dev.token, solveBody());
    assert.equal(r.status, 200, `solve ${i + 1}: ${r.status} ${r.text.slice(0, 200)}`);
    await sleep(50); // let settlement release the in-flight reservation
  }
  assert.equal(await mockCount(), n0 + 6);
  const r = await noUpstream(() => solve(dev.token, solveBody()));
  assertError(r, 429, 'rate_limited');
  assert.ok(r.headers.get('retry-after'));
});

test('usage is visible in /api/me after a solve', async () => {
  const { dev, user } = await linkedDevice();
  const r = await solve(dev.token, solveBody());
  assert.equal(r.status, 200);
  await sleep(300); // settlement runs in waitUntil
  const me = await api('GET', '/api/me', { cookie: user.cookie });
  assert.equal(me.status, 200);
  assert.ok(me.json.usage.solves_this_month >= 1, 'solve counted');
  const d = me.json.devices.find((x) => x.id === dev.device_id);
  assert.ok(d?.last_seen_at, 'last_seen_at recorded');
});
