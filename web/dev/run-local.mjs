// One command for the whole site on your computer, with nothing real behind it:
//
//   cd web/api && npm install        (once)
//   node web/dev/run-local.mjs       (from the repo root; Ctrl+C to stop)
//
// Starts:
//   http://127.0.0.1:8787   the website AND its API (one Cloudflare Worker serving web/site)
//   http://127.0.0.1:8789   a fake Stripe (pretend checkout and billing pages, signed webhooks)
//   http://127.0.0.1:8788   a fake AI that answers every photo with the same worked example
// Data (accounts, calculators, orders) is kept between runs in <temp>/aicalc-local.
// `node web/dev/run-local.mjs --fresh` starts from an empty database.
//
// Sign-in e-mails aren't sent: the login page shows a "Local test only" button that opens
// the e-mail's link. To pretend to be a calculator, use web/dev/calculator.mjs.
//
// Nothing here costs money, needs an account, or talks to the internet (apart from the
// Google Fonts the pages load).

import { spawn, spawnSync } from 'node:child_process';
import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { startFakeStripe, FAKE_STRIPE_KEY } from './fake-stripe.mjs';
import { startMock } from '../tests/mock-anthropic.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const API_DIR = path.resolve(here, '../api');
const HOST = '127.0.0.1';
const PORT = 8787;
const AI_PORT = 8788;
const STRIPE_PORT = 8789;
const BASE = `http://${HOST}:${PORT}`;
// Short path on purpose: wrangler's local SQLite fails on very long Windows paths.
const STATE = path.join(os.tmpdir(), 'aicalc-local');
const isWin = process.platform === 'win32';
const log = (...a) => console.log('[local]', ...a);
const quote = (s) => (isWin ? `"${String(s).replace(/"/g, '""')}"` : `'${String(s).replace(/'/g, `'\\''`)}'`);

async function up(url) {
  try {
    return (await fetch(url, { signal: AbortSignal.timeout(1500) })).ok;
  } catch {
    return false;
  }
}

if (!fs.existsSync(path.join(API_DIR, 'node_modules'))) {
  log('First run "npm install" in web/api.');
  process.exit(2);
}
for (const p of [PORT, AI_PORT, STRIPE_PORT]) {
  if (await up(`http://${HOST}:${p}/healthz`)) {
    log(`Something is already running on port ${p}. Stop it first (another run-local, or the test suite).`);
    process.exit(2);
  }
}

if (process.argv.includes('--fresh')) fs.rmSync(STATE, { recursive: true, force: true });
fs.mkdirSync(STATE, { recursive: true });

// Per-machine local secrets (never real ones): reused between runs so links and webhooks keep working.
const secretsFile = path.join(STATE, 'local-secrets.json');
let secrets;
try {
  secrets = JSON.parse(fs.readFileSync(secretsFile, 'utf8'));
} catch {
  secrets = {
    ADMIN_TOKEN: crypto.randomBytes(24).toString('hex'),
    STRIPE_WEBHOOK_SECRET: 'whsec_' + crypto.randomBytes(24).toString('base64url'),
  };
  fs.writeFileSync(secretsFile, JSON.stringify(secrets, null, 2));
}

log('Preparing the local database…');
const mig = spawnSync(`npx wrangler d1 migrations apply DB --local --persist-to ${quote(STATE)}`, {
  cwd: API_DIR,
  shell: true,
  encoding: 'utf8',
  env: { ...process.env, CI: '1', WRANGLER_SEND_METRICS: 'false' },
});
if (mig.status !== 0) {
  log('Database setup failed:\n' + mig.stdout + mig.stderr);
  process.exit(1);
}

const ai = await startMock({ port: AI_PORT });
const stripe = await startFakeStripe({ port: STRIPE_PORT, host: HOST, workerUrl: BASE, webhookSecret: secrets.STRIPE_WEBHOOK_SECRET, priceSubYearly: 'price_localYearly100' });

const vars = {
  DEV_MODE: '1',
  PUBLIC_URL: BASE,
  ANTHROPIC_BASE_URL: ai.url,
  ANTHROPIC_API_KEY: 'sk-ant-local-fake-not-a-real-key',
  ADMIN_TOKEN: secrets.ADMIN_TOKEN,
  STRIPE_SECRET_KEY: FAKE_STRIPE_KEY,
  STRIPE_WEBHOOK_SECRET: secrets.STRIPE_WEBHOOK_SECRET,
  STRIPE_PRICE_SUB: 'price_localMonthly10',
  STRIPE_PRICE_SUB_YEARLY: 'price_localYearly100',
  STRIPE_PRICE_DEVICE: 'price_localDevice225',
  STRIPE_API_BASE: stripe.url,
  STRIPE_API_VERSION: '2026-09-30.clover', // pinned like production (review G4); the fake ignores it
  STORE_OPEN: '1', // so the whole buying flow can be tried locally (the real site starts closed)
  RESEND_API_KEY: '', // e-mails go to the local outbox only
  MAIL_DAILY_CAP: '100000',
};
// The site comes from [assets] in wrangler.toml (../site), exactly as in production.
const args = ['wrangler', 'dev', '--local', '--port', String(PORT), '--ip', HOST, '--persist-to', quote(STATE)];
for (const [k, v] of Object.entries(vars)) args.push('--var', quote(`${k}:${v}`));

log('Starting the website and API…');
const dev = spawn(`npx ${args.join(' ')}`, {
  cwd: API_DIR,
  shell: true,
  detached: !isWin,
  stdio: ['ignore', 'pipe', 'pipe'],
  env: { ...process.env, CI: '1', WRANGLER_SEND_METRICS: 'false' },
});
const devLog = fs.createWriteStream(path.join(STATE, 'wrangler.log'));
dev.stdout.pipe(devLog);
dev.stderr.pipe(devLog);

function stop(code = 0) {
  try {
    if (isWin) spawnSync('taskkill', ['/pid', String(dev.pid), '/T', '/F'], { stdio: 'ignore' });
    else process.kill(-dev.pid, 'SIGTERM');
  } catch {
    /* already gone */
  }
  Promise.allSettled([ai.close(), stripe.close()]).then(() => process.exit(code));
}
process.on('SIGINT', () => stop(0));
process.on('SIGTERM', () => stop(0));
dev.on('exit', (c) => {
  log(`The website server stopped (code ${c}). Its log: ${path.join(STATE, 'wrangler.log')}`);
  stop(1);
});

const deadline = Date.now() + 90_000;
while (!(await up(`${BASE}/healthz`))) {
  if (Date.now() > deadline) {
    log(`The website didn't start. See ${path.join(STATE, 'wrangler.log')}`);
    stop(1);
  }
  await new Promise((r) => setTimeout(r, 500));
}

console.log(`
  Your local site is running.

    Website     ${BASE}/
    Shop        ${BASE}/shop/
    Sign in     ${BASE}/login/      (use any e-mail; click "Local test only: open the sign-in e-mail")
    Account     ${BASE}/account/

  Pretend to be a calculator (in another terminal, from the repo root):
    node web/dev/calculator.mjs new      register a calculator and show its link code
    node web/dev/calculator.mjs status   what the calculator would show
    node web/dev/calculator.mjs solve    send a photo and print the answer

  Payments go to a fake Stripe page (${stripe.url}); no money moves.
  Data is kept in ${STATE} (start empty with --fresh). Press Ctrl+C to stop.
`);
