// node tools/farm/test/farm.test.cjs - runs the whole farm loop against a throwaway git repo with a fake Claude.
// Uses no Claude usage and doesn't touch your real farm folder or repos.
'use strict';
const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const MANAGER = path.join(__dirname, '..', 'manager.cjs');
const FAKE = path.join(__dirname, 'fake-claude.cjs');
const tasks = require('../lib/tasks.cjs');
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'farmtest-'));
const git = (cwd, ...a) => spawnSync('git', a, { cwd, encoding: 'utf8' });

// A tiny repo with one commit on main.
const repo = path.join(tmp, 'repo');
fs.mkdirSync(repo);
git(repo, 'init', '-q', '-b', 'main');
git(repo, 'config', 'user.email', 'test@example.com');
git(repo, 'config', 'user.name', 'Farm Test');
fs.writeFileSync(path.join(repo, 'README.md'), 'test\n');
git(repo, 'add', '.');
git(repo, 'commit', '-qm', 'init');

let n = 0;
function freshFarm(extraCfg = {}) {
  const farm = path.join(tmp, `farm${++n}`);
  fs.mkdirSync(path.join(farm, 'tasks'), { recursive: true });
  fs.writeFileSync(path.join(farm, 'farm.config.json'), JSON.stringify({
    repos: { TEST: { path: repo, base: 'main' } }, claudeCmd: ['node', FAKE], tickSeconds: 0.2, summaryHour: 0, ...extraCfg,
  }));
  return farm;
}
const HELLO_CHECK = `node -e "process.exit(require('fs').readFileSync('hello.txt','utf8').includes('hello')?0:1)"`;
function addTask(farm, title, extra = {}) {
  const t = tasks.create(path.join(farm, 'tasks'), { title, repo: 'TEST', doneWhen: [{ type: 'cmd', text: HELLO_CHECK }, { type: 'judge', text: 'it says hello' }], ...extra });
  return t.meta.id;
}
function once(farm, env = {}) {
  const r = spawnSync('node', [MANAGER, 'once'], {
    encoding: 'utf8', timeout: 120000,
    env: { ...process.env, PROOFLINE_FARM_DIR: farm, PROOFLINE_AGENT_WATCH: path.join(farm, 'no-agent-watch.json'), ...env },
  });
  if (r.status !== 0) console.log(r.stdout, r.stderr);
  return r.stdout;
}
const task = (farm, id) => tasks.load(path.join(farm, 'tasks', `${id}.md`));
const state = farm => JSON.parse(fs.readFileSync(path.join(farm, 'state.json'), 'utf8'));
const results = [];
function test(name, fn) {
  try { fn(); results.push(`ok   ${name}`); } catch (e) { results.push(`FAIL ${name}: ${e.message}`); process.exitCode = 1; }
}

test('a good task passes and waits for you to commit; the inspector adds a follow-up', () => {
  const farm = freshFarm({ maxTaskDepth: 0 });
  const id = addTask(farm, 'Say hello');
  once(farm);
  const t = task(farm, id);
  assert.strictEqual(t.meta.status, 'harvest');
  assert.strictEqual(t.meta.tries, 1);
  const field = path.join(farm, 'fields', id);
  assert.ok(fs.existsSync(path.join(field, 'hello.txt')), 'work is in the worktree');
  assert.ok(!git(field, 'status', '--porcelain').stdout.includes('.farm'), '.farm/ is ignored by git');
  assert.ok(fs.readFileSync(path.join(farm, 'summary.md'), 'utf8').includes('Ready for you to review'));
  // You commit it -> the next pass marks it done.
  git(field, 'add', '.');
  git(field, 'commit', '-qm', 'hello');
  once(farm);
  assert.strictEqual(task(farm, id).meta.status, 'done');
});

test('follow-up tasks from the inspector are added as ready (decision 4)', () => {
  const farm = freshFarm({ maxTaskDepth: 1 });
  addTask(farm, 'Say hello again');
  once(farm);
  const all = tasks.loadAll(path.join(farm, 'tasks')).tasks;
  const child = all.find(t => t.meta.from);
  assert.ok(child, 'a follow-up exists');
  assert.ok(['harvest', 'ready'].includes(child.meta.status), `follow-up status ${child.meta.status}`);
});

test('bad work fails the cmd check twice, then waits for you', () => {
  const farm = freshFarm();
  const id = addTask(farm, 'Say hello badly');
  once(farm, { FAKE_MODE: 'badwork' });
  const t = task(farm, id);
  assert.strictEqual(t.meta.status, 'needs-you');
  assert.strictEqual(t.meta.tries, 2);
  assert.match(t.farmLog, /Fix these failing checks/);
});

