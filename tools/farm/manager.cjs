#!/usr/bin/env node
// Proofline Farm manager (phase 1). Plain code, no AI: it picks ready tasks, gives each its own git worktree,
// runs a farmhand (`claude -p`), runs the "cmd:" checks, runs an inspector (a second, fresh `claude -p`), and
// writes the result back. It keeps the weekly usage reserve, waits out usage limits and Wi-Fi drops, and writes
// a morning summary.
//
//   node manager.cjs run       keep farming until stopped (Ctrl+C, or create the PAUSE file)
//   node manager.cjs once      work through the ready tasks once, then exit
//   node manager.cjs status    what the farm is doing right now
//   node manager.cjs summary   the summary since the last morning summary
//   node manager.cjs add "title"   make a new task file to fill in
//
// Farm folder: PROOFLINE_FARM_DIR or C:\Users\<you>\ProoflineFarm. Plan: docs/business/proofline-farm-app-plan.md
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync, spawn } = require('child_process');
const tasks = require('./lib/tasks.cjs');
const { runClaude } = require('./lib/run.cjs');
const prompts = require('./lib/prompts.cjs');
const crew = require('./lib/agents.cjs');
const planner = require('./lib/planner.cjs');
const office = require('./lib/office.cjs');
const penny = require('./lib/penny.cjs');

const FARM = process.env.PROOFLINE_FARM_DIR || path.join(os.homedir(), 'ProoflineFarm');
const P = {
  config: path.join(FARM, 'farm.config.json'),
  tasks: path.join(FARM, 'tasks'),
  fields: path.join(FARM, 'fields'),
  runs: path.join(FARM, 'runs'),
  logs: path.join(FARM, 'logs'),
  state: path.join(FARM, 'state.json'),
  feed: path.join(FARM, 'farm.js'),
  usage: path.join(FARM, 'usage.json'),
  usageHistory: path.join(FARM, 'usage-history.jsonl'),
  wifi: path.join(FARM, 'wifi-state.json'),
  pause: path.join(FARM, 'PAUSE'),
  summary: path.join(FARM, 'summary.md'),
  summaries: path.join(FARM, 'summaries'),
  settings: path.join(FARM, 'farm-run-settings.json'),
  events: path.join(FARM, 'logs', 'events.jsonl'),
  business: path.join(FARM, 'business.json'),
  agentWatch: process.env.PROOFLINE_AGENT_WATCH || path.join(os.homedir(), '.claude', 'agent-watch', 'status.json'),
};

const DEFAULTS = {
  repos: { 'AI-CALCULATOR': { path: path.join(os.homedir(), 'OneDrive', 'Documents', 'GitHub', 'AI-CALCULATOR'), base: 'main' } },
  claudeCmd: ['claude'],
  workerModel: 'sonnet',
  checkerModel: 'haiku',
  restricted: true,                // claude --restricted: no user settings/mods in farm runs, file tools kept inside the worktree
  maxFarmhands: 3,                 // when you are not chatting (decision 1)
  maxWhileChatting: 1,             // while you chat (decision 2)
  chatIdleMinutes: 10,             // a chat counts as "you are chatting" until it has been quiet this long
  weeklyReserve: 30,               // % of the weekly limit that stays yours (decision 7)
  awakeHours: [7, 23],             // during these hours the farm keeps the 5-hour limit under awakeFiveHourCap
  awakeFiveHourCap: 60,
  staleUsageHours: 6,              // older usage reading = be careful (1 farmhand)
  limits: { small: { minutes: 25 }, medium: { minutes: 70 } },
  stuckMinutes: 10,
  repeatLimit: 5,
  maxTries: 2,
  cmdTimeoutMinutes: 10,
  maxNewTasksPerDay: 20,
  plannerModel: 'sonnet',          // the Foreman's and the agents' own planning runs
  chatModel: 'sonnet',             // chatting with an agent
  planEveryMinutes: 90,            // the Foreman's cycle
  agentCycleMinutes: 120,          // each worker's own cycle (when idle with nothing ready in its line)
  foremanMaxTasks: 5,
  agentCycleMaxTasks: 2,
  minReadyPerAgent: 1,             // the Foreman plans early when the seed bag has fewer ready tasks than this per worker
  maxTaskDepth: 2,                 // follow-up of a follow-up, no deeper
  summaryHour: 7,
  mailModel: 'sonnet',             // Penny's drafts (no tools)
  mailPollSeconds: 60,             // how often Penny looks for new customer mail
  mailMaxPerCycle: 3,              // drafts per look (the rest wait for the next one)
  businessEveryMinutes: 5,         // refresh the business numbers (Worker /v1/admin/business)
  tickSeconds: 15,
};

// ---------- small helpers ----------
const now = () => new Date();
const iso = d => (d ? new Date(d).toISOString() : null);
const localDay = (d = now()) => `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, '0')}-${String(d.getDate()).padStart(2, '0')}`;
const readJson = (f, dflt = null) => { try { return JSON.parse(fs.readFileSync(f, 'utf8').replace(/^\uFEFF/, '')); } catch { return dflt; } };
const writeJson = (f, v) => { fs.writeFileSync(f + '.tmp', JSON.stringify(v, null, 2)); fs.renameSync(f + '.tmp', f); };
const sleep = ms => new Promise(r => setTimeout(r, ms));
const fmtTime = d => new Date(d).toLocaleString('en-US', { weekday: 'short', hour: 'numeric', minute: '2-digit' });

function log(msg) {
  const d = new Date();
  const line = `${localDay(d)} ${d.toTimeString().slice(0, 8)}  ${msg}`;
  console.log(line);
  try { fs.appendFileSync(path.join(P.logs, 'farm.log'), line + '\n'); } catch { /* folder not made yet */ }
}

function event(type, task, text, agent = null) {
  const e = { at: iso(now()), type, task: task ? task.meta.id : null, title: task ? task.meta.title : null, text, agent: agent || (task && task.meta.agent) || null };
  try { fs.appendFileSync(P.events, JSON.stringify(e) + '\n'); } catch { /* best effort */ }
}

function loadConfig() {
  const user = readJson(P.config, {});
  const cfg = { ...DEFAULTS, ...user, limits: { ...DEFAULTS.limits, ...(user.limits || {}) } };
  if (typeof cfg.claudeCmd === 'string') cfg.claudeCmd = [cfg.claudeCmd];
  return cfg;
}

function setup() {
  for (const d of [FARM, P.tasks, P.fields, P.runs, P.logs, P.summaries, path.join(FARM, 'inbox')]) fs.mkdirSync(d, { recursive: true });
  if (!fs.existsSync(P.config)) writeJson(P.config, { repos: DEFAULTS.repos, weeklyReserve: DEFAULTS.weeklyReserve, maxFarmhands: DEFAULTS.maxFarmhands });
  const example = path.join(P.tasks, '_EXAMPLE.md');
  if (!fs.existsSync(example)) fs.copyFileSync(path.join(__dirname, 'templates', 'task-example.md'), example);
  crew.roster(FARM);
  crew.ensureGoals(FARM);
  penny.ensureFacts(FARM);
  // Every farm run gets the guard as a PreToolUse hook.
  const guard = path.join(__dirname, 'lib', 'guard.cjs');
  writeJson(P.settings, {
    hooks: { PreToolUse: [{ matcher: '*', hooks: [{ type: 'command', command: `node "${guard}"` }] }] },
  });
}

function git(cwd, args, opts = {}) {
  const r = spawnSync('git', args, { cwd, encoding: 'utf8', windowsHide: true, maxBuffer: 64 * 1024 * 1024, ...opts });
  return { ok: r.status === 0, out: (r.stdout || '').trim(), err: (r.stderr || '').trim() };
}

// ---------- the outside world: usage, your chats, the internet ----------
function usageNow(cfg) {
  const u = readJson(P.usage);
  if (!u || !Array.isArray(u.rateLimits)) return { known: false };
  const pick = kind => u.rateLimits.find(r => r.kind === kind) || null;
  const week = pick('seven_day');
  const five = pick('five_hour');
  const ageH = (Date.now() - Number(u.at || 0)) / 3600000;
  return {
    known: true, at: u.at, ageH, stale: ageH > cfg.staleUsageHours,
    weekly: week ? week.percentUsed : null, weeklyResetsAt: week && week.resetsAt,
    five: five ? five.percentUsed : null, fiveResetsAt: five && five.resetsAt,
  };
}

function farmSessionIds(state) {
  return new Set(Object.values(state.active || {}).map(a => a.sessionId).filter(Boolean).concat(state.pastSessions || []));
}

