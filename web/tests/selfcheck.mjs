// npm run selfcheck: checks the test kit itself, no Worker needed.
//   - every .mjs file parses (node --check)
//   - contract.mjs loads openapi.yaml and compiles every response/request schema
//   - the mock Anthropic server streams valid SSE and honours FAIL500 / MIDFAIL / SLOW
import { spawnSync } from 'node:child_process';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const ok = (m) => console.log('  ok  ' + m);

console.log('syntax');
for (const f of fs.readdirSync(here).filter((f) => f.endsWith('.mjs'))) {
  const r = spawnSync(process.execPath, ['--check', path.join(here, f)], { encoding: 'utf8' });
  assert.equal(r.status, 0, `${f}: ${r.stderr}`);
  ok(f);
}

console.log('contract');
const c = await import('./contract.mjs');
ok(`loaded ${c.CONTRACT_PATH}`);
ok(`compiled ${c.compileAll()} schemas`);
c.assertSchema('Error', { type: 'error', error: { type: 'rate_limited', message: 'Slow down.' } });
assert.throws(() => c.assertSchema('Error', { type: 'error', error: { type: 'nope', message: 'x' } }));
assert.throws(() => c.assertSchema('Error', { type: 'error', error: { type: 'bad_request', message: 'x'.repeat(111) } }));
ok('Error schema enforces enum and 110-char message');
const H = (o) => new Headers(o);
c.assertMatches('POST', '/v1/device/register', {
  status: 201, headers: H({ 'content-type': 'application/json' }),
  json: { device_id: 'dev_0123456789abcdef', token: 'dt_' + 'A'.repeat(43), pair_code: 'ABCD-EF23', link_url: 'http://localhost:8787/link?code=ABCD-EF23', expires_at: 1 },
});
assert.throws(() => c.assertMatches('POST', '/v1/device/register', { status: 201, headers: H({ 'content-type': 'application/json' }), json: { device_id: 'x' } }));
assert.throws(() => c.assertMatches('POST', '/v1/device/register', { status: 418, headers: H({}), text: '' }), /not documented/);
assert.throws(() => c.assertMatches('POST', '/v1/solve', { status: 429, headers: H({ 'content-type': 'application/json' }), json: { type: 'error', error: { type: 'rate_limited', message: 'x' } } }), /Retry-After/);
c.assertMatches('POST', '/v1/solve', { status: 429, headers: H({ 'content-type': 'application/json', 'retry-after': '30' }), json: { type: 'error', error: { type: 'rate_limited', message: 'x' } } });
c.assertMatches('POST', '/v1/solve', { status: 200, headers: H({ 'content-type': 'text/event-stream' }), text: 'event: message_stop\n\n' });
c.assertMatches('POST', '/api/devices/dev_0123456789abcdef/revoke', { status: 204, headers: H({}), text: '' });
assert.throws(() => c.assertMatches('POST', '/api/auth/confirm', { status: 204, headers: H({}), text: '' }), /Set-Cookie/);
c.assertRequestValid('POST', '/v1/solve', { images: [{ media_type: 'image/jpeg', data: '/9j/' }] });
c.assertRequestValid('POST', '/v1/solve', { messages: [{ content: [{ type: 'image' }] }], model: 'x' });
ok('assertMatches: status, schema, headers, empty bodies, $ref + oneOf');

console.log('mock');
const { startMock, MOCK_ANSWER } = await import('./mock-anthropic.mjs');
const lib = await import('./lib.mjs');
const m = await startMock({ port: 18788, slowMs: 600 });
try {
  const send = (marker, headers = { 'x-api-key': 'k', 'anthropic-version': '2023-06-01' }) => fetch(m.url + '/v1/messages', {
    method: 'POST', headers: { 'content-type': 'application/json', ...headers },
    body: JSON.stringify({ model: 'claude-sonnet-5-5', max_tokens: 16000, stream: true, system: 's', messages: [{ role: 'user', content: [{ type: 'image', source: { type: 'base64', media_type: 'image/jpeg', data: lib.jpeg(marker) } }] }] }),
  });

  let r = await send('OK');
  assert.equal(r.status, 200);
  assert.match(r.headers.get('content-type'), /^text\/event-stream/);
  const body = await r.text();
  const events = body.trim().split('\n\n').map((b) => ({ event: b.split('\n')[0].slice(7), data: JSON.parse(b.split('\n')[1].slice(6)) }));
  assert.deepEqual([...new Set(events.map((e) => e.event))], ['message_start', 'content_block_start', 'ping', 'content_block_delta', 'content_block_stop', 'message_delta', 'message_stop']);
  for (const e of events) assert.equal(e.event, e.data.type, 'event name matches data.type');
  assert.ok(events[0].data.message.usage.input_tokens > 0);
  assert.ok(events.find((e) => e.event === 'message_delta').data.usage.output_tokens > 0);
  const text = events.filter((e) => e.event === 'content_block_delta').map((e) => e.data.delta.text).join('');
  assert.deepEqual(JSON.parse(text), MOCK_ANSWER);
  ok('normal: Anthropic-format SSE, usage in message_start and message_delta, JSON answer');

  r = await send('FAIL500');
  assert.equal(r.status, 500);
  assert.equal((await r.json()).error.type, 'api_error');
  ok('FAIL500: HTTP 500 before streaming');

  r = await send('MIDFAIL');
  assert.equal(r.status, 200);
  let mid = '';
  let broke = false;
  try {
    const reader = r.body.getReader();
    for (;;) {
      const { done, value } = await reader.read();
      if (done) break;
      mid += Buffer.from(value).toString();
    }
  } catch {
    broke = true;
  }
  assert.ok(mid.includes('message_start') && !mid.includes('message_stop'));
  assert.ok(broke, 'socket destroyed mid-stream (reader errors)');
  ok('MIDFAIL: message_start then socket destroyed');

  const t0 = Date.now();
  r = await send('SLOW');
  await r.text();
  assert.ok(Date.now() - t0 >= 550, 'SLOW delays');
  ok(`SLOW: delayed ${Date.now() - t0} ms`);

  r = await send('OK', { 'anthropic-version': '2023-06-01' });
  assert.equal(r.status, 401);
  r = await send('OK', { 'x-api-key': 'k' });
  assert.equal(r.status, 400);
  ok('header validation: no x-api-key -> 401, no anthropic-version -> 400');

  const cnt = await (await fetch(m.url + '/__count')).json();
  assert.equal(cnt.count, 6);
  const last = await (await fetch(m.url + '/__last')).json();
  assert.equal(last.headers.has_api_key, true);
  assert.ok(!JSON.stringify(last).includes('"k"'), 'api key value never echoed');
  assert.equal(last.body.max_tokens, 16000);
  ok('/__count and /__last');
} finally {
  await m.close();
}
console.log('selfcheck passed');