test('an inspector "fail" sends it back with fix notes', () => {
  const farm = freshFarm({ maxTries: 3 });
  const id = addTask(farm, 'Greeting');
  once(farm, { FAKE_MODE: 'fail-verdict' });
  const t = task(farm, id);
  assert.strictEqual(t.meta.status, 'needs-you');
  assert.match(t.farmLog, /Make it longer/);
});

test('usage limit: back to ready without using a try, farm sleeps until the reset', () => {
  const farm = freshFarm();
  const id = addTask(farm, 'Hit the limit');
  once(farm, { FAKE_MODE: 'limit' });
  const t = task(farm, id);
  assert.strictEqual(t.meta.status, 'ready');
  assert.strictEqual(t.meta.tries, 0);
  assert.ok(state(farm).limitUntil, 'limitUntil set');
  assert.ok(new Date(state(farm).limitUntil).getHours() === 3, 'reset parsed as 3am (+2 min)');
});

test('internet drop: back to ready without using a try', () => {
  const farm = freshFarm();
  const id = addTask(farm, 'Lose the internet');
  once(farm, { FAKE_MODE: 'network' });
  assert.strictEqual(task(farm, id).meta.status, 'ready');
  assert.strictEqual(task(farm, id).meta.tries, 0);
});

test('not logged in: pauses the farm and says so', () => {
  const farm = freshFarm();
  addTask(farm, 'No login');
  once(farm, { FAKE_MODE: 'login' });
  assert.ok(fs.existsSync(path.join(farm, 'LOGIN-NEEDED')));
});

test('weekly reserve: no farmhand starts once the week is past 100% - reserve', () => {
  const farm = freshFarm();
  const id = addTask(farm, 'Too late in the week');
  fs.writeFileSync(path.join(farm, 'usage.json'), JSON.stringify({ at: Date.now(), rateLimits: [
    { kind: 'seven_day', percentUsed: 71, resetsAt: new Date(Date.now() + 2 * 86400e3).toISOString() },
    { kind: 'five_hour', percentUsed: 10 }] }));
  once(farm);
  assert.strictEqual(task(farm, id).meta.status, 'ready');
  assert.match(state(farm).feed.reason, /reached the farm line/);
});

test('daily pacing: stops for the day when today\'s share is used', () => {
  process.env.PROOFLINE_FARM_DIR = freshFarm();
  const m = require('../manager.cjs');
  const cfg = { ...m.DEFAULTS };
  const st = { pacing: { day: new Date().toLocaleDateString('en-CA'), weeklyAtStart: 20, daysLeft: 4 } };
  // line 70, share left 50 over 4 days -> today's line 32.5
  fs.mkdirSync(m.FARM, { recursive: true });
  const prev = fs.existsSync(m.P.usage) ? fs.readFileSync(m.P.usage) : null;
  try {
    fs.writeFileSync(m.P.usage, JSON.stringify({ at: Date.now(), rateLimits: [{ kind: 'seven_day', percentUsed: 33, resetsAt: new Date(Date.now() + 4 * 86400e3).toISOString() }, { kind: 'five_hour', percentUsed: 1 }] }));
    const b = m.budget(cfg, st);
    assert.strictEqual(b.max, 0);
    assert.match(b.reason, /allowance/);
  } finally {
    if (prev) fs.writeFileSync(m.P.usage, prev); else fs.rmSync(m.P.usage, { force: true });
  }
});

test('a stuck farmhand is stopped and counted as a failed try', () => {
  const farm = freshFarm({ stuckMinutes: 0.05, maxTries: 1 });
  const id = addTask(farm, 'Get stuck');
  once(farm, { FAKE_MODE: 'stuck' });
  const t = task(farm, id);
  assert.strictEqual(t.meta.status, 'needs-you');
  assert.match(t.farmLog, /stuck/);
});

test('tasks without Done-when never start', () => {
  const farm = freshFarm();
  const t = tasks.create(path.join(farm, 'tasks'), { title: 'Vague idea', repo: 'TEST', status: 'ready' });
  once(farm);
  assert.strictEqual(task(farm, t.meta.id).meta.status, 'idea');
});

console.log(results.join('\n'));
// The worktrees belong to the throwaway repo; remove it all.
try { fs.rmSync(tmp, { recursive: true, force: true }); } catch { console.log(`(could not fully remove ${tmp})`); }