function chattingNow(cfg, state) {
  const s = readJson(P.agentWatch);
  if (!s || !s.at || Date.now() - s.at > 120000 || !Array.isArray(s.sessions)) return false;
  const mine = farmSessionIds(state);
  return s.sessions.some(x => {
    if (mine.has(x.id)) return false;
    if (/\[Farm T-\d+\]|ProoflineFarm|[\\/ ]fields[\\/ ]|T-\d{4}/i.test(`${x.title} ${x.project}`)) return false;
    return x.status === 'running' || Number(x.idle) < cfg.chatIdleMinutes * 60;
  });
}

function online() {
  const w = readJson(P.wifi);
  if (!w) return true; // no watchdog installed: assume online, network errors are still handled per run
  const fresh = w.checkedAt && Date.now() - new Date(w.checkedAt).getTime() < 10 * 60000;
  return !fresh || w.online !== false;
}

// ---------- budget rules (plan: "Weekly usage budget") ----------
function budget(cfg, state) {
  const u = usageNow(cfg);
  const line = 100 - cfg.weeklyReserve;
  const out = { usage: u, line, pace: null, max: cfg.maxFarmhands, reason: '' };
  if (state.limitUntil && Date.now() < new Date(state.limitUntil).getTime()) {
    return { ...out, max: 0, reason: `usage limit reached, sleeping until ${fmtTime(state.limitUntil)}` };
  }
  if (!u.known) return { ...out, max: 1, reason: 'no usage reading yet: 1 farmhand until the first run reports one' };

  if (u.weekly !== null && u.weekly >= line) {
    return { ...out, max: 0, stopAll: true, reason: `weekly use ${u.weekly}% reached the farm line (${line}%; ${cfg.weeklyReserve}% is yours). Back after the reset ${u.weeklyResetsAt ? fmtTime(u.weeklyResetsAt) : ''}` };
  }
  // Daily pacing: spread what's left of the farm's share over the days until the weekly reset.
  if (u.weekly !== null && u.weeklyResetsAt) {
    const today = localDay();
    if (!state.pacing || state.pacing.day !== today) {
      const daysLeft = Math.max(1, (new Date(u.weeklyResetsAt).getTime() - Date.now()) / 86400000);
      state.pacing = { day: today, weeklyAtStart: u.weekly, daysLeft };
    }
    const p = state.pacing;
    out.pace = Math.min(line, p.weeklyAtStart + (line - p.weeklyAtStart) / Math.max(1, p.daysLeft));
    if (u.weekly >= out.pace) return { ...out, max: 0, reason: `today's farm allowance is used (week at ${u.weekly}%, today's line ${out.pace.toFixed(1)}%). More tomorrow` };
  }
  const h = now().getHours();
  const awake = h >= cfg.awakeHours[0] && h < cfg.awakeHours[1];
  if (awake && u.five !== null && u.five >= cfg.awakeFiveHourCap) {
    return { ...out, max: 0, reason: `5-hour limit at ${u.five}% during your awake hours (cap ${cfg.awakeFiveHourCap}%), waiting for it to refill` };
  }
  if (u.stale) return { ...out, max: 1, reason: `usage reading is ${u.ageH.toFixed(1)} h old: 1 farmhand until a fresh one` };
  return out;
}

// ---------- worktrees ----------
function repoFor(cfg, task) {
  const r = cfg.repos[task.meta.repo || 'AI-CALCULATOR'];
  if (!r || !fs.existsSync(r.path)) throw new Error(`repo "${task.meta.repo}" is not in farm.config.json (or its folder is missing)`);
  return r;
}

function ensureField(cfg, task) {
  const repo = repoFor(cfg, task);
  const field = path.join(P.fields, task.meta.id);
  let branch = task.meta.branch || `farm/${task.meta.id}`;
  if (!fs.existsSync(path.join(field, '.git'))) {
    git(repo.path, ['worktree', 'prune']);
    // A branch can be checked out in only one worktree; if another farm folder holds this name, pick a new one.
    const inUse = new Set(git(repo.path, ['worktree', 'list', '--porcelain']).out.split(/\r?\n/)
      .filter(l => l.startsWith('branch refs/heads/')).map(l => l.slice('branch refs/heads/'.length)));
    for (let i = 2; inUse.has(branch); i++) branch = `farm/${task.meta.id}-${i}`;
    if (branch !== task.meta.branch) tasks.update(task, { branch });
    const hasBranch = git(repo.path, ['rev-parse', '--verify', '--quiet', `refs/heads/${branch}`]).ok;
    const r = hasBranch
      ? git(repo.path, ['worktree', 'add', field, branch])
      : git(repo.path, ['worktree', 'add', '-b', branch, field, repo.base || 'main']);
    if (!r.ok) throw new Error(`could not make the worktree: ${r.err}`);
  }
  // .farm/ (report, drafts) never shows up as a change to commit.
  const common = git(field, ['rev-parse', '--git-common-dir']).out;
  const exclude = path.join(path.resolve(field, common), 'info', 'exclude');
  try {
    const cur = fs.existsSync(exclude) ? fs.readFileSync(exclude, 'utf8') : '';
    if (!/^\.farm\/$/m.test(cur)) { fs.mkdirSync(path.dirname(exclude), { recursive: true }); fs.appendFileSync(exclude, `${cur.endsWith('\n') || !cur ? '' : '\n'}.farm/\n`); }
  } catch { /* not fatal */ }
  fs.mkdirSync(path.join(field, '.farm'), { recursive: true });
  return { field, branch, repo };
}

function changes(field) {
  git(field, ['add', '-N', '.']);
  const stat = git(field, ['diff', 'HEAD', '--stat']).out;
  let diff = git(field, ['diff', 'HEAD']).out;
  if (diff.length > 40000) diff = diff.slice(0, 40000) + '\n... (diff cut at 40,000 characters)';
  return { stat, diff };
}

// ---------- one task, start to finish ----------
function runEnv(task, field) {
  const env = { ...process.env, PROOFLINE_FARM_DIR: FARM, PROOFLINE_TASK_ID: task.meta.id, PROOFLINE_FIELD: field };
  // If the manager was started from inside a Claude chat or the desktop app, drop that session's own settings
  // (its mods, its login hand-off, its proxy address). Farm runs use your normal `claude /login` sign-in.
  for (const k of Object.keys(env)) {
    if (/^(CLAUDE_CODE_|CLAUDECODE$|CLAUDE_AGENT_SDK|CLAUDE_PID$|CLAUDE_EFFORT$|CLAUDE_PREVIEW|ANTHROPIC_BASE_URL$|PROOFLINE_ADMIN_TOKEN$)/.test(k)) delete env[k];
  }
  return env;
}

function commonArgs(cfg) {
  const a = [];
  if (cfg.restricted) a.push('--restricted');
  a.push('--strict-mcp-config', '--settings', P.settings, '--plugin-dir', path.join(__dirname, 'usage-mod'));
  return a;
}

function runCmd(cmd, cwd, timeoutMin) {
  return new Promise(resolve => {
    const child = spawn(cmd, { cwd, shell: true, windowsHide: true });
    let out = '';
    const add = d => { out = (out + d).slice(-6000); };
    child.stdout.on('data', add);
    child.stderr.on('data', add);
    const t = setTimeout(() => { require('./lib/run.cjs').killTree(child); resolve({ cmd, code: -1, timedOut: true, output: out }); }, timeoutMin * 60000);
    child.on('close', code => { clearTimeout(t); resolve({ cmd, code, timedOut: false, output: out }); });
    child.on('error', e => { clearTimeout(t); resolve({ cmd, code: -1, timedOut: false, output: e.message }); });
  });
}

function extractJson(text) {
  const s = String(text || '');
  const a = s.indexOf('{');
  const b = s.lastIndexOf('}');
  if (a < 0 || b <= a) return null;
  try { return JSON.parse(s.slice(a, b + 1)); } catch { return null; }
}

