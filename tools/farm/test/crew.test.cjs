// node tools/farm/test/crew.test.cjs - the agents: picking who does a task, the planner's filters, Fern's planning
// cycle, chatting with an agent (reply, planted task, memory), live messages, cycles. Fake Claude, no usage.
'use strict';
const assert = require('assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const crew = require('../lib/agents.cjs');
const planner = require('../lib/planner.cjs');
const results = [];
function test(name, fn) {
  try { fn(); results.push(`ok   ${name}`); } catch (e) { results.push(`FAIL ${name}: ${e.message}`); process.exitCode = 1; }
}

// ---------- pure parts ----------
test('tasks go to a free agent of the right kind', () => {
  const roster = crew.DEFAULT_ROSTER;
  const t = kind => ({ meta: { kind } });
  assert.strictEqual(crew.pickAgentFor(roster, t('marketing'), new Set()).id, 'poppy');
  assert.strictEqual(crew.pickAgentFor(roster, t('research'), new Set()).id, 'sage');
  assert.strictEqual(crew.pickAgentFor(roster, t('code'), new Set()).id, 'bolt');
  assert.strictEqual(crew.pickAgentFor(roster, t('code'), new Set(['bolt'])).id, 'rivet');
  assert.strictEqual(crew.pickAgentFor(roster, t('code'), new Set(['bolt', 'rivet'])), null, 'no coder free -> wait');
  assert.strictEqual(crew.pickAgentFor(roster, { meta: { kind: 'code', agent: 'rivet' } }, new Set()).id, 'rivet', 'named agent wins');
  assert.strictEqual(crew.pickAgentFor(roster, t('marketing'), new Set(['poppy'])), null, 'marketing waits for Poppy');
});

test('planner filter: needs Done-when, skips repeats and low-value ideas', () => {
  const list = [{ meta: { id: 'T-0001', title: 'Add a unit test for the x^-1 key', status: 'ready' } }];
  const { accepted, rejected } = planner.filterTasks([
    { title: 'Add unit test for x^-1 key', done_when: ['cmd: x'], value: 5 },
    { title: 'Document the keypad matrix', done_when: [], value: 5 },
    { title: 'Rename variables for fun', done_when: ['judge: nicer'], value: 1 },
    { title: 'Explain the camera pins in firmware README', done_when: ['judge: clear'], value: 4, agent: 'rivet', kind: 'code' },
  ], { list, roster: crew.DEFAULT_ROSTER, max: 5 });
  assert.deepStrictEqual(accepted.map(a => a.title), ['Explain the camera pins in firmware README']);
  assert.strictEqual(accepted[0].agent, 'rivet');
  assert.deepStrictEqual(rejected.map(r => r.why.split(' ')[0]), ['too', 'no', 'low']);
});

test('lessons come out of a farmhand report', () => {
  const r = '# Report\nDid things.\n\n## Lessons\n- Tests live in tests/tests.cpp\n- Nirav wants plain words\n';
  assert.deepStrictEqual(crew.lessonsFromReport(r), ['Tests live in tests/tests.cpp', 'Nirav wants plain words']);
  assert.deepStrictEqual(crew.lessonsFromReport('no lessons here'), []);
});

test('a chat reply splits into text and farm actions', () => {
  const f = '`'.repeat(3);
  const { reply, actions } = crew.splitFarmBlock(`On it!\n${f}farm\n{"tasks":[{"title":"x"}]}\n${f}`);
  assert.strictEqual(reply, 'On it!');
  assert.strictEqual(actions.tasks[0].title, 'x');
});

// ---------- the crew against a throwaway repo, in a child process (the manager reads its farm folder at start) ----------
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'crewtest-'));
const git = (cwd, ...a) => spawnSync('git', a, { cwd, encoding: 'utf8' });
const repo = path.join(tmp, 'repo');
fs.mkdirSync(repo);
git(repo, 'init', '-q', '-b', 'main');
git(repo, 'config', 'user.email', 't@example.com');
git(repo, 'config', 'user.name', 'T');
fs.writeFileSync(path.join(repo, 'README.md'), 'test\n');
git(repo, 'add', '.');
git(repo, 'commit', '-qm', 'init');
const farm = path.join(tmp, 'farm');
fs.mkdirSync(farm);
fs.writeFileSync(path.join(farm, 'farm.config.json'), JSON.stringify({ repos: { TEST: { path: repo, base: 'main' } }, claudeCmd: ['node', path.join(__dirname, 'fake-claude.cjs')] }));

