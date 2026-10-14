// npm run test:full
//   1. starts the mock Anthropic server (127.0.0.1:8788)
//   2. for each pass: applies the D1 migrations to a throw-away local state dir (fresh DB
//      and KV), starts `npm run dev -- --ip 127.0.0.1 --persist-to <tmp> --var KEY:VALUE ...`
//      in ../api, waits for http://127.0.0.1:8787/healthz, runs that pass's test files,
//      then kills the wrangler process tree (taskkill /T /F on Windows)
//
// Passes:
//   main   all *.test.mjs with the normal limits
//   quota  limits.test.mjs with MONTHLY_SOLVES=2 and MAX_NEW_TRIALS_PER_DAY=1
//   cap    limits.test.mjs with GLOBAL_DAILY_CAP_USD=0.01 (below one worst-case solve)
//
// 127.0.0.1, not localhost: on Windows Node may resolve localhost to ::1 while workerd
// listens on 127.0.0.1 only. PUBLIC_URL is the same origin, so Origin checks and the
// messages that embed the host all use 127.0.0.1:8787.
//
// --var KEY:VALUE: wrangler splits on the first ':' only and --var overrides wrangler.toml
// vars and .dev.vars, so ../api/.dev.vars is never read for these keys and never touched.
//
// Env knobs: PASSES="main quota cap", TEST_FILES (main pass only), KEEP_STATE=1 (reuse
// ../api/.wrangler/state and `npm run db:migrate:local`), API_DIR, PORT, MOCK_PORT.
import { spawn, spawnSync } from 'node:child_process';
import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startMock } from './mock-anthropic.mjs';
import { MOCK_STRIPE_KEY, startStripeMock } from './mock-stripe.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const API_DIR = path.resolve(here, process.env.API_DIR || '../api');
const HOST = '127.0.0.1';
const PORT = Number(process.env.PORT || 8787);
const MOCK_PORT = Number(process.env.MOCK_PORT || 8788);
const STRIPE_MOCK_PORT = Number(process.env.STRIPE_MOCK_PORT || 8789);
const BASE_URL = `http://${HOST}:${PORT}`;
const KEEP_STATE = process.env.KEEP_STATE === '1';
const isWin = process.platform === 'win32';
const MAIN_FILES = (process.env.TEST_FILES || 'device.test.mjs solve.test.mjs auth.test.mjs account.test.mjs enrolment.test.mjs firmware.test.mjs dev.test.mjs stage3.test.mjs').split(/\s+/).filter(Boolean);

const ADMIN_TOKEN = crypto.randomBytes(24).toString('hex'); // 48 chars (S10: must be >= 32)
const STRIPE_WEBHOOK_SECRET = 'whsec_' + crypto.randomBytes(24).toString('base64url');
const BASE_VARS = {
  DEV_MODE: '1',
  ANTHROPIC_BASE_URL: `http://127.0.0.1:${MOCK_PORT}`,
  ANTHROPIC_API_KEY: 'sk-ant-test-mock-not-a-real-key',
  MODEL: process.env.EXPECT_MODEL || 'claude-sonnet-5-5',
  MAX_TOKENS: process.env.EXPECT_MAX_TOKENS || '16000',
  PUBLIC_URL: BASE_URL,
  TRIAL_DAYS: '30',
  MONTHLY_SOLVES: '300',
  MONTHLY_SPEND_USD: '5',
  GLOBAL_DAILY_CAP_USD: '50',
  MAX_NEW_TRIALS_PER_DAY: '100000', // the suite starts far more than 20 trials
  MAIL_DAILY_CAP: '100000', // and sends far more than 80 sign-in mails (outbox only, no Resend)
  ADMIN_TOKEN,
  RESEND_API_KEY: '', // never send real e-mail from a test run, whatever .dev.vars says
  // Stage 3: Stripe goes to the local mock only (mock-stripe.mjs); never a real key.
  STRIPE_SECRET_KEY: MOCK_STRIPE_KEY,
  STRIPE_WEBHOOK_SECRET,
  STRIPE_PRICE_SUB: 'price_mockMonthly15',
  STRIPE_PRICE_SUB_YEARLY: 'price_mockYearly100',
  STRIPE_PRICE_DEVICE: 'price_mockDevice225',
  STRIPE_API_BASE: `http://127.0.0.1:${STRIPE_MOCK_PORT}`,
  STORE_OPEN: '1',
};