// Ends a run that didn't finish its job for an outside reason (limit, internet, login, stop) without counting a try.
function handleInterruption(state, task, r, phase) {
  const back = { status: 'ready', tries: Math.max(0, Number(task.meta.tries || 1) - 1) };
  if (r.outcome === 'limit') {
    const until = r.resetAt || new Date(Date.now() + 60 * 60000);
    state.limitUntil = iso(new Date(until.getTime() + 2 * 60000));
    tasks.update(task, back, `${phase}: usage limit reached, back to ready (no try used). Farm sleeps until ${fmtTime(state.limitUntil)}.`);
    event('limit', task, `usage limit, sleeping until ${state.limitUntil}`);
    log(`${task.meta.id} hit the usage limit; farm sleeps until ${state.limitUntil}`);
    return true;
  }
  if (r.outcome === 'network') {
    tasks.update(task, back, `${phase}: internet dropped, back to ready (no try used).`);
    event('network', task, 'internet dropped during the run');
    state.networkPauseUntil = iso(new Date(Date.now() + 5 * 60000));
    return true;
  }
  if (r.outcome === 'login') {
    tasks.update(task, back, `${phase}: Claude is not logged in, back to ready (no try used).`);
    state.loginNeeded = true;
    event('login', task, 'Claude needs a login: run `claude` in a terminal and sign in');
    log('Claude is not logged in. Run `claude` in a terminal, sign in, then delete the LOGIN-NEEDED file.');
    fs.writeFileSync(path.join(FARM, 'LOGIN-NEEDED'), 'Run `claude` in a terminal and sign in, then delete this file.\n');
    return true;
  }
  if (r.outcome === 'killed' && (state.active[task.meta.id] || {}).stopBy === 'you') {
    tasks.update(task, { ...back, status: 'idea' }, `${phase}: stopped by you. Set status: ready to run it again.`);
    event('stopped', task, 'stopped by you');
    return true;
  }
  if (r.outcome === 'killed') {
    tasks.update(task, back, `${phase}: stopped by the farm (budget line or pause), back to ready (no try used).`);
    return true;
  }
  return false;
}

async function processTask(cfg, state, task, agent, live) {
  const id = task.meta.id;
  const tries = Number(task.meta.tries || 0) + 1;
  const runDir = path.join(P.runs, id);
  fs.mkdirSync(runDir, { recursive: true });
  if (task.meta.agent !== agent.id) tasks.update(task, { agent: agent.id });
  const active = state.active[id] = { id, title: task.meta.title, agent: agent.id, phase: 'working', try: tries, since: iso(now()), doing: 'getting the field ready', toolCalls: 0, sessionId: null };
  const shouldStop = () => !!active.stop;

  let fieldInfo;
  try { fieldInfo = ensureField(cfg, task); } catch (e) {
    tasks.update(task, { status: 'needs-you' }, `could not start: ${e.message}`);
    event('needs-you', task, e.message);
    delete state.active[id];
    return;
  }
  const { field, branch } = fieldInfo;
  tasks.update(task, { status: 'working', tries }, `try ${tries} started by ${agent.name} in ${field} (branch ${branch}).`);
  log(`${id} try ${tries}: ${agent.name} starts "${task.meta.title}"`);
  event('start', task, `${agent.name} starts try ${tries}`, agent.id);
  crew.say(FARM, agent.id, 'system', `Started ${id}: ${task.meta.title}`);

  const limit = cfg.limits[task.meta.budget] || cfg.limits.small;
  const env = runEnv(task, field);
  const w = await runClaude({
    cmd: cfg.claudeCmd, cwd: field, env, logFile: path.join(runDir, `try${tries}-worker.jsonl`), interactive: true,
    args: ['-p', '--input-format', 'stream-json', '--output-format', 'stream-json', '--verbose', '--model', task.meta.model || cfg.workerModel,
      ...commonArgs(cfg),
      '--tools', 'Read,Edit,Write,Glob,Grep,Bash,WebSearch,WebFetch',
      '--allowedTools', 'Read', 'Edit', 'Write', 'Glob', 'Grep', 'Bash', 'WebSearch', 'WebFetch',
      '--permission-mode', 'acceptEdits',
      '--append-system-prompt', prompts.workerRules(field, agent, crew.memory(FARM, agent.id))],
    prompt: prompts.workerPrompt(task, task.raw),
    timeoutMin: limit.minutes, stuckMin: cfg.stuckMinutes, repeatLimit: cfg.repeatLimit, shouldStop,
    onSpawn: h => live.set(agent.id, { ...h, kind: 'task', taskId: id }),
    onEvent: e => {
      if (e.kind === 'tool') Object.assign(active, { doing: e.doing, tool: e.tool, toolCalls: e.toolCalls, lastAt: iso(now()) });
      if (e.kind === 'text') { active.lastLine = e.text.slice(0, 200); crew.say(FARM, agent.id, 'agent', e.text.slice(0, 600)); }
    },
  });
  live.delete(agent.id);
  active.sessionId = w.sessionId;
  if (w.sessionId) state.pastSessions = [...(state.pastSessions || []), w.sessionId].slice(-200);
  state.spent = (state.spent || 0) + (w.costUsd || 0);

  if (handleInterruption(state, task, w, 'farmhand')) { delete state.active[id]; return; }
  if (w.outcome !== 'ok') {
    return finishTry(cfg, state, task, tries, `farmhand run ended badly (${w.outcome}): ${w.detail.slice(0, 300)}`, '');
  }

  // cmd: checks, run by the farm itself.
  active.phase = 'checking';
  active.doing = 'running the Done-when checks';
  tasks.update(task, { status: 'checking' });
  const cmdResults = [];
  for (const d of task.doneWhen.filter(x => x.type === 'cmd' && x.text)) cmdResults.push(await runCmd(d.text, field, cfg.cmdTimeoutMinutes));
  fs.writeFileSync(path.join(runDir, `try${tries}-checks.json`), JSON.stringify(cmdResults, null, 2));
  const report = readText(path.join(field, '.farm', 'REPORT.md'));
  if (report) fs.writeFileSync(path.join(runDir, `try${tries}-report.md`), report);
  const failed = cmdResults.filter(r => r.code !== 0);
  if (failed.length) {
    const notes = failed.map(r => `"${r.cmd}" exited ${r.code}${r.timedOut ? ' (timed out)' : ''}: ${r.output.trim().split(/\r?\n/).slice(-6).join(' | ').slice(0, 400)}`).join('; ');
    return finishTry(cfg, state, task, tries, `cmd checks failed`, `Fix these failing checks: ${notes}`);
  }

  // The inspector: a fresh run that never saw the farmhand's session.
  active.doing = 'Hawk the inspector is checking the work';
  active.inspector = 'hawk';
  event('checking', task, `Hawk inspects ${agent.name}'s work`, 'hawk');
  const { stat, diff } = changes(field);
  const drafts = readDrafts(field);
  const c = await runClaude({
    cmd: cfg.claudeCmd, cwd: field, env, logFile: path.join(runDir, `try${tries}-checker.jsonl`),
    args: ['-p', '--output-format', 'stream-json', '--verbose', '--model', cfg.checkerModel, ...commonArgs(cfg),
      '--tools', 'Read,Glob,Grep', '--allowedTools', 'Read', 'Glob', 'Grep'],
    prompt: prompts.checkerPrompt(task, { cmdResults, diffStat: stat, diff, report, drafts }),
    timeoutMin: 15, stuckMin: cfg.stuckMinutes, repeatLimit: cfg.repeatLimit, shouldStop,
  });
  if (c.sessionId) state.pastSessions = [...(state.pastSessions || []), c.sessionId].slice(-200);
  state.spent = (state.spent || 0) + (c.costUsd || 0);
  if (handleInterruption(state, task, c, 'inspector')) { delete state.active[id]; return; }
  const v = c.outcome === 'ok' ? extractJson(c.result) : null;
  fs.writeFileSync(path.join(runDir, `try${tries}-verdict.json`), JSON.stringify(v || { error: c.outcome, raw: c.result }, null, 2));
  if (!v || !['pass', 'fail', 'needs-you'].includes(v.verdict)) {
    tasks.update(task, { status: 'needs-you' }, `the inspector gave no clear verdict (${c.outcome}). Work is in ${field}.`);
    event('needs-you', task, 'inspector gave no clear verdict');
    delete state.active[id];
    return;
  }

  addFollowUps(cfg, state, task, v.next_tasks);
  crew.say(FARM, 'hawk', 'agent', `${id} (${agent.name}): ${v.verdict.toUpperCase()}. ${v.why}`);
  const learned = crew.lessonsFromReport(report);
  if (learned.length) crew.remember(FARM, agent.id, learned.map(l => `${id}: ${l}`));
  if (v.verdict === 'pass') {
    tasks.update(task, { status: 'harvest' }, `PASSED: ${v.why} Ready for you to review and commit in ${field} (branch ${branch}). Changed: ${stat.split(/\r?\n/).pop() || 'nothing'}`);
    event('harvest', task, `${v.why} | ${field} | ${stat.split(/\r?\n/).pop() || ''}`, agent.id);
    crew.say(FARM, agent.id, 'system', `${id} passed Hawk's inspection and waits for Nirav to commit.`);
    log(`${id} passed: ${v.why}`);
    delete state.active[id];
    return;
  }
  if (v.verdict === 'needs-you') {
    tasks.update(task, { status: 'needs-you' }, `NEEDS YOU: ${v.question || v.why}`);
    event('needs-you', task, v.question || v.why, agent.id);
    delete state.active[id];
    return;
  }
  return finishTry(cfg, state, task, tries, `inspector: ${v.why}`, v.fix_notes || v.why);
}