const script = `
const m = require(${JSON.stringify(path.join(__dirname, '..', 'manager.cjs'))});
const tasks = require(${JSON.stringify(path.join(__dirname, '..', 'lib', 'tasks.cjs'))});
const crew = require(${JSON.stringify(path.join(__dirname, '..', 'lib', 'agents.cjs'))});
const out = {};
const roster0 = () => crew.roster(process.env.PROOFLINE_FARM_DIR);
(async () => {
  m.setup();
  const cfg = m.loadConfig();
  const farm = process.env.PROOFLINE_FARM_DIR;
  tasks.create(farm + '/tasks', { title: 'Say hello', repo: 'TEST', doneWhen: [{ type: 'cmd', text: 'x' }], status: 'harvest' });
  tasks.create(farm + '/tasks', { title: 'Build with cmake', repo: 'TEST', doneWhen: [{ type: 'cmd', text: 'cmake' }], status: 'needs-you' });
  tasks.create(farm + '/tasks', { title: 'Solder the board', repo: 'TEST', doneWhen: [{ type: 'judge', text: 'soldered' }], status: 'needs-you' });
  out.stuckDue = m.dueCycles(cfg, { active: {}, agentBusy: {}, chatQueue: {}, cycles: { fern: { nextAt: new Date(Date.now() - 1).toISOString() } } }, tasks.loadAll(farm + '/tasks').tasks.concat([{ meta: { status: 'ready' } }, { meta: { status: 'ready' } }, { meta: { status: 'ready' } }, { meta: { status: 'ready' } }]), roster0(), new Set()).map(a => a.id);
  const state = { active: {}, agentBusy: {}, chatQueue: {}, cycles: {} };
  const live = new Map();
  const roster = crew.roster(farm);
  // cycles are staggered on first look, Fern first
  m.dueCycles(cfg, state, [], roster, new Set());
  out.firstCycles = Object.fromEntries(Object.entries(state.cycles).map(([k, v]) => [k, Math.round((new Date(v.nextAt) - Date.now()) / 60000)]));
  state.cycles.fern.nextAt = new Date(Date.now() - 1000).toISOString();
  out.due = m.dueCycles(cfg, state, [], roster, new Set()).map(a => a.id);
  // Fern plans
  await m.planCycle(cfg, state, roster.find(a => a.id === 'fern'), live);
  out.afterPlan = tasks.loadAll(farm + '/tasks').tasks.map(t => ({ id: t.meta.id, title: t.meta.title, status: t.meta.status, agent: t.meta.agent || null, fern_retries: t.meta.fern_retries || 0, dropped: !!t.meta.dropped }));
  out.fernNote = state.cycles.fern.note;
  // Nirav messages Bolt while Bolt is idle -> queued -> chat
  require('fs').mkdirSync(farm + '/inbox', { recursive: true });
  require('fs').writeFileSync(farm + '/inbox/msg-bolt-1.json', JSON.stringify({ text: 'Can you make a goodbye page?' }));
  m.processInbox(state, live);
  out.queued = state.chatQueue.bolt;
  await m.chatWith(cfg, state, roster.find(a => a.id === 'bolt'), state.chatQueue.bolt.join(' '), live);
  out.chat = crew.chatLog(farm, 'bolt');
  out.memory = crew.memory(farm, 'bolt');
  out.afterChat = tasks.loadAll(farm + '/tasks').tasks.map(t => ({ title: t.meta.title, agent: t.meta.agent || null, status: t.meta.status }));
  // Nirav messages Poppy while she works -> sent live
  const sent = [];
  live.set('poppy', { send: t => sent.push(t), isOpen: () => true });
  require('fs').writeFileSync(farm + '/inbox/msg-poppy-2.json', JSON.stringify({ text: 'Keep it short' }));
  m.processInbox(state, live);
  out.liveSent = sent;
  out.feed = m.agentFeed(state).map(a => ({ id: a.id, status: a.status }));
  console.log(JSON.stringify(out));
})().catch(e => { console.error(e.stack); process.exit(1); });`;
const r = spawnSync('node', ['-e', script], { encoding: 'utf8', timeout: 120000, env: { ...process.env, PROOFLINE_FARM_DIR: farm } });
let out = null;
try { out = JSON.parse(r.stdout.trim().split(/\r?\n/).pop()); } catch { console.log(r.stdout, r.stderr); }

