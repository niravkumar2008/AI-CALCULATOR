// Pretend to be a calculator against the local site (web/dev/run-local.mjs must be running).
//
//   node web/dev/calculator.mjs new       a fresh calculator: enrolled like a QA'd board,
//                                         registered, and showing its link code
//   node web/dev/calculator.mjs status    what the calculator's screen would say
//   node web/dev/calculator.mjs code      a new link code (if the old one expired)
//   node web/dev/calculator.mjs solve     "take a photo" and print the answer and steps
//   node web/dev/calculator.mjs forget    throw this calculator away
//
// Its identity is kept in <temp>/aicalc-local/calculator.json. Local only: refuses any
// address other than 127.0.0.1.

import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const BASE = 'http://127.0.0.1:8787';
const STATE = path.join(os.tmpdir(), 'aicalc-local');
const FILE = path.join(STATE, 'calculator.json');
const say = (...a) => console.log(...a);

function load() {
  try {
    return JSON.parse(fs.readFileSync(FILE, 'utf8'));
  } catch {
    return null;
  }
}
const save = (c) => fs.writeFileSync(FILE, JSON.stringify(c, null, 2));
const headers = (c) => ({
  authorization: `Bearer ${c.token}`,
  'x-firmware': 'local-dev',
  'x-device-id': c.hw_id,
  'x-dev-client-ip': '10.77.0.1',
});

async function call(method, p, { body, headers: h = {} } = {}) {
  const r = await fetch(BASE + p, {
    method,
    headers: { ...(body ? { 'content-type': 'application/json' } : {}), ...h },
    body: body ? JSON.stringify(body) : undefined,
  }).catch(() => null);
  if (!r) {
    say(`Can't reach ${BASE}. Start it first: node web/dev/run-local.mjs`);
    process.exit(1);
  }
  const text = await r.text();
  let json = null;
  try {
    json = JSON.parse(text);
  } catch {
    /* SSE or empty */
  }
  return { status: r.status, json, text };
}

function screen(lines) {
  const w = 28;
  say('  ┌' + '─'.repeat(w) + '┐');
  for (const l of lines.flatMap((x) => (String(x).match(new RegExp(`.{1,${w}}(\\s|$)|.{${w}}`, 'g')) || ['']).map((s) => s.trimEnd()))) {
    say('  │' + l.padEnd(w) + '│');
  }
  say('  └' + '─'.repeat(w) + '┘');
}

async function cmdNew() {
  const secrets = JSON.parse(fs.readFileSync(path.join(STATE, 'local-secrets.json'), 'utf8'));
  const hw_id = 'calc-' + crypto.randomBytes(6).toString('hex');
  const hw_secret = 'hs_' + crypto.randomBytes(32).toString('base64url');
  // QA step: enrol the board, so it can get the free month (security decision S1).
  const e = await call('POST', '/v1/admin/hardware', { body: { hw_id, hw_secret }, headers: { 'x-admin-token': secrets.ADMIN_TOKEN } });
  if (e.status !== 201) return say(`Enrolment failed: ${e.status} ${e.text}`);
  // First Wi-Fi connection: the calculator registers itself.
  const r = await call('POST', '/v1/device/register', { body: { hw_id, hw_secret }, headers: { 'x-firmware': 'local-dev', 'x-dev-client-ip': '10.77.0.1' } });
  if (r.status !== 201) return say(`Registration failed: ${r.status} ${r.text}`);
  save({ hw_id, hw_secret, device_id: r.json.device_id, token: r.json.token });
  say('\nA new calculator is on. Its screen shows:\n');
  screen(['LINK THIS CALCULATOR', '', `Code: ${r.json.pair_code}`, '', 'Go to', r.json.link_url]);
  say(`\nOpen ${r.json.link_url} in your browser (sign in first if asked).`);
  say('Then run: node web/dev/calculator.mjs status\n');
}

async function cmdStatus(c) {
  const r = await call('GET', '/v1/device/status', { headers: headers(c) });
  if (r.status !== 200) return screen([r.json?.error?.message ?? `Error ${r.status}`]);
  const s = r.json;
  const lines = [`Plan: ${s.plan_reason ?? s.plan}`, s.linked ? `Account: ${s.account_hint ?? ''}` : 'Not linked yet'];
  if (!s.linked && s.pair_code) lines.push(`Code: ${s.pair_code}`, `Go to ${s.link_url ?? ''}`);
  if (s.can_solve) lines.push('', 'AI Solve ready (MODE 4)', `Solves this month: ${s.solves_this_month ?? 0}/${s.solves_limit ?? '?'}`);
  else lines.push('', s.reason ?? 'AI Solve is not available.');
  if (s.trial_ends_at) lines.push(`Free month ends ${new Date(s.trial_ends_at * 1000).toDateString()}`);
  screen(lines);
}

async function cmdCode(c) {
  const r = await call('POST', '/v1/pair/start', { headers: headers(c) });
  // pair/start answers 200 with `code` (register is the one that answers 201 with `pair_code`).
  if (r.status !== 200) return screen([r.json?.error?.message ?? `Error ${r.status}`]);
  if (r.json?.already_linked) return screen(['This calculator is already linked.', '', 'Run: node web/dev/calculator.mjs status']);
  screen(['LINK THIS CALCULATOR', '', `Code: ${r.json.code}`, '', 'Go to', r.json.link_url]);
}

function photo() {
  // A tiny JPEG-shaped picture (the local fake AI doesn't look at it).
  const bytes = Buffer.concat([
    Buffer.from([0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00]),
    Buffer.from('LOCAL DEMO PHOTO'),
    Buffer.alloc(600, 0x20),
    Buffer.from([0xff, 0xd9]),
  ]);
  return bytes.toString('base64');
}

async function cmdSolve(c) {
  say('\nPressing = … (photo sent)\n');
  const r = await call('POST', '/v1/solve', { body: { images: [{ media_type: 'image/jpeg', data: photo() }] }, headers: headers(c) });
  if (r.status !== 200) return screen([r.json?.error?.message ?? `Error ${r.status}`]);
  let text = '';
  for (const line of r.text.split('\n')) {
    if (!line.startsWith('data:')) continue;
    try {
      const d = JSON.parse(line.slice(5));
      if (d.delta?.type === 'text_delta') text += d.delta.text;
    } catch {
      /* not JSON */
    }
  }
  let ans;
  try {
    ans = JSON.parse(text);
  } catch {
    return screen([text.slice(0, 200) || 'No answer.']);
  }
  screen(['ANSWER', ans.answer ?? '', '', ...(ans.steps ?? []).map((s, i) => `${i + 1}. ${s}`)]);
}

const cmd = process.argv[2] || 'status';
if (cmd === 'new') await cmdNew();
else if (cmd === 'forget') {
  fs.rmSync(FILE, { force: true });
  say('This calculator was thrown away. Make another with: node web/dev/calculator.mjs new');
} else {
  const c = load();
  if (!c) say('No calculator yet. Make one with: node web/dev/calculator.mjs new');
  else if (cmd === 'status') await cmdStatus(c);
  else if (cmd === 'code') await cmdCode(c);
  else if (cmd === 'solve') await cmdSolve(c);
  else say('Commands: new, status, code, solve, forget');
}