function finishTry(cfg, state, task, tries, why, fixNotes) {
  const id = task.meta.id;
  if (tries >= cfg.maxTries) {
    tasks.update(task, { status: 'needs-you' }, `try ${tries} failed (${why}). ${cfg.maxTries} tries used, so it waits for you. ${fixNotes}`);
    event('needs-you', task, `failed ${tries} times: ${why}`);
    // Remember why, so the planner doesn't propose the same impossible work again.
    crew.addLesson(FARM, `${id} "${task.meta.title}" failed ${tries} times: ${why}. ${String(fixNotes || '').slice(0, 200)}`);
    log(`${id} failed ${tries} times, now waits for you`);
  } else {
    tasks.update(task, { status: 'ready' }, `try ${tries} failed (${why}). Next try must: ${fixNotes || 'see above'}`);
    event('retry', task, why);
    log(`${id} try ${tries} failed, will try again: ${why}`);
  }
  delete state.active[id];
}

// Creates task files from filtered drafts (planner, inspector or chat), within the daily cap.
function plant(cfg, state, drafts, { from, by, depth = 0, parent = null, userAsked = false }) {
  const today = localDay();
  if (!state.newTasks || state.newTasks.day !== today) state.newTasks = { day: today, count: 0 };
  const made = [];
  for (const d of drafts) {
    if (!userAsked && state.newTasks.count >= cfg.maxNewTasksPerDay) { event('skipped', null, `daily cap of ${cfg.maxNewTasksPerDay} new tasks reached: "${d.title}" not planted`, by); break; }
    const t = tasks.create(P.tasks, { ...d, repo: (parent && parent.meta.repo) || Object.keys(cfg.repos)[0], from });
    const extra = { depth };
    if (d.agent) extra.agent = d.agent;
    if (parent) extra.from = parent.meta.id;
    tasks.update(t, extra);
    if (!userAsked) state.newTasks.count++;
    event('new-task', t, `planted by ${from}`, by);
    made.push(t);
  }
  return made;
}

function addFollowUps(cfg, state, parent, list) {
  const depth = Number(parent.meta.depth || 0) + 1;
  if (!Array.isArray(list) || !list.length || depth > cfg.maxTaskDepth) return;
  const roster = crew.roster(FARM);
  const { accepted, rejected } = planner.filterTasks(list, { list: tasks.loadAll(P.tasks).tasks, roster, defaultAgent: null, max: 3, requireValue: false });
  for (const r of rejected) event('skipped', parent, `inspector idea "${r.title}" skipped: ${r.why}`, 'hawk');
  plant(cfg, state, accepted, { from: `Hawk (inspecting ${parent.meta.id})`, by: 'hawk', depth, parent });
}

// ---------- the crew: talking to agents, and their own planning ----------
// A read-only copy of main for planning and chats (they look at the code without touching anything).
function foremanField(cfg) {
  const name = Object.keys(cfg.repos)[0];
  const repo = cfg.repos[name];
  const field = path.join(P.fields, '_foreman');
  if (!fs.existsSync(path.join(field, '.git'))) {
    git(repo.path, ['worktree', 'prune']);
    const r = git(repo.path, ['worktree', 'add', '--detach', field, repo.base || 'main']);
    if (!r.ok) throw new Error(`could not make the Foreman's copy: ${r.err}`);
  } else {
    git(field, ['checkout', '-q', '--detach', '--force', repo.base || 'main']);
  }
  return field;
}

function crewEnv(agentId, field) {
  const env = { ...process.env, PROOFLINE_FARM_DIR: FARM, PROOFLINE_TASK_ID: `${agentId}-chat`, PROOFLINE_FIELD: field };
  for (const k of Object.keys(env)) if (/^(CLAUDE_CODE_|CLAUDECODE$|CLAUDE_AGENT_SDK|CLAUDE_PID$|CLAUDE_EFFORT$|CLAUDE_PREVIEW|ANTHROPIC_BASE_URL$|PROOFLINE_ADMIN_TOKEN$)/.test(k)) delete env[k];
  return env;
}

function applyActions(cfg, state, agent, actions) {
  if (!actions || typeof actions !== 'object') return [];
  const notes = [];
  if (actions.memory) { crew.remember(FARM, agent.id, `Nirav: ${String(actions.memory).slice(0, 200)}`); notes.push('saved a note to memory'); }
  if (actions.goals_append && agent.role === 'foreman') {
    fs.appendFileSync(path.join(FARM, 'goals.md'), `\n- ${String(actions.goals_append).replace(/\r?\n/g, ' ').slice(0, 300)} (added by Fern from chat, ${localDay()})\n`);
    notes.push('added to the goals');
    event('goals', null, `Fern added to the goals: ${String(actions.goals_append).slice(0, 120)}`, agent.id);
  }
  if (Array.isArray(actions.tasks) && actions.tasks.length) {
    const roster = crew.roster(FARM);
    const { accepted, rejected } = planner.filterTasks(actions.tasks, { list: tasks.loadAll(P.tasks).tasks, roster, defaultAgent: agent.role === 'worker' ? agent.id : null, max: 5, requireValue: false, userAsked: true });
    const made = plant(cfg, state, accepted, { from: `${agent.name} (chat with Nirav)`, by: agent.id, userAsked: true });
    if (made.length) notes.push(`planted ${made.map(t => t.meta.id).join(', ')}`);
    for (const r of rejected) notes.push(`skipped "${r.title}" (${r.why})`);
  }
  return notes;
}

async function chatWith(cfg, state, agent, text, live) {
  const field = foremanField(cfg);
  const sid = crew.chatSession(FARM, agent.id);
  state.agentBusy[agent.id] = { what: 'chatting', since: iso(now()) };
  event('chat', null, `Nirav talks to ${agent.name}`, agent.id);
  let replyText = '';
  const r = await runClaude({
    cmd: cfg.claudeCmd, cwd: field, env: crewEnv(agent.id, field), interactive: true,
    logFile: path.join(P.logs, `chat-${agent.id}.jsonl`),
    args: ['-p', '--input-format', 'stream-json', '--output-format', 'stream-json', '--verbose', '--model', cfg.chatModel,
      ...commonArgs(cfg), ...(sid ? ['--resume', sid] : []),
      '--tools', 'Read,Glob,Grep', '--allowedTools', 'Read', 'Glob', 'Grep',
      '--append-system-prompt', planner.chatSystemPrompt(agent, crew.memory(FARM, agent.id), field)],
    prompt: `Nirav says: ${text}`,
    timeoutMin: 10, stuckMin: 5, repeatLimit: cfg.repeatLimit,
    onSpawn: h => live.set(agent.id, { ...h, kind: 'chat' }),
    onEvent: e => { if (e.kind === 'text') replyText = e.text; },
  });
  live.delete(agent.id);
  delete state.agentBusy[agent.id];
  if (r.sessionId) { crew.setChatSession(FARM, agent.id, r.sessionId); state.pastSessions = [...(state.pastSessions || []), r.sessionId].slice(-200); }
  if (r.outcome === 'error' && sid && /session|resume|not found/i.test(r.detail)) crew.setChatSession(FARM, agent.id, ' ');
  if (r.outcome === 'limit') { state.limitUntil = iso(new Date((r.resetAt || new Date(Date.now() + 3600e3)).getTime() + 120e3)); crew.say(FARM, agent.id, 'system', `Out of usage until ${fmtTime(state.limitUntil)}. I'll be back then.`); return; }
  if (r.outcome !== 'ok') { crew.say(FARM, agent.id, 'system', `Couldn't answer (${r.outcome}). Try again in a bit.`); return; }
  const { reply, actions } = crew.splitFarmBlock(r.result || replyText);
  crew.say(FARM, agent.id, 'agent', reply || '(no reply)');
  const notes = applyActions(cfg, state, agent, actions);
  if (notes.length) crew.say(FARM, agent.id, 'system', notes.join('; '));
}