test('cycles: Fern plans first, workers are staggered', () => {
  assert.ok(out, 'crew script ran');
  assert.ok(out.firstCycles.fern <= 1);
  assert.ok(out.firstCycles.bolt >= 10 && out.firstCycles.sage > out.firstCycles.bolt);
  assert.deepStrictEqual(out.due, ['fern']);
});

test('Fern plans: good tasks planted for the right agents, repeat and low-value skipped', () => {
  const titles = out.afterPlan.map(t => t.title);
  assert.ok(titles.includes('Add Windows test runner script for the calculator core'));
  assert.ok(titles.includes('Draft three waitlist launch posts'));
  assert.ok(!titles.includes('Polish whitespace in comments everywhere'));
  assert.strictEqual(titles.filter(t => t === 'Say hello').length, 1);
  assert.strictEqual(out.afterPlan.find(t => t.title.startsWith('Draft three')).agent, 'poppy');
  assert.strictEqual(out.afterPlan.find(t => t.title.startsWith('Add Windows')).status, 'ready');
  assert.match(out.fernNote, /Planted T-\d+, T-\d+/);
});

test('Fern decides stuck tasks: one retried with new instructions, one dropped', () => {
  assert.ok(out.stuckDue.includes('fern'), 'stuck tasks make Fern plan even with plenty of ready work');
  const retry = out.afterPlan.find(t => t.title === 'Build with cmake');
  const drop = out.afterPlan.find(t => t.title === 'Solder the board');
  assert.strictEqual(retry.status, 'ready');
  assert.strictEqual(retry.fern_retries, 1);
  assert.strictEqual(drop.status, 'done');
  assert.ok(drop.dropped);
  assert.match(out.fernNote, /Stuck tasks: T-\d+ retry; T-\d+ dropped/);
});

test('chatting with an idle agent: reply, task planted for him, memory saved', () => {
  assert.deepStrictEqual(out.queued, ['Can you make a goodbye page?']);
  const froms = out.chat.map(c => c.from);
  assert.deepStrictEqual(froms.slice(0, 3), ['you', 'agent', 'system']);
  assert.strictEqual(out.chat[1].text, 'Sure, planting it now.');
  assert.match(out.chat[2].text, /planted T-\d+/);
  const t = out.afterChat.find(x => x.title === 'Write a goodbye page for the farm demo');
  assert.ok(t && t.agent === 'bolt' && t.status === 'ready');
  assert.match(out.memory, /likes short replies/);
});

test('a message to a working agent is sent to it live', () => {
  assert.strictEqual(out.liveSent.length, 1);
  assert.match(out.liveSent[0], /Keep it short/);
});

test('every agent shows up in the feed', () => {
  assert.deepStrictEqual(out.feed.map(a => a.id), ['fern', 'bolt', 'rivet', 'poppy', 'sage', 'hawk', 'penny']);
});

console.log(results.join('\n'));
try { fs.rmSync(tmp, { recursive: true, force: true }); } catch { /* fine */ }