const PASSES = {
  main: { files: MAIN_FILES, vars: {} },
  quota: { files: ['limits.test.mjs'], vars: { MONTHLY_SOLVES: '2', MAX_NEW_TRIALS_PER_DAY: '1' } },
  cap: { files: ['limits.test.mjs'], vars: { GLOBAL_DAILY_CAP_USD: '0.01' } },
  closed: { files: ['stage3.test.mjs'], vars: { STORE_OPEN: '0' } },
};
const RUN = (process.env.PASSES || 'main quota cap closed').split(/\s+/).filter(Boolean);

const log = (...a) => console.log('[test:full]', ...a);
const quote = (s) => (isWin ? `"${String(s).replace(/"/g, '""')}"` : `'${String(s).replace(/'/g, `'\\''`)}'`);

function killTree(child) {
  if (!child || child.exitCode !== null) return;
  try {
    if (isWin) spawnSync('taskkill', ['/pid', String(child.pid), '/T', '/F'], { stdio: 'ignore' });
    else process.kill(-child.pid, 'SIGTERM');
  } catch { /* already gone */ }
}

async function up(url) {
  try {
    const r = await fetch(url, { signal: AbortSignal.timeout(1500) });
    return r.ok;
  } catch {
    return false;
  }
}

async function waitDown(url, ms = 15000) {
  const end = Date.now() + ms;
  while (Date.now() < end && (await up(url))) await new Promise((r) => setTimeout(r, 300));
}

let mock;
let stripeMock;
let dev = null;
let stateDir = null;
let cleaning = false;

function removeState() {
  if (!stateDir) return;
  try { fs.rmSync(stateDir, { recursive: true, force: true }); } catch { log(`could not remove ${stateDir}; delete it by hand`); }
  stateDir = null;
}

async function stopWorker() {
  killTree(dev);
  dev = null;
  await waitDown(`${BASE_URL}/healthz`);
  await new Promise((r) => setTimeout(r, 500)); // let workerd release its files on Windows
  removeState();
}

async function cleanup(code) {
  if (cleaning) return;
  cleaning = true;
  await stopWorker();
  if (mock) await mock.close().catch(() => {});
  if (stripeMock) await stripeMock.close().catch(() => {});
  process.exit(code);
}
process.on('SIGINT', () => cleanup(130));
process.on('SIGTERM', () => cleanup(143));

async function startWorker(name, vars) {
  const devArgs = ['--ip', HOST];
  if (KEEP_STATE) {
    const m = spawnSync('npm run db:migrate:local', { cwd: API_DIR, shell: true, encoding: 'utf8', env: { ...process.env, CI: '1' } });
    if (m.status !== 0) throw new Error('migrations failed:\n' + m.stdout + m.stderr);
  } else {
    stateDir = fs.mkdtempSync(path.join(os.tmpdir(), `aicalc-test-${name}-`));
    const cmd = ['npx', 'wrangler', 'd1', 'migrations', 'apply', 'DB', '--local', '--persist-to', quote(stateDir)].join(' ');
    const m = spawnSync(cmd, { cwd: API_DIR, shell: true, encoding: 'utf8', env: { ...process.env, CI: '1' } });
    if (m.status !== 0) throw new Error(`migrations failed (${cmd}):\n` + m.stdout + m.stderr);
    devArgs.push('--persist-to', quote(stateDir));
  }
  for (const [k, v] of Object.entries(vars)) devArgs.push('--var', quote(`${k}:${v}`));

  const devLog = path.join(os.tmpdir(), `aicalc-wrangler-${name}-${Date.now()}.log`);
  const out = fs.openSync(devLog, 'w');
  log(`[${name}] starting wrangler dev on ${BASE_URL} (log ${devLog})`);
  // `npm run dev` hard-codes --port 8787; for another PORT call wrangler with the same flags.
  const cmd = PORT === 8787 ? `npm run dev -- ${devArgs.join(' ')}` : `npx wrangler dev --local --port ${PORT} ${devArgs.join(' ')}`;
  dev = spawn(cmd, {
    cwd: API_DIR, shell: true, detached: !isWin, stdio: ['ignore', out, out],
    env: { ...process.env, CI: '1', WRANGLER_SEND_METRICS: 'false' },
  });
  let exited = false;
  dev.on('exit', () => { exited = true; });

  const deadline = Date.now() + 90_000;
  while (!(await up(`${BASE_URL}/healthz`))) {
    if (Date.now() > deadline || exited) {
      const tail = fs.readFileSync(devLog, 'utf8').split('\n').slice(-30).join('\n');
      throw new Error(`Worker did not come up on ${BASE_URL}/healthz${exited ? ' (wrangler exited)' : ''}. Last log lines:\n${tail}`);
    }
    await new Promise((r) => setTimeout(r, 500));
  }
  return devLog;
}

function runTests(name, files, vars) {
  const t = spawn(process.execPath, ['--test', '--test-concurrency=1', '--test-timeout=120000', ...files], {
    cwd: here,
    stdio: 'inherit',
    env: {
      ...process.env,
      BASE_URL,
      MOCK_URL: mock.url,
      PUBLIC_URL: BASE_URL,
      ADMIN_TOKEN,
      FRESH_STATE: KEEP_STATE ? '0' : '1',
      LIMITS_PASS: name === 'main' ? '' : name,
      EXPECT_MODEL: vars.MODEL,
      EXPECT_MAX_TOKENS: vars.MAX_TOKENS,
      EXPECT_MONTHLY_SOLVES: vars.MONTHLY_SOLVES,
      STRIPE_MOCK_URL: stripeMock.url,
      STRIPE_WEBHOOK_SECRET,
      STAGE: '3',
      STAGE3_PASS: name,
    },
  });
  return new Promise((r) => t.on('exit', (c) => r(c ?? 1)));
}

async function main() {
  if (!fs.existsSync(path.join(API_DIR, 'package.json'))) {
    log(`no Worker found at ${API_DIR} (web/api). Nothing to test.`);
    return cleanup(2);
  }
  if (!fs.existsSync(path.join(API_DIR, 'node_modules'))) {
    log(`run "npm install" in ${API_DIR} first.`);
    return cleanup(2);
  }
  if ((await up(`${BASE_URL}/healthz`)) || (await up(`http://localhost:${PORT}/healthz`))) {
    log(`something is already answering on port ${PORT}. Stop it, or use "npm test" to test that server.`);
    return cleanup(2);
  }
  if (fs.existsSync(path.join(API_DIR, '.dev.vars'))) {
    log('note: ../api/.dev.vars exists; it is left alone and the --var flags take precedence.');
  }
  mock = await startMock({ port: MOCK_PORT });
  log(`mock Anthropic on ${mock.url}`);
  stripeMock = await startStripeMock({ port: STRIPE_MOCK_PORT });
  log(`mock Stripe on ${stripeMock.url}`);

  const results = [];
  for (const name of RUN) {
    const pass = PASSES[name];
    if (!pass) throw new Error(`unknown pass ${name}`);
    const vars = { ...BASE_VARS, ...pass.vars };
    const devLog = await startWorker(name, vars);
    log(`[${name}] Worker is up; running ${pass.files.join(' ')}`);
    const code = await runTests(name, pass.files, vars);
    log(`[${name}] finished with code ${code}; wrangler log: ${devLog}`);
    results.push([name, code]);
    await stopWorker();
  }
  log('summary: ' + results.map(([n, c]) => `${n}=${c === 0 ? 'pass' : 'FAIL'}`).join(' '));
  return cleanup(results.every(([, c]) => c === 0) ? 0 : 1);
}

main().catch((e) => {
  console.error(e.message || e);
  cleanup(1);
});