// Self-prompting: the Foreman's cycle and each worker's own cycle.
async function planCycle(cfg, state, agent, live) {
  const field = foremanField(cfg);
  const isForeman = agent.role === 'foreman';
  const list = tasks.loadAll(P.tasks).tasks;
  const ctx = { goals: crew.ensureGoals(FARM), list, lessons: crew.lessons(FARM), timeline: eventsSince(null).slice(-40), roster: crew.roster(FARM),
    business: office.planningNotes((readJson(P.business) || {}).biz) };
  const max = isForeman ? cfg.foremanMaxTasks : cfg.agentCycleMaxTasks;
  state.agentBusy[agent.id] = { what: 'planning', since: iso(now()) };
  event('plan', null, isForeman ? 'Fern starts planning the farm\'s next work' : `${agent.name} looks for work in their line`, agent.id);
  const r = await runClaude({
    cmd: cfg.claudeCmd, cwd: field, env: crewEnv(agent.id, field), logFile: path.join(P.logs, `plan-${agent.id}.jsonl`),
    args: ['-p', '--output-format', 'stream-json', '--verbose', '--model', cfg.plannerModel, ...commonArgs(cfg),
      '--tools', 'Read,Glob,Grep', '--allowedTools', 'Read', 'Glob', 'Grep'],
    prompt: isForeman ? planner.foremanPrompt(ctx, max) : planner.agentCyclePrompt(agent, crew.memory(FARM, agent.id), ctx, max),
    timeoutMin: 15, stuckMin: cfg.stuckMinutes, repeatLimit: cfg.repeatLimit,
  });
  delete state.agentBusy[agent.id];
  if (r.sessionId) state.pastSessions = [...(state.pastSessions || []), r.sessionId].slice(-200);
  const cyc = state.cycles[agent.id] = state.cycles[agent.id] || {};
  cyc.lastAt = iso(now());
  if (r.outcome === 'limit') { state.limitUntil = iso(new Date((r.resetAt || new Date(Date.now() + 3600e3)).getTime() + 120e3)); cyc.note = 'out of usage'; return; }
  const out = r.outcome === 'ok' ? extractJson(r.result) : null;
  if (!out) { cyc.note = `planning failed (${r.outcome})`; event('plan', null, `${agent.name}'s planning gave no usable plan (${r.outcome})`, agent.id); return; }
  const decided = isForeman ? applyDecisions(list, out.decisions) : [];
  const { accepted, rejected } = planner.filterTasks(out.tasks, { list, roster: ctx.roster, defaultAgent: isForeman ? null : agent.id, max });
  const made = plant(cfg, state, accepted, { from: isForeman ? 'Fern (Foreman plan)' : `${agent.name} (own cycle)`, by: agent.id });
  for (const x of rejected) event('skipped', null, `${agent.name}'s idea "${x.title}" skipped: ${x.why}`, agent.id);
  cyc.note = `${String(out.note || '').slice(0, 300)}${made.length ? ` Planted ${made.map(t => t.meta.id).join(', ')}.` : ' Nothing new planted.'}${decided.length ? ` Stuck tasks: ${decided.join('; ')}.` : ''}`;
  crew.say(FARM, agent.id, 'agent', cyc.note);
  log(`${agent.name} planned: ${cyc.note}`);
}

// ---------- the business (Business HQ, docs/web/BACK_OFFICE.md) ----------
async function refreshBusiness(api, state) {
  const [health, biz] = await Promise.all([api.health(), api.business()]);
  const al = office.alerts(biz.ok ? biz.data : null, health);
  if (!biz.ok && health.ok) al.unshift({ kind: 'worker', level: 'urgent', text: `Business numbers: ${biz.error}`, go: 'settings' });
  state.businessFeed = {
    linked: true, at: Date.now(), ok: biz.ok, workerMs: health.ms, workerUp: health.ok, alerts: al,
    numbers: biz.ok ? { sales: biz.data.sales, toShip: biz.data.orders.to_ship, store: biz.data.store, mail: biz.data.mail,
      waitlist: { total: biz.data.waitlist.total, week: biz.data.waitlist.week }, plan: biz.data.plan } : null,
  };
  if (biz.ok) writeJson(P.business, { at: iso(now()), biz: biz.data });
}

// Penny: one look at the inbox. Drafts (never sends) replies to new mail, and redoes drafts Nirav sent back.
// Returns how many drafts she saved.
async function pennyCycle(cfg, state, api, agent) {
  const queue = (state.redraftQueue || []).splice(0);
  const fresh = await api.mail('new');
  if (!fresh.ok) { state.redraftQueue = [...queue, ...(state.redraftQueue || [])]; return 0; }
  for (const m of fresh.data.mail || []) if (!m.auto && !queue.some(q => q.id === m.id)) queue.push({ id: m.id, note: '' });
  const facts = penny.ensureFacts(FARM);
  const store = ((readJson(P.business) || {}).biz || {}).store || null;
  const dir = path.join(FARM, 'agents', agent.id);
  fs.mkdirSync(dir, { recursive: true });
  let made = 0;
  const later = queue.slice(cfg.mailMaxPerCycle).filter(q => q.note);   // redo requests beyond this round wait
  for (const job of queue.slice(0, cfg.mailMaxPerCycle)) {
    const d = await api.mailGet(job.id);
    if (!d.ok || !['new', 'drafted'].includes(d.data.mail.status)) continue;
    const subject = String(d.data.mail.subject || '(no subject)').slice(0, 60);
    state.agentBusy[agent.id] = { what: 'writing', since: iso(now()), doing: `drafting a reply: "${subject}"` };
    const r = await runClaude({
      cmd: cfg.claudeCmd, cwd: dir, env: crewEnv(agent.id, dir), logFile: path.join(P.logs, 'mail-penny.jsonl'),
      args: ['-p', '--output-format', 'stream-json', '--verbose', '--model', cfg.mailModel, ...commonArgs(cfg), '--tools', ''],
      prompt: penny.draftPrompt({ detail: d.data, facts, store, note: job.note, memory: crew.memory(FARM, agent.id) }),
      timeoutMin: 6, stuckMin: 4, repeatLimit: cfg.repeatLimit,
    });
    delete state.agentBusy[agent.id];
    if (r.sessionId) state.pastSessions = [...(state.pastSessions || []), r.sessionId].slice(-200);
    if (r.outcome === 'limit') {
      state.limitUntil = iso(new Date((r.resetAt || new Date(Date.now() + 3600e3)).getTime() + 120e3));
      if (job.note) later.unshift(job);
      break;
    }
    const out = r.outcome === 'ok' ? penny.parseDraft(r.result) : null;
    if (!out) { event('mail', null, `Penny couldn't draft "${subject}" (${r.outcome}); she'll try again next round`, agent.id); continue; }
    const saved = await api.mailDraft(job.id, out);
    if (!saved.ok) { event('mail', null, `Penny's draft for "${subject}" wasn't saved: ${saved.error}`, agent.id); continue; }
    made++;
    event('mail', null, out.draft ? `Penny drafted a reply to "${subject}" (${out.category}): it waits for your OK` : `Penny read "${subject}" (${out.category}): she suggests no reply`, agent.id);
  }
  state.redraftQueue = [...later, ...(state.redraftQueue || [])];
  if (made) {
    const c = state.cycles[agent.id] = state.cycles[agent.id] || {};
    c.lastAt = iso(now());
    c.note = `Drafted ${made} repl${made > 1 ? 'ies' : 'y'}. They wait in Business HQ, Inbox, for your OK.`;
  }
  return made;
}

// Fern's calls on stuck (needs-you) tasks: retry once with new instructions, drop, or keep for Nirav.
function applyDecisions(list, decisions) {
  const done = [];
  for (const d of Array.isArray(decisions) ? decisions : []) {
    const t = list.find(x => x.meta.id === d.task && x.meta.status === 'needs-you');
    if (!t) continue;
    const why = String(d.instructions || '').replace(/\r?\n/g, ' ').slice(0, 500);
    const retries = Number(t.meta.fern_retries || 0);
    if (d.action === 'retry' && retries < 1) {
      tasks.update(t, { status: 'ready', tries: 0, fern_retries: retries + 1 }, `Fern sent it back for one more try. Do this differently: ${why}`);
      event('fern-retry', t, why, 'fern');
      done.push(`${t.meta.id} retry`);
    } else if (d.action === 'drop') {
      tasks.update(t, { status: 'done', dropped: true }, `Dropped by Fern: ${why}`);
      event('fern-drop', t, why, 'fern');
      crew.addLesson(FARM, `${t.meta.id} "${t.meta.title}" dropped by Fern: ${why}`);
      done.push(`${t.meta.id} dropped`);
    } else {
      tasks.update(t, { fern_reviewed: true }, `Fern: this one needs Nirav. ${why}`);
      event('needs-you', t, `Fern says it needs you: ${why}`, 'fern');
      done.push(`${t.meta.id} kept for Nirav`);
    }
  }
  return done;
}

