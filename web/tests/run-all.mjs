// npm run test:full
//   1. starts the mock Anthropic server (127.0.0.1:8788)
//   2. applies the D1 migrations to a throw-away local state dir (fresh DB and KV every run)
//   3. starts `npm run dev` in ../api with settings passed as `--var KEY:VALUE` flags
//      (wrangler splits on the first ':' only, and --var overrides wrangler.toml vars and
//      .dev.vars, so your own .dev.vars is never read for these keys and never touched)
//   4. waits for /healthz, runs every *.test.mjs, shuts everything down (Windows too)
//
// Env knobs: KEEP_STATE=1 (reuse ../api/.wrangler/state and `npm run db:migrate:local`
// instead of a temp dir), API_DIR, PORT (8787), MOCK_PORT (8788), TEST_FILES (space separated).
import { spawn, spawnSync } from 'node:child_process';
import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startMock } from './mock-anthropic.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const API_DIR = path.resolve(here, process.env.API_DIR || '../api');
const PORT = Number(process.env.PORT || 8787);
const MOCK_PORT = Number(process.env.MOCK_PORT || 8788);
const BASE_URL = `http://localhost:${PORT}`;
const KEEP_STATE = process.env.KEEP_STATE === '1';
const isWin = process.platform === 'win32';
const FILES = (process.env.TEST_FILES || 'device.test.mjs solve.test.mjs auth.test.mjs account.test.mjs firmware.test.mjs dev.test.mjs stage3.todo.test.mjs').split(/\s+/).filter(Boolean);

const ADMIN_TOKEN = crypto.randomBytes(24).toString('hex');
const VARS = {
  DEV_MODE: '1',
  ANTHROPIC_BASE_URL: `http://127.0.0.1:${MOCK_PORT}`,
  ANTHROPIC_API_KEY: 'sk-ant-test-mock-not-a-real-key',
  MODEL: process.env.EXPECT_MODEL || 'claude-sonnet-5-5',
  MAX_TOKENS: process.env.EXPECT_MAX_TOKENS || '16000',
  PUBLIC_URL: BASE_URL,
  TRIAL_DAYS: '30',
  MONTHLY_SOLVES: '300',
  MONTHLY_SPEND_USD: '8',
  GLOBAL_DAILY_CAP_USD: '50',
  ADMIN_TOKEN,
  RESEND_API_KEY: '', // never send real e-mail from a test run, whatever .dev.vars says
};

const log = (...a) => console.log('[test:full]', ...a);
const quote = (s) => (isWin ? `"${String(s).replace(/"/g, '""')}"` : `'${String(s).replace(/'/g, `'\\''`)}'`);

function killTree(child) {
  if (!child || child.exitCode !== null || child.killed) return;
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

let mock, dev, stateDir, devLog;
let cleaning = false;
async function cleanup(code) {
  if (cleaning) return;
  cleaning = true;
  killTree(dev);
  if (mock) await mock.close().catch(() => {});
  if (stateDir) {
    await new Promise((r) => setTimeout(r, 500)); // let workerd release its files on Windows
    try { fs.rmSync(stateDir, { recursive: true, force: true }); } catch { log(`could not remove ${stateDir}; delete it by hand`); }
  }
  process.exit(code);
}
process.on('SIGINT', () => cleanup(130));
process.on('SIGTERM', () => cleanup(143));

async function main() {
  if (!fs.existsSync(path.join(API_DIR, 'package.json'))) {
    log(`no Worker found at ${API_DIR} (web/api). Nothing to test.`);
    return cleanup(2);
  }
  if (!fs.existsSync(path.join(API_DIR, 'node_modules'))) {
    log(`run "npm install" in ${API_DIR} first.`);
    return cleanup(2);
  }
  if (await up(`${BASE_URL}/healthz`)) {
    log(`something is already answering on ${BASE_URL}. Stop it, or use "npm test" to test that server.`);
    return cleanup(2);
  }
  if (fs.existsSync(path.join(API_DIR, '.dev.vars'))) {
    log('note: ../api/.dev.vars exists; it is left alone and the --var flags below take precedence.');
  }

  mock = await startMock({ port: MOCK_PORT });
  log(`mock Anthropic on ${mock.url}`);

  const devArgs = [];
  if (KEEP_STATE) {
    log('applying migrations (npm run db:migrate:local)');
    const m = spawnSync(`npm run db:migrate:local`, { cwd: API_DIR, shell: true, encoding: 'utf8', env: { ...process.env, CI: '1' } });
    if (m.status !== 0) {
      log('migrations failed:\n' + m.stdout + m.stderr);
      return cleanup(1);
    }
  } else {
    stateDir = fs.mkdtempSync(path.join(os.tmpdir(), 'aicalc-test-state-'));
    log(`fresh local state in ${stateDir}`);
    const cmd = ['npx', 'wrangler', 'd1', 'migrations', 'apply', 'DB', '--local', '--persist-to', quote(stateDir)].join(' ');
    const m = spawnSync(cmd, { cwd: API_DIR, shell: true, encoding: 'utf8', env: { ...process.env, CI: '1' } });
    if (m.status !== 0) {
      log(`migrations failed (${cmd}):\n` + m.stdout + m.stderr);
      return cleanup(1);
    }
    devArgs.push('--persist-to', quote(stateDir));
  }
  for (const [k, v] of Object.entries(VARS)) devArgs.push('--var', quote(`${k}:${v}`));

  const devCmd = `npm run dev -- ${devArgs.join(' ')}`;
  log(`starting: npm run dev -- --persist-to <tmp> --var KEY:VALUE x${Object.keys(VARS).length} (in ${API_DIR})`);
  devLog = path.join(os.tmpdir(), `aicalc-wrangler-${Date.now()}.log`);
  const out = fs.openSync(devLog, 'w');
  dev = spawn(devCmd, { cwd: API_DIR, shell: true, detached: !isWin, stdio: ['ignore', out, out], env: { ...process.env, CI: '1', WRANGLER_SEND_METRICS: 'false' } });
  dev.on('exit', (code) => {
    if (!cleaning) log(`wrangler dev exited early (code ${code}); log: ${devLog}`);
  });

  const deadline = Date.now() + 90_000;
  while (!(await up(`${BASE_URL}/healthz`))) {
    if (Date.now() > deadline || dev.exitCode !== null) {
      log(`Worker did not come up on ${BASE_URL}/healthz. Last log lines:\n` + fs.readFileSync(devLog, 'utf8').split('\n').slice(-30).join('\n'));
      return cleanup(1);
    }
    await new Promise((r) => setTimeout(r, 500));
  }
  log('Worker is up; running tests');

  const t = spawn(process.execPath, ['--test', '--test-concurrency=1', '--test-timeout=120000', ...FILES], {
    cwd: here,
    stdio: 'inherit',
    env: {
      ...process.env,
      BASE_URL,
      MOCK_URL: mock.url,
      PUBLIC_URL: BASE_URL,
      ADMIN_TOKEN,
      FRESH_STATE: KEEP_STATE ? '0' : '1',
      EXPECT_MODEL: VARS.MODEL,
      EXPECT_MAX_TOKENS: VARS.MAX_TOKENS,
      EXPECT_MONTHLY_SOLVES: VARS.MONTHLY_SOLVES,
    },
  });
  const code = await new Promise((r) => t.on('exit', (c) => r(c ?? 1)));
  log(`tests finished with code ${code}; wrangler log: ${devLog}`);
  return cleanup(code);
}

main().catch((e) => {
  console.error(e);
  cleanup(1);
});