function dueCycles(cfg, state, list, roster, busy) {
  const nowMs = Date.now();
  const due = [];
  const ready = list.filter(t => t.meta.status === 'ready');
  const workers = roster.filter(a => a.role === 'worker');
  roster.forEach((a, i) => {
    const cyc = state.cycles[a.id] = state.cycles[a.id] || {};
    if (!cyc.nextAt) cyc.nextAt = iso(new Date(nowMs + (a.role === 'foreman' ? 60e3 : (10 + i * 7) * 60e3))); // stagger first cycles
  });
  const fern = roster.find(a => a.role === 'foreman');
  if (fern && !busy.has(fern.id)) {
    const c = state.cycles[fern.id];
    const low = ready.length < cfg.minReadyPerAgent * workers.length
      || list.some(t => t.meta.status === 'needs-you' && !t.meta.fern_reviewed); // stuck tasks wait for her call
    if (c.planNow || (nowMs >= new Date(c.nextAt).getTime() && low)) due.push(fern);
    else if (nowMs >= new Date(c.nextAt).getTime()) c.nextAt = iso(new Date(nowMs + 30 * 60e3)); // enough work: check again later
  }
  for (const a of workers) {
    if (busy.has(a.id)) continue;
    const c = state.cycles[a.id];
    if (nowMs < new Date(c.nextAt).getTime()) continue;
    const mine = ready.some(t => t.meta.agent === a.id || (!t.meta.agent && (a.kinds || []).includes(t.meta.kind || 'code')));
    if (mine) { c.nextAt = iso(new Date(nowMs + 30 * 60e3)); continue; }
    due.push(a);
  }
  return due;
}

function readText(f) { try { return fs.readFileSync(f, 'utf8'); } catch { return ''; } }
function readDrafts(field) {
  const dir = path.join(field, '.farm', 'drafts');
  let files = [];
  try { files = fs.readdirSync(dir); } catch { return ''; }
  return files.slice(0, 10).map(f => `--- ${f}\n${readText(path.join(dir, f)).slice(0, 3000)}`).join('\n');
}

// ---------- harvest: notice when you've committed ----------
function checkHarvests(cfg, list) {
  for (const t of list.filter(x => x.meta.status === 'harvest')) {
    const field = path.join(P.fields, t.meta.id);
    if (!fs.existsSync(field)) continue;
    let base = 'main';
    try { base = repoFor(cfg, t).base || 'main'; } catch { continue; }
    const dirty = git(field, ['status', '--porcelain']).out.split(/\r?\n/).filter(l => l.trim() && !/\.farm\//.test(l));
    const ahead = Number(git(field, ['rev-list', '--count', `${base}..HEAD`]).out || 0);
    if (!dirty.length && ahead > 0) {
      tasks.update(t, { status: 'done' }, `committed by you (${ahead} commit(s) on ${t.meta.branch || 'farm/' + t.meta.id}). Done.`);
      event('done', t, 'you committed it');
    }
  }
}

// ---------- feed for the wallpaper / app, and the summary ----------
function writeFeed(cfg, state, list, b, chatting) {
  const counts = {};
  for (const s of tasks.STATUSES) counts[s] = 0;
  for (const t of list) counts[t.meta.status] = (counts[t.meta.status] || 0) + 1;
  const feed = {
    at: Date.now(), paused: state.paused || null, reason: b.reason || '', online: online(), chatting,
    loginNeeded: !!state.loginNeeded, limitUntil: state.limitUntil || null,
    usage: { weekly: b.usage.weekly ?? null, five: b.usage.five ?? null, line: b.line, pace: b.pace, reserve: cfg.weeklyReserve, weeklyResetsAt: b.usage.weeklyResetsAt || null, readingAgeH: b.usage.ageH ?? null },
    farmhands: Object.values(state.active).map(a => ({ id: a.id, title: a.title, agent: a.agent || null, inspector: a.inspector || null, phase: a.phase, try: a.try, doing: a.doing, tool: a.tool || null, toolCalls: a.toolCalls, since: a.since, lastLine: a.lastLine || null })),
    agents: agentFeed(state),
    foreman: { nextCycleAt: (state.cycles && state.cycles.fern && state.cycles.fern.nextAt) || null, lastCycleAt: (state.cycles && state.cycles.fern && state.cycles.fern.lastAt) || null, lastNote: (state.cycles && state.cycles.fern && state.cycles.fern.note) || null },
    counts,
    tasks: list.map(t => ({ id: t.meta.id, title: t.meta.title, status: t.meta.status, priority: t.meta.priority, kind: t.meta.kind })),
    business: state.businessFeed || null,
  };
  writeJson(P.state, { ...state, feed });
  fs.writeFileSync(P.feed, `window.FARM = ${JSON.stringify(feed)};\n`);
}

// What each character is doing right now, for the game screen.
function agentFeed(state) {
  const asleep = !!(state.paused || state.loginNeeded || (state.limitUntil && Date.now() < new Date(state.limitUntil).getTime()));
  return crew.roster(FARM).map(a => {
    const job = Object.values(state.active).find(x => x.agent === a.id);
    const check = a.role === 'inspector' ? Object.values(state.active).find(x => x.phase === 'checking' && x.inspector) : null;
    const busy = (state.agentBusy || {})[a.id];
    const cyc = (state.cycles || {})[a.id] || {};
    const log = crew.chatLog(FARM, a.id, 1)[0];
    let status = 'idle', taskId = null, taskTitle = null, doing = null, since = null;
    if (job) { status = job.phase === 'checking' ? 'waiting' : 'working'; taskId = job.id; taskTitle = job.title; doing = job.doing; since = job.since; }
    else if (check) { status = 'checking'; taskId = check.id; taskTitle = check.title; doing = `inspecting ${check.id}`; since = check.since; }
    else if (busy) { status = busy.what; since = busy.since; doing = busy.doing || (busy.what === 'planning' ? 'planning the next work' : 'talking with Nirav'); }
    else if (asleep) status = 'asleep';
    if (status === 'waiting') status = 'working';
    return { id: a.id, name: a.name, title: a.title, role: a.role, color: a.color, home: a.home, kinds: a.kinds || [],
      status, taskId, taskTitle, doing, since, lastLine: (job && job.lastLine) || (log && log.from !== 'you' ? log.text.slice(0, 200) : null),
      nextCycleAt: cyc.nextAt || null, cycleNote: cyc.note || null, queued: ((state.chatQueue || {})[a.id] || []).length };
  });
}

// Wakes the loop as soon as the app drops a command in the inbox (so chats feel instant).
function waitInbox() {
  return new Promise(resolve => {
    let w;
    const done = () => { try { w && w.close(); } catch { /* ignore */ } resolve(); };
    try { fs.mkdirSync(path.join(FARM, 'inbox'), { recursive: true }); w = fs.watch(path.join(FARM, 'inbox'), done); } catch { /* polling still works */ }
    setTimeout(done, 60000);
  });
}

function eventsSince(sinceIso) {
  const since = sinceIso ? new Date(sinceIso).getTime() : 0;
  return readText(P.events).split(/\r?\n/).filter(Boolean).map(l => { try { return JSON.parse(l); } catch { return null; } })
    .filter(e => e && new Date(e.at).getTime() > since);
}

function wifiDropsSince(sinceIso) {
  const since = sinceIso ? new Date(sinceIso).getTime() : 0;
  const lines = readText(path.join(FARM, 'logs', 'wifi.log')).split(/\r?\n/);
  return lines.filter(l => /no internet \(check 1/.test(l) && new Date(l.slice(0, 19).replace(' ', 'T')).getTime() > since).length;
}

function buildSummary(cfg, state) {
  const since = state.lastSummaryAt || null;
  const ev = eventsSince(since);
  const list = tasks.loadAll(P.tasks).tasks;
  const by = s => list.filter(t => t.meta.status === s);
  const u = usageNow(cfg);
  const L = [];
  L.push(`# Farm summary, ${new Date().toLocaleString('en-US', { weekday: 'long', month: 'short', day: 'numeric', hour: 'numeric', minute: '2-digit' })}`);
  L.push(since ? `Since ${fmtTime(since)}.` : 'Since the farm started.');
  L.push('');
  const harvest = by('harvest');
  L.push(`## Ready for you to review and commit (${harvest.length})`);
  for (const t of harvest) L.push(`- **${t.meta.id} ${t.meta.title}**: worktree \`${path.join(P.fields, t.meta.id)}\`, branch \`${t.meta.branch || 'farm/' + t.meta.id}\``);
  if (!harvest.length) L.push('- nothing new');
  const needs = by('needs-you');
  L.push('', `## Waiting for you (${needs.length})`);
  for (const t of needs) L.push(`- **${t.meta.id} ${t.meta.title}**: ${lastLog(t)}`);
  if (!needs.length) L.push('- nothing');
  const done = ev.filter(e => e.type === 'done');
  if (done.length) L.push('', `## Committed by you: ${done.map(e => e.task).join(', ')}`);
  const retries = ev.filter(e => e.type === 'retry');
  if (retries.length) L.push('', `## Failed once, trying again (${retries.length})`, ...retries.map(e => `- ${e.task} ${e.title}: ${e.text}`));
  const added = ev.filter(e => e.type === 'new-task');
  if (added.length) L.push('', `## New tasks the farm added (${added.length})`, ...added.map(e => `- ${e.task} ${e.title} (${e.text})`));
  L.push('', '## Usage');
  if (u.known) {
    L.push(`- Weekly limit: **${u.weekly}%** used. The farm stops at ${100 - cfg.weeklyReserve}% (your reserve is ${cfg.weeklyReserve}%). Resets ${u.weeklyResetsAt ? fmtTime(u.weeklyResetsAt) : '?'}.`);
    L.push(`- 5-hour limit: ${u.five}%. Reading is ${u.ageH.toFixed(1)} h old.`);
  } else L.push('- No usage reading yet (the first farm run writes one).');
  const limits = ev.filter(e => e.type === 'limit').length;
  if (limits) L.push(`- Hit the usage limit ${limits} time(s) and waited for the reset.`);
  const drops = wifiDropsSince(since);
  const net = ev.filter(e => e.type === 'network').length;
  if (drops || net) L.push('', `## Wi-Fi`, `- ${drops} drop(s) caught by the watchdog; ${net} farm run(s) were cut off and requeued without using a try.`);
  if (state.loginNeeded) L.push('', '## Action needed', '- Claude is not logged in. Open a terminal, run `claude`, sign in, then delete `LOGIN-NEEDED` in the farm folder.');
  const blocks = readText(path.join(P.logs, 'guard.log')).split(/\r?\n/).filter(l => l && (!since || l.slice(0, 24) > since)).length;
  if (blocks) L.push('', `## Safety guard`, `- Blocked ${blocks} action(s) (see logs/guard.log). That's the guard working, not an error.`);
  L.push('', `Counts: ${tasks.STATUSES.map(s => `${s} ${by(s).length}`).join(' · ')}`);
  return L.join('\n') + '\n';
}

function lastLog(t) {
  const lines = (t.farmLog || '').split(/\r?\n/).filter(Boolean);
  return (lines.pop() || '').replace(/^-\s*\S+\s+\S+\s+/, '').slice(0, 220);
}

function maybeMorningSummary(cfg, state) {
  const d = now();
  if (d.getHours() < cfg.summaryHour || state.lastSummaryDay === localDay(d)) return;
  const text = buildSummary(cfg, state);
  fs.writeFileSync(P.summary, text);
  fs.writeFileSync(path.join(P.summaries, `${localDay(d)}.md`), text);
  state.lastSummaryDay = localDay(d);
  state.lastSummaryAt = iso(d);
  log('morning summary written: ' + P.summary);
}

function recordUsageHistory(cfg, state, chatting) {
  const u = usageNow(cfg);
  if (!u.known) return;
  const hour = new Date().toISOString().slice(0, 13);
  if (state.lastHistoryHour === hour) return;
  state.lastHistoryHour = hour;
  fs.appendFileSync(P.usageHistory, JSON.stringify({ at: iso(now()), weekly: u.weekly, five: u.five, readingAt: iso(u.at), farmhands: Object.keys(state.active).length, chatting }) + '\n');
}

// ---------- the loop ----------
// Commands from the desktop app: small files in <farm>/inbox, e.g. "stop-T-0001".
// Also messages: "msg-<agent>-<time>.json" ({ text }) and "plan-now".
function processInbox(state, live) {
  const dir = path.join(FARM, 'inbox');
  let files = [];
  try { files = fs.readdirSync(dir).sort(); } catch { return; }
  for (const f of files) {
    const full = path.join(dir, f);
    const stop = f.match(/^stop-(T-\d+)$/);
    if (stop && state.active[stop[1]]) { state.active[stop[1]].stop = true; state.active[stop[1]].stopBy = 'you'; log(`${stop[1]}: stop requested from the app`); }
    const redraft = f.match(/^redraft-(mail_[0-9a-f]{16})\.json$/);
    if (redraft) {
      const note = String((readJson(full) || {}).note || '').trim().slice(0, 1000);
      state.redraftQueue = [...(state.redraftQueue || []).filter(x => x.id !== redraft[1]), { id: redraft[1], note }];
      event('mail', null, `Nirav asked Penny to redo a draft${note ? ': ' + note.slice(0, 80) : ''}`, 'penny');
    }
    if (f === 'plan-now') { state.cycles.fern = { ...(state.cycles.fern || {}), planNow: true }; log('Fern was asked to plan now'); }
    const msg = f.match(/^msg-([a-z0-9_-]+)-\d+\.json$/i);
    if (msg) {
      const text = String((readJson(full) || {}).text || '').trim().slice(0, 4000);
      const id = msg[1];
      if (text) {
        crew.say(FARM, id, 'you', text);
        const h = live.get(id);
        if (h && h.isOpen()) {
          h.send(`Message from Nirav (the owner), sent while you work. Take it into account now and reply to it briefly in your next message:\n${text}`);
          event('message', null, `Nirav → ${id} (live): ${text.slice(0, 100)}`, id);
        } else {
          (state.chatQueue[id] = state.chatQueue[id] || []).push(text);
        }
      }
    }
    try { fs.unlinkSync(full); } catch { /* ignore */ }
  }
}

function pickReady(list, busy) {
  const done = new Set(list.filter(t => t.meta.status === 'done').map(t => t.meta.id));
  return list
    .filter(t => t.meta.status === 'ready' && !busy.has(t.meta.id))
    .filter(t => (t.meta.blocked_by || []).every(b => done.has(b)))
    .sort((a, b) => (Number(a.meta.priority) || 9) - (Number(b.meta.priority) || 9) || String(a.meta.id).localeCompare(String(b.meta.id)));
}

function validate(list) {
  for (const t of list.filter(x => x.meta.status === 'ready')) {
    if (!t.doneWhen.some(d => d.text)) tasks.update(t, { status: 'idea' }, 'has no "Done when" items, so it stays an idea until you add some.');
    else if (!['small', 'medium'].includes(t.meta.budget || 'small')) tasks.update(t, { status: 'idea' }, `budget "${t.meta.budget}" is too big for one run: split it into small/medium tasks.`);
  }
}

// Only one manager may farm at a time (the login task and a start-farm.cmd window would fight over tasks).
function takeLock() {
  const lock = path.join(FARM, 'manager.lock');
  const other = readJson(lock);
  if (other && other.pid && other.pid !== process.pid) {
    try { process.kill(other.pid, 0); return false; } catch { /* that manager is gone */ }
  }
  writeJson(lock, { pid: process.pid, since: iso(now()) });
  process.on('exit', () => { try { if ((readJson(lock) || {}).pid === process.pid) fs.unlinkSync(lock); } catch { /* ignore */ } });
  return true;
}

async function loop(mode) {
  setup();
  if (!takeLock()) {
    console.log(`Another farm manager is already running (see ${path.join(FARM, 'manager.lock')}). Nothing to do.`);
    return;
  }
  const cfg = loadConfig();
  const state = readJson(P.state, {}) || {};
  delete state.feed;
  state.active = {};
  state.agentBusy = {};
  state.chatQueue = state.chatQueue || {};
  state.cycles = state.cycles || {};
  for (const c of Object.values(state.cycles)) delete c.planNow;
  const live = new Map();       // agent id -> open run that accepts live messages
  const crewRuns = new Map();   // agent id -> chat/plan promise
  state.loginNeeded = fs.existsSync(path.join(FARM, 'LOGIN-NEEDED'));
  // Anything left "working"/"checking" by a crash or restart goes back to ready.
  for (const t of tasks.loadAll(P.tasks).tasks) {
    if (['working', 'checking'].includes(t.meta.status)) tasks.update(t, { status: 'ready', tries: Math.max(0, Number(t.meta.tries || 1) - 1) }, 'the farm restarted mid-run; back to ready (no try used).');
  }
  log(`farm manager started (${mode}), farm folder ${FARM}`);
  const running = new Map();
  let stopping = false;
  process.on('SIGINT', () => { if (stopping) process.exit(1); stopping = true; log('stopping: letting current runs end (Ctrl+C again to quit now)'); for (const a of Object.values(state.active)) a.stop = true; });

  const officeCfg = office.settings();
  const officeOn = officeCfg.enabled;
  const api = officeOn ? office.client(officeCfg) : null;
  let lastBiz = 0, lastMailPoll = 0, bizRun = null;
  if (officeOn) log(`business link on (${officeCfg.apiUrl})`); else log('business link off (no admin token: add it in the app, Business HQ -> Settings)');
  if (!officeOn) state.businessFeed = { linked: false, at: Date.now(), alerts: [] };

  let lastReason = '';
  for (;;) {
    const { tasks: list, errors } = tasks.loadAll(P.tasks);
    for (const e of errors) if (!state.badFiles?.includes(e)) { log(`task file problem: ${e}`); state.badFiles = [...(state.badFiles || []), e]; }
    validate(list);
    processInbox(state, live);
    state.loginNeeded = fs.existsSync(path.join(FARM, 'LOGIN-NEEDED'));
    state.paused = fs.existsSync(P.pause) ? 'PAUSE file' : stopping ? 'stopping' : state.loginNeeded ? 'login needed' : null;
    const chatting = chattingNow(cfg, state);
    const b = budget(cfg, state);
    let max = Math.min(b.max, chatting ? cfg.maxWhileChatting : cfg.maxFarmhands);
    let reason = b.reason;
    if (state.paused) { max = 0; reason = `paused (${state.paused})`; }
    else if (!online() || (state.networkPauseUntil && Date.now() < new Date(state.networkPauseUntil).getTime())) { max = 0; reason = 'internet is down, waiting (no tries used)'; }
    if (b.stopAll) for (const a of Object.values(state.active)) a.stop = true;
    if (reason !== lastReason) { if (reason) log(reason); lastReason = reason; }

    const roster = crew.roster(FARM);
    const busyAgents = () => new Set([...Object.values(state.active).map(a => a.agent), ...Object.keys(state.agentBusy)]);
    const canWork = max > 0;                       // budget, pause, internet and login all allow runs
    const canTalk = !state.paused && !state.loginNeeded && online() && !(state.limitUntil && Date.now() < new Date(state.limitUntil).getTime());

    // 1. Answer Nirav first: idle agents with messages waiting get a chat run.
    for (const [id, queue] of Object.entries(state.chatQueue)) {
      const agent = roster.find(a => a.id === id);
      if (!agent || !queue.length) { delete state.chatQueue[id]; continue; }
      if (!canTalk || busyAgents().has(id) || crewRuns.has(id)) continue;
      const text = queue.splice(0).join('\n\n');
      crewRuns.set(id, chatWith(cfg, state, agent, text, live)
        .catch(e => { log(`chat with ${id} crashed: ${e.message}`); crew.say(FARM, id, 'system', `Chat failed: ${e.message}`); delete state.agentBusy[id]; })
        .finally(() => crewRuns.delete(id)));
    }

    // 2. Start farmhands up to the allowed number, each task to a free agent of the right kind.
    const ready = pickReady(list, new Set(running.keys()));
    for (const t of ready) {
      if (running.size >= max) break;
      const agent = crew.pickAgentFor(roster, t, busyAgents());
      if (!agent) continue;
      state.active[t.meta.id] = { id: t.meta.id, agent: agent.id, title: t.meta.title, phase: 'working', since: iso(now()), doing: 'walking to the field' };
      const p = processTask(cfg, state, t, agent, live).catch(e => {
        log(`${t.meta.id} crashed: ${e.stack || e.message}`);
        try { tasks.update(t, { status: 'needs-you' }, `the farm hit an error: ${e.message}`); } catch { /* ignore */ }
        delete state.active[t.meta.id];
      }).finally(() => { running.delete(t.meta.id); live.delete(agent.id); });
      running.set(t.meta.id, p);
    }

    // 3. Self-prompting: the Foreman's cycle and each idle worker's own cycle (one planning run at a time).
    if (canWork && mode !== 'once' && ![...crewRuns.keys()].some(k => (state.agentBusy[k] || {}).what === 'planning')) {
      const due = dueCycles(cfg, state, list, roster, busyAgents());
      const agent = due[0];
      if (agent) {
        const c = state.cycles[agent.id];
        delete c.planNow;
        c.nextAt = iso(new Date(Date.now() + (agent.role === 'foreman' ? cfg.planEveryMinutes : cfg.agentCycleMinutes) * 60e3));
        crewRuns.set(agent.id, planCycle(cfg, state, agent, live)
          .catch(e => { log(`${agent.name}'s planning crashed: ${e.message}`); delete state.agentBusy[agent.id]; })
          .finally(() => crewRuns.delete(agent.id)));
      }
    }

    // 4. The business: refresh the numbers, and Penny drafts replies to new customer mail.
    if (officeOn) {
      if (!bizRun && Date.now() - lastBiz >= cfg.businessEveryMinutes * 60e3) {
        lastBiz = Date.now();
        bizRun = refreshBusiness(api, state).catch(e => log(`business numbers failed: ${e.message}`)).finally(() => { bizRun = null; });
      }
      const pennyAgent = roster.find(a => a.id === 'penny');
      const due = Date.now() - lastMailPoll >= cfg.mailPollSeconds * 1000 || (state.redraftQueue || []).length > 0;
      if (pennyAgent && canTalk && due && !crewRuns.has('penny') && !busyAgents().has('penny')) {
        lastMailPoll = Date.now();
        crewRuns.set('penny', pennyCycle(cfg, state, api, pennyAgent)
          .then(made => { if (made) lastBiz = 0; })
          .catch(e => { log(`Penny crashed: ${e.message}`); delete state.agentBusy.penny; })
          .finally(() => crewRuns.delete('penny')));
      }
    }

    checkHarvests(cfg, list);
    recordUsageHistory(cfg, state, chatting);
    maybeMorningSummary(cfg, state);
    writeFeed(cfg, state, tasks.loadAll(P.tasks).tasks, { ...b, reason }, chatting);

    if (mode === 'once' && running.size === 0 && crewRuns.size === 0 && !Object.values(state.chatQueue).some(q => q.length) && (pickReady(tasks.loadAll(P.tasks).tasks, new Set()).length === 0 || max === 0)) break;
    if (stopping && running.size === 0) break;
    await Promise.race([sleep(cfg.tickSeconds * 1000), ...running.values(), ...crewRuns.values(), waitInbox()]);
  }
  writeFeed(cfg, state, tasks.loadAll(P.tasks).tasks, { ...budget(cfg, state), reason: `manager stopped${lastReason ? ' (last: ' + lastReason + ')' : ''}` }, false);
  log('farm manager stopped');
}

function status() {
  const cfg = loadConfig();
  const s = readJson(P.state, {}) || {};
  const f = s.feed || {};
  const list = tasks.loadAll(P.tasks).tasks;
  const age = f.at ? Math.round((Date.now() - f.at) / 1000) : null;
  console.log(`Farm folder: ${FARM}`);
  console.log(age === null ? 'Manager: never ran' : age > 60 ? `Manager: not running (last seen ${Math.round(age / 60)} min ago)` : 'Manager: running');
  if (f.reason) console.log(`Now: ${f.reason}`);
  const u = usageNow(cfg);
  console.log(u.known ? `Usage: week ${u.weekly}% (farm line ${100 - cfg.weeklyReserve}%), 5-hour ${u.five}%, reading ${u.ageH.toFixed(1)} h old` : 'Usage: no reading yet');
  for (const a of f.farmhands || []) console.log(`  farmhand ${a.id} [${a.phase}, try ${a.try}] ${a.title} — ${a.doing}`);
  for (const st of tasks.STATUSES) {
    const xs = list.filter(t => t.meta.status === st);
    if (xs.length) console.log(`${st.padEnd(9)} ${xs.map(t => `${t.meta.id} ${t.meta.title}`).join(' | ')}`);
  }
}

function main() {
  const [cmd = 'status', ...rest] = process.argv.slice(2);
  if (cmd === 'run' || cmd === 'once') return loop(cmd);
  if (cmd === 'status') return status();
  if (cmd === 'summary') { setup(); const s = readJson(P.state, {}) || {}; return process.stdout.write(buildSummary(loadConfig(), s)); }
  if (cmd === 'add') {
    setup();
    const t = tasks.create(P.tasks, { title: rest.join(' ') || 'New task', status: 'idea' });
    console.log(`Made ${t.file}. Fill in "What to do" and "Done when", then set status: ready.`);
    return;
  }
  console.log('usage: node manager.cjs run | once | status | summary | add "title"');
}

module.exports = { budget, chattingNow, usageNow, pickReady, buildSummary, P, FARM, DEFAULTS,
  // for tests
  setup, loadConfig, planCycle, chatWith, processInbox, dueCycles, agentFeed, plant, pennyCycle, refreshBusiness, runEnv, crewEnv };
if (require.main === module) Promise.resolve(main()).catch(e => { log(`fatal: ${e.stack || e.message}`); process.exit(1); });
