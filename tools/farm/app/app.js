// Proofline Farm app screen: HUD, panels and overlays around the canvas world (game.js).
// Everything shown comes from the farm folder. Task titles, logs and chat are written by AI runs, so they are
// only ever set with textContent (DOM) or fillText (canvas), never as HTML.
'use strict';
const $ = id => document.getElementById(id);

// ---------- small helpers ----------
function el(tag, cls, text) {
  const e = document.createElement(tag);
  if (cls) e.className = cls;
  if (text !== undefined && text !== null) e.textContent = String(text);
  return e;
}
function btn(label, onClick, cls = 'gbtn small') {
  const b = el('button', cls, label);
  b.addEventListener('click', async e => { e.stopPropagation(); b.disabled = true; try { await onClick(); } finally { b.disabled = false; refresh(); } });
  return b;
}
const toMs = v => v == null || v === '' ? null : typeof v === 'number' ? v : new Date(v).getTime();
const ago = iso => {
  const t = toMs(iso); if (!t) return '';
  const s = Math.max(0, Math.round((Date.now() - t) / 1000));
  return s < 60 ? `${s}s` : s < 3600 ? `${Math.floor(s / 60)} min` : `${Math.floor(s / 3600)} h ${Math.floor((s % 3600) / 60)} min`;
};
const inTime = iso => {
  const t = toMs(iso); if (!t || isNaN(t)) return null;
  const s = Math.round((t - Date.now()) / 1000);
  if (s <= 0) return 'any moment';
  return s < 60 ? `${s}s` : s < 3600 ? `${Math.ceil(s / 60)}m` : `${Math.floor(s / 3600)}h ${Math.floor((s % 3600) / 60)}m`;
};
const when = iso => { const t = toMs(iso); return t ? new Date(t).toLocaleString(undefined, { weekday: 'short', hour: 'numeric', minute: '2-digit' }) : '?'; };
const hhmm = iso => { const t = toMs(iso); return t ? new Date(t).toLocaleTimeString(undefined, { hour: '2-digit', minute: '2-digit' }) : '--:--'; };
const short = (s, n) => { s = String(s ?? ''); return s.length > n ? s.slice(0, n - 1) + '…' : s; };
const STATUS_LABEL = { idle: 'Idle', working: 'Working', checking: 'Checking', chatting: 'Chatting', planning: 'Planning', writing: 'Writing', asleep: 'Asleep' };
function statusLine(a) {
  switch (a.status) {
    case 'working': return a.taskId ? `working on ${a.taskId}` : 'working';
    case 'checking': return a.taskId ? `checking ${a.taskId}` : 'checking';
    case 'planning': return 'planning the next tasks';
    case 'writing': return a.doing || 'drafting a customer reply';
    case 'chatting': return 'talking with you';
    case 'asleep': return 'asleep';
    default: return 'awaiting orders';
  }
}

// ---------- the farm API (real, or demo) ----------
const realApi = window.farm || {};
let demo = false;
let api = realApi;
async function call(name, ...args) {
  const f = api && api[name];
  if (typeof f !== 'function') return undefined;
  try { return await f(...args); } catch (e) { console.warn(`farm.${name} is not available yet:`, e && e.message); return undefined; }
}

// ---------- demo farm (#demo): fake data, nothing touches the real farm ----------
const DEFAULT_ROSTER = [
  { id: 'fern', name: 'Fern', title: 'Foreman', role: 'foreman', color: '#e57373', home: 'farmhouse', kinds: [] },
  { id: 'bolt', name: 'Bolt', title: 'Coder', role: 'worker', color: '#64b5f6', home: 'workshop', kinds: ['code', 'hardware-check'] },
  { id: 'rivet', name: 'Rivet', title: 'Coder', role: 'worker', color: '#4db6ac', home: 'workshop', kinds: ['code', 'hardware-check'] },
  { id: 'poppy', name: 'Poppy', title: 'Marketer', role: 'worker', color: '#f06292', home: 'market', kinds: ['marketing'] },
  { id: 'sage', name: 'Sage', title: 'Researcher', role: 'worker', color: '#ba68c8', home: 'library', kinds: ['research'] },
  { id: 'hawk', name: 'Hawk', title: 'Inspector', role: 'inspector', color: '#ffb74d', home: 'tower', kinds: [] },
  { id: 'penny', name: 'Penny', title: 'Postmaster', role: 'clerk', color: '#7986cb', home: 'postoffice', kinds: ['mail'] },
];
let D = null;
function demoInit() {
  const now = Date.now(), m = 60000;
  const T = (n, title, status, kind, extra = {}) => ({ id: `T-${n}`, title, status, priority: extra.priority ?? 3, kind, tries: extra.tries ?? 0, from: extra.from || null, branch: null, last: extra.last || '', hasField: true, todo: '', doneWhen: [], agent: extra.agent || null });
  D = {
    start: now, running: true, paused: false, startWithWindows: true, config: { weeklyReserve: 30, maxFarmhands: 3 },
    goals: 'This week:\n- Keypad tests for every key on the v15 board\n- Draft 3 short posts about the AI Solve feature (drafts only)\n- Find 2 cheaper suppliers for the colour LCD',
    tasks: [
      T(1, 'Set up the test runner', 'done', 'code'), T(2, 'Keypad map for row 1', 'done', 'code'), T(3, 'Write the waitlist page copy', 'done', 'marketing'),
      T(4, 'Check the battery holder footprint', 'done', 'hardware-check'), T(5, 'Compare 3 LCD suppliers', 'done', 'research'),
      T(6, 'Fix the x^-1 key rounding test', 'needs-you', 'code', { tries: 2, last: 'Try 2 failed: cmd "npm test -- keypad" exited 1 (expected 0.333333333, got 0.3333). Needs a decision: round to 9 or 10 digits?' }),
      T(7, 'Add unit tests for log_a b', 'harvest', 'code', { last: 'Passed all 3 checks. 4 files changed, 120 lines added.', agent: 'rivet' }),
      T(8, 'Draft 3 posts about AI Solve', 'harvest', 'marketing', { last: 'Passed. Drafts saved in docs/marketing/drafts.', agent: 'poppy' }),
      T(9, 'Abs key edge cases', 'checking', 'code', { agent: 'bolt', last: 'Work done, inspector is checking.' }),
      T(12, 'Unit tests for the x^3 key', 'working', 'code', { agent: 'bolt', priority: 1 }),
      T(15, 'Camera pin note for stage 6', 'working', 'hardware-check', { agent: 'rivet', priority: 2 }),
      T(16, 'Keypad row 3 tests', 'ready', 'code', { priority: 2, from: 'T-12' }), T(17, 'Find a cheaper LCD supplier', 'ready', 'research'),
      T(18, 'Write the FAQ draft', 'ready', 'marketing', { priority: 4 }),
      T(19, 'Make the boot logo smaller', 'idea', 'code', { priority: 5 }), T(20, 'Kickstarter reward tiers', 'idea', 'marketing'),
    ],
    hands: [
      { id: 'T-12', phase: 'working', try: 1, doing: 'Editing tests/keypad.test.js', tool: 'Edit', toolCalls: 14, since: now - 9 * m, agent: 'bolt' },
      { id: 'T-15', phase: 'working', try: 1, doing: 'Reading the KiCad netlist', tool: 'Read', toolCalls: 4, since: now - 3 * m, agent: 'rivet' },
      { id: 'T-9', phase: 'checking', try: 1, doing: 'Running npm test -- abs', tool: 'Bash', toolCalls: 22, since: now - 1 * m, agent: 'hawk' },
    ],
    chat: {
      fern: [{ at: now - 40 * m, from: 'system', text: 'Fern started planning (cycle 14).' }, { at: now - 39 * m, from: 'you', text: 'Focus on keypad tests first please.' }, { at: now - 38 * m, from: 'agent', text: 'Got it. I moved T-12 and T-16 to the front and held the logo idea.' }],
      bolt: [{ at: now - 20 * m, from: 'system', text: 'Bolt picked up T-12.' }, { at: now - 8 * m, from: 'you', text: 'Use the same style as the row 1 tests.' }, { at: now - 7 * m, from: 'agent', text: 'Will do. I am copying the table layout from tests/row1.test.js.' }],
      poppy: [{ at: now - 2 * m, from: 'you', text: 'Can the posts be shorter?' }],
    },
    lines: {
      fern: ['Splitting the keypad work into 3 small tasks', 'Next up: T-16 keypad row 3', 'Week is 46% used, pace is fine'],
      bolt: ['Found the old row 1 tests, copying their layout', 'x^3 of -2 should be -8, adding that case', '14 steps in, tests look good'],
      rivet: ['Reading the stage-6 camera pins', 'GPIO 12 and 13 are free on v15'],
      poppy: ['Thinking about a shorter hook…'], sage: ['Two LCD suppliers look cheaper'], hawk: ['Running the abs tests now', 'All 3 checks passing so far'],
    },
    timeline: [],
  };
  const ev = (minAgo, agent, kind, text) => D.timeline.push({ at: now - minAgo * m, agent, kind, text });
  ev(55, 'fern', 'plan', 'Planned 4 tasks from the goals (cycle 13).');
  ev(41, 'poppy', 'done', 'T-8 passed: 3 post drafts are in the harvest.');
  ev(38, 'fern', 'chat', 'Answered you: moved keypad tests to the front.');
  ev(30, 'rivet', 'done', 'T-7 passed all checks.');
  ev(26, 'bolt', 'fail', 'T-6 failed twice and needs you.');
  ev(20, 'bolt', 'start', 'Picked up T-12 Unit tests for the x^3 key.');
  ev(12, 'rivet', 'start', 'Picked up T-15 Camera pin note for stage 6.');
  ev(4, 'hawk', 'check', 'Started checking T-9 Abs key edge cases.');
  ev(1, 'fern', 'plan', 'Planning the next cycle.');
}
function demoSnap() {
  if (!D) demoInit();
  const now = Date.now(), m = 60000, el8 = (now - D.start) / 1000;
  const tick = Math.floor(el8 / 8);
  const line = id => { const l = D.lines[id] || []; return l.length ? l[tick % l.length] : ''; };
  const hands = D.running ? D.hands.map(h => ({ ...h, toolCalls: h.toolCalls + Math.floor(el8 / 5), title: (D.tasks.find(t => t.id === h.id) || {}).title || h.id })) : [];
  const handOf = id => hands.find(h => h.agent === id);
  const st = (id, fallback) => { const h = handOf(id); return h ? (h.phase === 'checking' ? 'checking' : 'working') : fallback; };
  const off = !D.running;
  const agents = DEFAULT_ROSTER.concat([{ id: 'clover', name: 'Clover', title: 'Night-shift marketer', role: 'worker', color: '#aed581', home: 'market', kinds: ['marketing'] }]).map(a => {
    const h = handOf(a.id);
    const want = { fern: 'planning', poppy: 'chatting', sage: 'idle', clover: 'asleep', penny: 'writing' }[a.id] || 'idle';
    const status = off ? 'asleep' : st(a.id, want);
    return { ...a, status, taskId: h ? h.id : null, taskTitle: h ? h.title : null, doing: h ? h.doing : null, since: h ? new Date(h.since).toISOString() : null,
      lastLine: off ? '' : line(a.id), nextCycleAt: new Date(now + (({ fern: 12, bolt: 3, rivet: 7, poppy: 18, sage: 25, hawk: 1, clover: 240 })[a.id] || 30) * m - (el8 * 1000 % m)).toISOString(),
      cycleNote: a.id === 'fern' ? 'Plans every 30 min' : 'Checks in every 15 min' };
  });
  const tasks = D.tasks.map(t => ({ ...t }));
  const attention = [];
  for (const t of tasks) if (t.status === 'harvest') attention.push({ kind: 'harvest', taskId: t.id, text: `${t.title}: passed, ready for you to review` });
  for (const t of tasks) if (t.status === 'needs-you') attention.push({ kind: 'needs-you', taskId: t.id, text: `${t.title}: failed twice, needs a decision` });
  if (off) attention.push({ kind: 'off', text: 'The farm is off.' });
  attention.unshift({ kind: 'biz', level: 'urgent', go: 'inbox', text: '2 customer replies drafted: read and approve' });
  attention.push({ kind: 'biz', level: 'todo', go: 'orders', text: '3 calculators to ship in 2 orders (oldest 9 days)' });
  return {
    farmDir: 'C:\\Users\\you\\ProoflineFarm (demo)', manager: { running: D.running, ours: true, pid: 1 }, wantRunning: D.running, paused: D.paused, loginNeeded: false,
    startWithWindows: D.startWithWindows,
    feed: { at: now, reason: '', online: true, chatting: false, limitUntil: null,
      usage: { weekly: 46, five: 31, line: 100 - D.config.weeklyReserve, pace: 52, reserve: D.config.weeklyReserve, weeklyResetsAt: new Date(now + 3.4 * 86400000).toISOString(), readingAgeH: 0.3 },
      farmhands: hands, counts: {} },
    usage: null, summary: 'Demo summary\n\nThis week: 5 tasks done, 2 in the harvest, 1 needs you.\nWeekly use 46% (farm stops at 70%).',
    config: { ...D.config }, tasks, agents,
    foreman: { nextCycleAt: agents[0].nextCycleAt, lastCycleAt: new Date(now - 18 * m).toISOString(), lastNote: 'Keypad tests come first this week. I held the boot-logo idea until the tests pass. T-6 needs you: pick 9 or 10 digits for rounding.' },
    attention, goals: D.goals, timeline: D.timeline.slice().sort((a, b) => toMs(b.at) - toMs(a.at)),
  };
}
const demoApi = {
  snapshot: async () => demoSnap(),
  start: async () => { D.running = true; }, stop: async () => { D.running = false; }, pause: async on => { D.paused = !!on; },
  stopTask: async id => { D.hands = D.hands.filter(h => h.id !== id); const t = D.tasks.find(x => x.id === id); if (t) t.status = 'ready'; toast('Demo: stopped ' + id); },
  setStatus: async (id, s) => { const t = D.tasks.find(x => x.id === id); if (t) t.status = s; },
  addTask: async t => { const id = `T-${21 + D.tasks.length}`; const ready = /^(cmd|judge)\s*:/im.test(t.doneWhen || ''); D.tasks.push({ id, title: t.title || 'New task', status: ready ? 'ready' : 'idea', priority: Number(t.priority) || 3, kind: t.kind, tries: 0, last: '', hasField: false, agent: t.agent || null }); return { id, status: ready ? 'ready' : 'idea' }; },
  openField: async id => toast(`Demo: would open the folder of ${id}`), openTask: async id => toast(`Demo: would open ${id}.md`), openFolder: async s => toast(`Demo: would open the ${s || 'farm'} folder`),
  refreshSummary: async () => demoSnap().summary, setStartWithWindows: async on => (D.startWithWindows = !!on),
  setConfig: async c => { Object.assign(D.config, c); },
  sendMessage: async (id, text) => {
    (D.chat[id] = D.chat[id] || []).push({ at: Date.now(), from: 'you', text });
    setTimeout(() => D.chat[id].push({ at: Date.now(), from: 'agent', text: 'Thanks! (demo reply) I will take that into account at my next step.' }), 1500);
    return true;
  },
  chatLog: async id => (D.chat[id] || []).slice(),
  getGoals: async () => D.goals, setGoals: async t => { D.goals = t; return true; }, planNow: async () => { D.timeline.push({ at: Date.now(), agent: 'fern', kind: 'plan', text: 'You asked Fern to plan now.' }); return true; },
};

// ---------- turn a snapshot into what the screen needs ----------
let snap = null, M = null;
const observed = [];   // timeline we build ourselves when the backend has none
let prevAgentState = null;
function deriveAgents(s) {
  const off = !s.manager || !s.manager.running;
  const roster = DEFAULT_ROSTER.map(a => ({ ...a, status: off || s.paused ? 'asleep' : 'idle', taskId: null, taskTitle: null, doing: null, since: null, lastLine: '' }));
  const byId = Object.fromEntries(roster.map(a => [a.id, a]));
  const free = a => a && ['idle', 'asleep'].includes(a.status);
  for (const h of (s.feed && s.feed.farmhands) || []) {
    const task = (s.tasks || []).find(t => t.id === h.id) || {};
    let ag = h.agent && byId[h.agent];
    if (!free(ag)) {
      if (h.phase === 'checking') ag = byId.hawk;
      else if (task.kind === 'marketing') ag = byId.poppy;
      else if (task.kind === 'research') ag = byId.sage;
      else ag = [byId.bolt, byId.rivet].find(free);
      if (!free(ag)) ag = roster.find(a => a.role === 'worker' && free(a));
    }
    if (!ag) continue;
    Object.assign(ag, { status: h.phase === 'checking' ? 'checking' : 'working', taskId: h.id, taskTitle: h.title || task.title, doing: h.doing, since: h.since, lastLine: h.doing || '' });
  }
  return roster;
}
function deriveAttention(s) {
  const out = [];
  if (s.loginNeeded) out.push({ kind: 'login', text: 'Claude needs a sign-in before the farm can work.' });
  if (!s.manager || !s.manager.running) out.push({ kind: 'off', text: 'The farm is off.' });
  if (s.feed && s.feed.limitUntil) out.push({ kind: 'limit', text: `Usage limit reached. The farm waits until ${when(s.feed.limitUntil)}.` });
  for (const t of s.tasks || []) if (t.status === 'harvest') out.push({ kind: 'harvest', taskId: t.id, text: `${t.title}: passed, ready for you to review` });
  for (const t of s.tasks || []) if (t.status === 'needs-you') out.push({ kind: 'needs-you', taskId: t.id, text: `${t.title}: needs you` });
  return out;
}
function statusOf(s) {
  const f = s.feed || {};
  const fresh = f.at && Date.now() - toMs(f.at) < 60000;
  const running = s.manager && s.manager.running && fresh;
  let cls = 'off', text = 'Farm is off. Press "Turn farm on" to start it.';
  if (s.loginNeeded) { cls = 'alert'; text = 'Claude needs a sign-in (see the red box)'; }
  else if (s.manager && s.manager.running && !fresh) { cls = 'wait'; text = 'Starting the farm…'; }
  else if (running && s.paused) { cls = 'wait'; text = 'Paused. Press Resume to continue.'; }
  else if (running && (f.farmhands || []).length) { cls = 'ok'; text = `${f.farmhands.length} farmhand${f.farmhands.length > 1 ? 's' : ''} working${f.chatting ? ' (you are chatting, so only 1)' : ''}`; }
  else if (running) { cls = 'wait'; text = f.reason ? `Waiting: ${f.reason}` : 'Waiting for ready tasks'; }
  return { cls, text };
}
function normalize(s) {
  s.tasks = s.tasks || []; s.config = s.config || { weeklyReserve: 30, maxFarmhands: 3 }; s.manager = s.manager || { running: false };
  const agents = Array.isArray(s.agents) && s.agents.length ? s.agents.map(a => ({ ...a })) : deriveAgents(s);
  const foremanAgent = agents.find(a => a.role === 'foreman') || agents[0];
  const foreman = s.foreman || null;
  if (foremanAgent && !foremanAgent.nextCycleAt && foreman && foreman.nextCycleAt) foremanAgent.nextCycleAt = foreman.nextCycleAt;
  // observe changes for our own timeline
  const now = Date.now();
  if (prevAgentState && !Array.isArray(s.timeline)) {
    for (const a of agents) {
      const p = prevAgentState[a.id];
      if (p && (p.status !== a.status || p.taskId !== a.taskId)) {
        const text = a.status === 'working' ? `Started work on ${a.taskId}${a.taskTitle ? ' ' + a.taskTitle : ''}.` : a.status === 'checking' ? `Started checking ${a.taskId}.` : `Is now ${STATUS_LABEL[a.status] ? STATUS_LABEL[a.status].toLowerCase() : a.status}.`;
        observed.unshift({ at: now, agent: a.id, kind: a.status, text });
      }
    }
    observed.splice(200);
  }
  prevAgentState = Object.fromEntries(agents.map(a => [a.id, { status: a.status, taskId: a.taskId }]));
  const st = statusOf(s);
  const f = s.feed || {};
  return {
    s, agents, foreman, foremanAgent, statusCls: st.cls, statusText: st.text,
    running: !!s.manager.running, paused: !!s.paused,
    attention: Array.isArray(s.attention) ? s.attention : deriveAttention(s),
    usage: f.usage || {}, hands: f.farmhands || [],
    timeline: Array.isArray(s.timeline) ? s.timeline.slice().sort((a, b) => (toMs(b.at) || 0) - (toMs(a.at) || 0)) : observed,
    timelineIsOurs: !Array.isArray(s.timeline),
    goals: typeof s.goals === 'string' ? s.goals : null,
  };
}
const agentById = id => M && M.agents.find(a => a.id === id);
const agentName = id => (agentById(id) || {}).name || id || '';

// ---------- HUD ----------
function renderHud() {
  const s = M.s;
  $('status').className = `status ${M.statusCls}`;
  $('statusText').textContent = M.statusText;
  const fa = M.foremanAgent;
  const next = (M.foreman && M.foreman.nextCycleAt) || (fa && fa.nextCycleAt);
  const left = inTime(next);
  $('cycleText').textContent = left ? `${fa ? fa.name : 'Foreman'} plans again in ${left}` : M.running ? 'Foreman plan time not known yet' : '';
  $('pauseBtn').textContent = s.paused ? 'Resume' : 'Pause';
  $('pauseBtn').hidden = !s.manager.running;
  $('powerBtn').textContent = s.manager.running ? 'Turn farm off' : 'Turn farm on';
  $('powerBtn').classList.toggle('primary', !s.manager.running);
  $('loginBanner').hidden = !s.loginNeeded;
  document.body.classList.toggle('demo', demo);
  // usage (HUD + silo)
  const u = M.usage;
  const line = u.line ?? (100 - (s.config.weeklyReserve || 30));
  for (const [bar, v] of [['weeklyBar', u.weekly], ['fiveBar', u.five], ['sWeeklyBar', u.weekly], ['sFiveBar', u.five]]) $(bar).style.width = v == null ? '0%' : `${Math.min(100, v)}%`;
  $('weeklyText').textContent = u.weekly == null ? '–' : `${u.weekly}%`;
  $('fiveText').textContent = u.five == null ? '–' : `${u.five}%`;
  $('sWeeklyText').textContent = u.weekly == null ? 'no reading yet' : `${u.weekly}% used`;
  $('sFiveText').textContent = u.five == null ? 'no reading yet' : `${u.five}% used`;
  for (const id of ['lineMark', 'sLineMark']) $(id).style.left = `calc(${line}% - 1px)`;
  for (const id of ['paceMark', 'sPaceMark']) { $(id).hidden = u.pace == null; if (u.pace != null) $(id).style.left = `calc(${u.pace}% - 1px)`; }
  const parts = [];
  if (u.weeklyResetsAt) parts.push(`Week resets ${when(u.weeklyResetsAt)}.`);
  parts.push(`The farm stops at ${line}%, so ${100 - line}% stays yours.`);
  if (u.readingAgeH != null) parts.push(`Reading is ${u.readingAgeH < 1 ? Math.round(u.readingAgeH * 60) + ' min' : u.readingAgeH.toFixed(1) + ' h'} old.`);
  if (s.feed && s.feed.reason && s.manager.running) parts.push(`Farm manager: ${s.feed.reason}`);
  $('usageNote').textContent = parts.join(' ');
  $('usageBox').title = `Weekly ${u.weekly ?? '?'}% used, farm stops at ${line}%${u.pace != null ? `, today's line ${u.pace}%` : ''}. Click for details.`;
  // ticker
  const tk = $('ticker'); tk.replaceChildren();
  const lines = M.timeline.slice(0, 3);
  if (!lines.length) tk.append(el('div', 'tk-line muted', M.running ? 'No farm events yet.' : 'The farm is quiet.'));
  for (const e of lines) {
    const row = el('div', 'tk-line');
    const a = agentById(e.agent);
    row.append(el('span', 'tk-time', hhmm(e.at)));
    const who = el('span', 'tk-who', a ? a.name : (e.agent || 'farm')); if (a) who.style.color = a.color; row.append(who);
    row.append(el('span', 'tk-text', e.text));
    tk.append(row);
  }
  // badges
  const ready = s.tasks.filter(t => t.status === 'ready').length;
  $('boardBadge').hidden = !ready; $('boardBadge').textContent = ready;
  const n = M.attention.length;
  $('mailBadge').hidden = !n; $('mailBadge').textContent = n;
}
function tickClock() {
  $('clock').textContent = new Date().toLocaleString(undefined, { weekday: 'short', hour: 'numeric', minute: '2-digit' }) + (demo ? '  ·  DEMO' : '');
}

// ---------- roster (mission control) ----------
const rosterEls = new Map();
function renderRoster() {
  const box = $('roster');
  const ids = M.agents.map(a => a.id).join();
  if (box.dataset.ids !== ids) {
    box.replaceChildren(); rosterEls.clear(); box.dataset.ids = ids;
    for (const a of M.agents) {
      const row = el('button', 'r-row');
      row.addEventListener('click', () => go(`agent=${encodeURIComponent(a.id)}`));
      const av = el('canvas', 'avatar');
      const txt = el('div', 'r-text');
      const top = el('div', 'r-top'); const dot = el('i', 'r-dot'); const name = el('b', 'r-name'); const timer = el('span', 'r-timer');
      top.append(dot, name, timer);
      const role = el('div', 'r-role'); const stl = el('div', 'r-status');
      txt.append(top, role, stl);
      row.append(av, txt);
      box.append(row);
      rosterEls.set(a.id, { row, av, dot, name, timer, role, stl, color: null });
    }
  }
  for (const a of M.agents) {
    const r = rosterEls.get(a.id); if (!r) continue;
    if (r.color !== a.color) { r.color = a.color; setTimeout(() => Game.drawAvatar(r.av, a), 0); }
    r.dot.style.background = a.color; r.dot.className = `r-dot s-${a.status}`;
    r.name.textContent = String(a.name || a.id).toUpperCase();
    const left = inTime(a.nextCycleAt);
    r.timer.textContent = left ? `next cycle ${left === 'any moment' ? 'now' : 'in ' + left}` : '';
    r.role.textContent = a.title || a.role || '';
    r.stl.textContent = statusLine(a);
    r.stl.className = `r-status s-${a.status}`;
    r.row.classList.toggle('sel', openAgent === a.id);
    r.row.title = `${a.name}, ${a.title || ''}: ${statusLine(a)}${a.taskTitle ? ' (' + a.taskTitle + ')' : ''}${a.cycleNote ? '. ' + a.cycleNote : ''}`;
  }
  $('rosterCount').textContent = `${M.agents.filter(a => !['idle', 'asleep'].includes(a.status)).length}/${M.agents.length} busy`;
}

// ---------- attention box + toasts ----------
const attnKey = a => `${a.kind}:${a.taskId || a.text}`;
let seenAttn = null;
function attnLabel(a) { return { harvest: 'HARVEST', 'needs-you': 'NEEDS YOU', login: 'SIGN IN', limit: 'LIMIT', off: 'FARM OFF', biz: a.level === 'urgent' ? 'BUSINESS!' : 'BUSINESS' }[a.kind] || String(a.kind || 'NOTE').toUpperCase(); }
const attnText = a => { const t = String(a.text || ''); return a.taskId && !t.includes(a.taskId) ? `${a.taskId} · ${t}` : t; };
function attnGo(a) { if (a.kind === 'biz') go(`biz=${a.go || 'today'}`); else if (a.kind === 'limit') go('silo'); else go('mail'); }
function renderAttention() {
  const box = $('attnList'); box.replaceChildren();
  $('attnCount').textContent = M.attention.length ? String(M.attention.length) : '';
  $('attnPanel').classList.toggle('has', M.attention.length > 0);
  if (!M.attention.length) box.append(el('div', 'attn-empty', 'All clear. Nothing waits for you.'));
  for (const a of M.attention.slice(0, 6)) {
    const row = el('button', `attn-item k-${a.kind}`);
    row.append(el('span', 'attn-kind', attnLabel(a)), el('span', 'attn-text', attnText(a)));
    row.addEventListener('click', () => attnGo(a));
    box.append(row);
  }
  if (M.attention.length > 6) box.append(el('div', 'attn-empty', `+ ${M.attention.length - 6} more in the mailbox`));
  // toasts for new items
  const keys = new Set(M.attention.map(attnKey));
  if (seenAttn === null) { if (M.attention.length) toast(`${M.attention.length} thing${M.attention.length > 1 ? 's' : ''} wait${M.attention.length > 1 ? '' : 's'} for you in the mailbox`, () => go('mail')); }
  else for (const a of M.attention) if (!seenAttn.has(attnKey(a))) toast(`${attnLabel(a)}: ${attnText(a)}`, () => attnGo(a));
  seenAttn = keys;
}
function toast(text, onClick) {
  const t = el('button', 'toast', short(text, 140));
  t.addEventListener('click', () => { t.remove(); onClick && onClick(); });
  $('toasts').append(t);
  while ($('toasts').children.length > 3) $('toasts').firstChild.remove();
  setTimeout(() => t.classList.add('out'), 6000);
  setTimeout(() => t.remove(), 6600);
}

// ---------- world model for the canvas ----------
function pushGame() {
  const s = M.s;
  const hands = M.hands.map(h => { const t = s.tasks.find(x => x.id === h.id) || {}; return { id: h.id, title: h.title || t.title, kind: t.kind, toolCalls: h.toolCalls, phase: h.phase }; });
  // tasks that agents say they work on but the feed misses still get a plot
  for (const a of M.agents) if (['working', 'checking'].includes(a.status) && a.taskId && !hands.find(h => h.id === a.taskId)) {
    const t = s.tasks.find(x => x.id === a.taskId) || {};
    hands.push({ id: a.taskId, title: a.taskTitle || t.title, kind: t.kind, toolCalls: 0, phase: a.status === 'checking' ? 'checking' : 'working' });
  }
  Game.setModel({
    agents: M.agents, plots: hands,
    harvest: s.tasks.filter(t => t.status === 'harvest').map(t => ({ id: t.id, title: t.title, kind: t.kind })),
    readyCount: s.tasks.filter(t => t.status === 'ready').length,
    mail: M.attention.length, usage: { weekly: M.usage.weekly, line: M.usage.line ?? (100 - (s.config.weeklyReserve || 30)) },
    asleep: !M.running || M.paused,
  });
}

// ---------- tooltips ----------
const HOME_INFO = {
  farmhouse: ['Farmhouse', 'The Foreman\'s office. Plans the work from your goals.'],
  workshop: ['Workshop', 'Where the coders live.'], library: ['Library', 'Where the researcher reads and compares.'],
  market: ['Market', 'Where the marketer drafts posts (drafts only, nothing is sent).'], tower: ['Watchtower', 'The inspector checks every task before it reaches the harvest.'],
  shed: ['Seed shed', 'The task board: every task, ready or not.'], silo: ['Silo', 'Usage this week. Grain = used, red line = where the farm stops.'],
  postoffice: ['Post office', 'Penny drafts replies to customer e-mail here. Click to open the inbox.'],
};
function tipFor(hit) {
  if (!hit) return null;
  if (hit.type === 'agent') {
    const a = agentById(hit.id); if (!a) return null;
    return [`${a.name} · ${a.title || a.role}`, `${statusLine(a)}${a.taskTitle ? ': ' + a.taskTitle : ''}${a.doing ? '\n' + a.doing : ''}\nClick to talk.`];
  }
  if (hit.type === 'building') {
    const [t, d] = HOME_INFO[hit.id] || [hit.id, ''];
    const who = M.agents.filter(a => a.home === hit.id).map(a => a.name);
    if (hit.id === 'shed') return [t, `${d}\n${M.s.tasks.filter(x => x.status === 'ready').length} ready to plant. Click to open.`];
    if (hit.id === 'silo') return [t, `${d}\nWeek: ${M.usage.weekly ?? '?'}% used.`];
    return [t, `${d}${who.length ? '\nLives here: ' + who.join(', ') : ''}\nClick to look inside.`];
  }
  if (hit.type === 'mailbox') return ['Mailbox', M.attention.length ? `${M.attention.length} waiting for you. Click to open.` : 'Empty. Nothing waits for you.'];
  if (hit.type === 'baskets') return ['Harvest baskets', `${M.s.tasks.filter(t => t.status === 'harvest').length} task(s) passed and wait for your review.`];
  if (hit.type === 'plot') {
    if (!hit.taskId) return ['Empty plot', 'A running task grows here.'];
    const h = M.hands.find(x => x.id === hit.taskId) || {};
    const t = M.s.tasks.find(x => x.id === hit.taskId) || {};
    const who = M.agents.filter(a => a.taskId === hit.taskId).map(a => `${a.name} (${a.status})`).join(', ');
    return [`${hit.taskId} · ${h.title || t.title || ''}`, `${who || 'In the field'}\n${h.toolCalls || 0} steps · try ${h.try || 1}${h.since ? ' · ' + ago(h.since) : ''}`];
  }
  if (hit.type === 'pond') return ['The pond', 'Nothing to do here. The duck says hi.'];
  return null;
}
function onHover(hit, x, y) {
  const tip = $('tip');
  const t = M && tipFor(hit);
  if (!t) { tip.hidden = true; return; }
  $('tipTitle').textContent = t[0]; $('tipText').textContent = t[1];
  tip.hidden = false;
  const w = tip.offsetWidth, h = tip.offsetHeight;
  tip.style.left = `${Math.min(window.innerWidth - w - 8, x + 16)}px`;
  tip.style.top = `${Math.min(window.innerHeight - h - 8, y + 18)}px`;
}
function onWorldClick(hit) {
  $('tip').hidden = true;
  if (hit.type === 'agent') go(`agent=${encodeURIComponent(hit.id)}`);
  else if (hit.type === 'mailbox' || hit.type === 'baskets') go('mail');
  else if (hit.type === 'plot') {
    const a = M.agents.find(x => x.taskId === hit.taskId && x.status === 'working') || M.agents.find(x => x.taskId === hit.taskId);
    if (a) go(`agent=${encodeURIComponent(a.id)}`); else if (hit.taskId) go('board');
  } else if (hit.type === 'building') {
    if (hit.id === 'shed') go('board');
    else if (hit.id === 'silo') go('silo');
    else if (hit.id === 'postoffice') go('biz=inbox');
    else go(`room=${hit.id}`);
  }
}

// ---------- the agent drawer ----------
let openAgent = null, chatTimer = null, chatSig = '';
function openDrawer(id) {
  if (!agentById(id) && M) { go(''); return; }
  const fresh = openAgent !== id;
  openAgent = id;
  $('drawer').hidden = false;
  Game.setSelected(id);
  if (fresh) {
    chatSig = ''; $('chat').replaceChildren(el('div', 'chat-sys', 'Loading the talk…'));
    $('chatInput').value = '';
    const a = agentById(id);
    if (a) setTimeout(() => Game.drawAvatar($('dAvatar'), a), 0);
    if (a && a.role === 'foreman') drawerGoals.load();
    loadChat();
    clearInterval(chatTimer); chatTimer = setInterval(loadChat, 2000);
    setTimeout(() => { $('chatInput').focus({ preventScroll: true }); $('drawer').scrollTop = 0; }, 50);
  }
  renderDrawer();
}
function closeDrawer() {
  if (!openAgent) return;
  openAgent = null; $('drawer').hidden = true; Game.setSelected(null);
  clearInterval(chatTimer); chatTimer = null;
}
function renderDrawer() {
  if (!openAgent || !M) return;
  const a = agentById(openAgent); if (!a) return;
  $('dName').textContent = a.name; $('dTitle').textContent = a.title || a.role || '';
  $('dName').style.color = a.color;
  const chip = $('dStatus'); chip.textContent = STATUS_LABEL[a.status] || a.status; chip.className = `chip s-${a.status}`;
  const task = $('dTask'); task.replaceChildren();
  if (a.taskId) {
    task.append(el('div', 'd-task-id', `${a.taskId} · ${a.status === 'checking' ? 'checking' : 'working'} for ${ago(a.since) || 'a moment'}`));
    task.append(el('div', 'd-task-title', a.taskTitle || ''));
    if (a.doing) task.append(el('div', 'd-doing', a.doing));
  } else task.append(el('div', 'd-task-id', statusLine(a)));
  if (a.lastLine) task.append(el('div', 'd-line', `“${a.lastLine}”`));
  const left = inTime(a.nextCycleAt);
  $('dCycle').textContent = [left ? `Next cycle in ${left}.` : '', a.cycleNote || ''].filter(Boolean).join(' ');
  const act = $('dActions'); act.replaceChildren();
  if (a.taskId && ['working', 'checking'].includes(a.status)) act.append(btn(`Stop ${a.taskId}`, () => call('stopTask', a.taskId), 'gbtn danger'));
  if (a.role !== 'inspector') act.append(btn(`Give ${a.name} a task`, async () => go(`plant&for=${encodeURIComponent(a.id)}`), 'gbtn primary'));
  if (a.role === 'foreman') act.append(btn('Command Center', async () => go('ops'), 'gbtn'));
  const isF = a.role === 'foreman';
  $('dForeman').hidden = !isF;
  if (isF) $('dNote').textContent = (M.foreman && M.foreman.lastNote) || 'No note yet.';
  $('chatInput').placeholder = `Tell ${a.name} something…`;
  $('chatHint').textContent = `If ${a.name} is working, ${a.name} reads it at the next step; if idle, ${a.name} answers and may plant tasks. Enter sends, Shift+Enter makes a new line.`;
}
async function loadChat() {
  if (!openAgent) return;
  const id = openAgent;
  const log = await call('chatLog', id);
  if (id !== openAgent) return;
  const box = $('chat');
  if (!Array.isArray(log)) {
    if (chatSig !== 'none') { chatSig = 'none'; box.replaceChildren(el('div', 'chat-sys', 'Talking needs the newer farm backend. It is on its way.')); }
    return;
  }
  const sig = `${log.length}|${log.length ? log[log.length - 1].at : ''}`;
  if (sig === chatSig) return;
  chatSig = sig;
  const nearBottom = box.scrollHeight - box.scrollTop - box.clientHeight < 40;
  box.replaceChildren();
  if (!log.length) box.append(el('div', 'chat-sys', `No talk yet. Say hello to ${agentName(id)}.`));
  for (const m of log.slice(-200)) {
    const from = ['you', 'agent', 'system'].includes(m.from) ? m.from : 'system';
    const row = el('div', `msg ${from}`);
    if (from !== 'system') row.append(el('div', 'msg-who', `${from === 'you' ? 'You' : agentName(id)} · ${hhmm(m.at)}`));
    row.append(el('div', 'msg-text', m.text));
    box.append(row);
  }
  if (nearBottom || box.dataset.first !== id) { box.scrollTop = box.scrollHeight; box.dataset.first = id; }
}
$('chatForm').addEventListener('submit', async e => {
  e.preventDefault();
  const text = $('chatInput').value.trim();
  if (!text || !openAgent) return;
  $('chatInput').value = '';
  const r = await call('sendMessage', openAgent, text);
  if (r === undefined && typeof api.sendMessage !== 'function') toast('Talking needs the newer farm backend.');
  chatSig = ''; loadChat();
});
$('chatInput').addEventListener('keydown', e => { if (e.key === 'Enter' && !e.shiftKey) { e.preventDefault(); $('chatForm').requestSubmit(); } });

// ---------- goals editor (used in Fern's drawer and the Command Center) ----------
function goalsEditor(container) {
  const ta = el('textarea', 'goals mono'); ta.rows = 7; ta.placeholder = 'What should the farm work toward? One goal per line.';
  const save = el('button', 'gbtn primary', 'Save goals');
  const plan = el('button', 'gbtn', 'Plan now');
  const msg = el('span', 'muted small');
  const row = el('div', 'row'); row.append(save, plan, msg);
  container.replaceChildren(ta, row);
  let dirty = false;
  ta.addEventListener('input', () => { dirty = true; msg.textContent = 'Not saved yet'; });
  save.addEventListener('click', async () => {
    save.disabled = true;
    const r = await call('setGoals', ta.value);
    save.disabled = false;
    if (r === undefined && typeof api.setGoals !== 'function') msg.textContent = 'Saving goals needs the newer farm backend.';
    else { dirty = false; msg.textContent = 'Saved. The Foreman uses them at the next plan.'; }
  });
  plan.addEventListener('click', async () => {
    plan.disabled = true;
    const r = await call('planNow');
    plan.disabled = false;
    msg.textContent = r === undefined && typeof api.planNow !== 'function' ? '"Plan now" needs the newer farm backend.' : 'Asked. The Foreman plans at the next tick.';
    refresh();
  });
  return {
    async load() {
      if (dirty) return;
      let g = await call('getGoals');
      if (typeof g !== 'string') g = M && M.goals;
      if (typeof g === 'string') { ta.value = g; msg.textContent = ''; } else { msg.textContent = 'Goals need the newer farm backend.'; }
    },
  };
}
const drawerGoals = goalsEditor($('dGoals'));
const opsGoals = goalsEditor($('opsGoals'));

// ---------- task board ----------
function taskCard(t) {
  const c = el('div', `tcard ${t.status}`);
  c.append(el('div', 'id', `${t.id} · ${t.kind || 'code'} · priority ${t.priority ?? '-'}${t.from ? ` · follow-up of ${t.from}` : ''}${t.agent ? ` · ${agentName(t.agent)}` : ''}`));
  c.append(el('div', 'title', t.title));
  if (t.last) c.append(el('div', 'last', t.last));
  const a = el('div', 'actions');
  if (t.hasField && ['harvest', 'needs-you', 'done'].includes(t.status)) a.append(btn('Open folder', () => call('openField', t.id)));
  if (t.status === 'needs-you') a.append(btn('Try again', () => call('setStatus', t.id, 'ready')));
  if (t.status === 'idea') a.append(btn('Make ready', () => call('setStatus', t.id, 'ready')));
  if (t.status === 'ready') a.append(btn('Hold', () => call('setStatus', t.id, 'idea')));
  if (['working', 'checking'].includes(t.status)) a.append(btn('Stop', () => call('stopTask', t.id), 'gbtn small danger'));
  if (['needs-you', 'harvest'].includes(t.status)) a.append(btn('Mark done', () => call('setStatus', t.id, 'done')));
  a.append(btn('Task file', () => call('openTask', t.id)));
  c.append(a);
  return c;
}
const COLUMNS = [
  ['ready', 'Ready', 'Waiting for a farmhand'],
  ['working', 'In the field', 'Being worked or checked'],
  ['harvest', 'Harvest', 'Passed: review and commit'],
  ['needs-you', 'Needs you', 'Failed twice or has a question'],
  ['idea', 'Ideas', 'Not started (no Done-when, or on hold)'],
  ['done', 'Done', 'Committed by you'],
];
function renderBoard() {
  const board = $('board');
  const scrolls = [...board.querySelectorAll('.col-body')].map(c => c.scrollTop);
  board.replaceChildren();
  COLUMNS.forEach(([key, name, hint], i) => {
    const items = M.s.tasks.filter(t => key === 'working' ? ['working', 'checking'].includes(t.status) : t.status === key)
      .sort((a, b) => (a.priority ?? 9) - (b.priority ?? 9) || String(a.id).localeCompare(String(b.id), undefined, { numeric: true }));
    const col = el('div', `col c-${key}`);
    const h = el('h3'); h.append(el('span', null, name), el('span', 'cnt', String(items.length)));
    const body = el('div', 'col-body');
    for (const t of key === 'done' ? items.slice(-15) : items) body.append(taskCard(t));
    if (!items.length) body.append(el('div', 'empty', 'Nothing here'));
    col.append(h, el('div', 'hint', hint), body);
    board.append(col);
    if (scrolls[i]) body.scrollTop = scrolls[i];
  });
}

// ---------- command center ----------
let opsTab = 'mission';
function showOpsTab(tab) {
  opsTab = ['mission', 'tasks', 'timeline', 'agents'].includes(tab) ? tab : 'mission';
  document.querySelectorAll('#opsTabs button').forEach(b => b.classList.toggle('active', b.dataset.otab === opsTab));
  document.querySelectorAll('#ov-ops .otab-body').forEach(p => { p.hidden = p.dataset.pane !== opsTab; });
  if (opsTab === 'mission') opsGoals.load();
  renderOps();
}
document.querySelectorAll('#opsTabs button').forEach(b => b.addEventListener('click', () => go(b.dataset.otab === 'tasks' ? 'board' : `ops=${b.dataset.otab}`)));
function renderOps() {
  if ($('ov-ops').hidden || !M) return;
  if (opsTab === 'tasks') renderBoard();
  if (opsTab === 'mission') {
    $('opsNote').textContent = (M.foreman && M.foreman.lastNote) || 'No note from the Foreman yet.';
    const f = M.foreman || {};
    $('opsCycle').textContent = [f.lastCycleAt ? `Last plan ${when(f.lastCycleAt)}.` : '', inTime(f.nextCycleAt || (M.foremanAgent || {}).nextCycleAt) ? `Next plan in ${inTime(f.nextCycleAt || M.foremanAgent.nextCycleAt)}.` : ''].join(' ');
  }
  if (opsTab === 'timeline') {
    const box = $('timeline'); box.replaceChildren();
    if (M.timelineIsOurs) box.append(el('div', 'tl-note', 'The farm backend has no timeline yet, so this shows changes this window has seen since it opened.'));
    if (!M.timeline.length) box.append(el('div', 'tl-row muted', 'Nothing has happened yet.'));
    for (const e of M.timeline.slice(0, 300)) {
      const r = el('div', `tl-row k-${String(e.kind || '').replace(/[^a-z-]/gi, '')}`);
      const a = agentById(e.agent);
      r.append(el('span', 'tl-time', new Date(toMs(e.at) || Date.now()).toLocaleString(undefined, { weekday: 'short', hour: '2-digit', minute: '2-digit' })));
      const who = el('span', 'tl-who', (a ? a.name : e.agent || 'farm').toUpperCase()); if (a) who.style.color = a.color; r.append(who);
      r.append(el('span', 'tl-kind', `[${e.kind || 'note'}]`), el('span', 'tl-text', e.text));
      box.append(r);
    }
  }
  if (opsTab === 'agents') {
    const box = $('agentsTable'); box.replaceChildren();
    const head = el('div', 'at-row at-head');
    ['', 'Agent', 'Role', 'Status', 'Task', 'Next cycle', ''].forEach(h => head.append(el('span', null, h)));
    box.append(head);
    for (const a of M.agents) {
      const r = el('div', 'at-row');
      const dot = el('i', 'r-dot'); dot.style.background = a.color;
      const name = el('b', null, a.name);
      r.append(dot, name, el('span', null, a.title || a.role), el('span', `s-${a.status}`, statusLine(a)), el('span', null, a.taskTitle ? `${a.taskId} ${a.taskTitle}` : '–'),
        el('span', null, inTime(a.nextCycleAt) ? `in ${inTime(a.nextCycleAt)}${a.cycleNote ? ' · ' + a.cycleNote : ''}` : (a.cycleNote || '–')));
      r.append(btn('Talk', async () => go(`agent=${encodeURIComponent(a.id)}`)));
      box.append(r);
    }
  }
}

// ---------- mailbox ----------
function renderMail() {
  if ($('ov-mail').hidden || !M) return;
  const box = $('mailList'); box.replaceChildren();
  if (!M.attention.length) box.append(el('p', 'muted', 'The mailbox is empty. Nothing waits for you.'));
  for (const a of M.attention) {
    const t = a.taskId && M.s.tasks.find(x => x.id === a.taskId);
    const c = el('div', `mail k-${a.kind}`);
    c.append(el('div', 'mail-kind', attnLabel(a)));
    c.append(el('div', 'mail-title', t ? `${t.id} · ${t.title}` : (a.text || '')));
    if (t && a.text) c.append(el('div', 'mail-text', a.text));
    if (a.kind === 'harvest') c.append(el('div', 'small', 'Passed. Review the changes in its folder and commit them if you like them.'));
    if (t && t.last && a.kind !== 'harvest') c.append(el('div', 'last', t.last));
    if (a.kind === 'login') c.append(el('div', 'small', 'In the Claude app\'s terminal panel (folder tools\\farm) run: powershell -ExecutionPolicy Bypass -File .\\sign-in-claude.ps1, then delete LOGIN-NEEDED in the farm folder.'));
    const row = el('div', 'actions');
    if (t && t.hasField) row.append(btn('Open folder', () => call('openField', t.id)));
    if (a.kind === 'harvest' && t) row.append(btn('Mark done', () => call('setStatus', t.id, 'done'), 'gbtn small primary'));
    if (a.kind === 'needs-you' && t) { row.append(btn('Try again', () => call('setStatus', t.id, 'ready'), 'gbtn small primary')); row.append(btn('Mark done', () => call('setStatus', t.id, 'done'))); }
    if (t) row.append(btn('Task file', () => call('openTask', t.id)));
    if (a.kind === 'off') row.append(btn('Turn farm on', async () => { await call('start'); setTimeout(refresh, 1500); }, 'gbtn small primary'));
    if (a.kind === 'limit') row.append(btn('Usage details', async () => go('silo')));
    c.append(row);
    box.append(c);
  }
}

// ---------- plant form ----------
function fillAgentSelect(pre) {
  const sel = $('fAgent');
  const keep = pre !== undefined ? pre : sel.value;
  sel.replaceChildren(el('option', null, 'Anyone'));
  sel.firstChild.value = '';
  for (const a of (M ? M.agents : DEFAULT_ROSTER).filter(x => x.role !== 'inspector')) { const o = el('option', null, `${a.name} (${a.title || a.role})`); o.value = a.id; sel.append(o); }
  sel.value = [...sel.options].some(o => o.value === keep) ? keep : '';
  const a = agentById(sel.value);
  if (a && a.kinds && a.kinds.length && pre !== undefined) $('fKind').value = a.kinds[0];
}
$('addForm').addEventListener('submit', async e => {
  e.preventDefault();
  const r = await call('addTask', {
    title: $('fTitle').value, todo: $('fTodo').value, doneWhen: $('fDone').value,
    kind: $('fKind').value, priority: $('fPrio').value, budget: $('fSize').value, agent: $('fAgent').value || undefined,
  });
  if (!r) { $('addMsg').textContent = 'Could not plant it (is the farm folder reachable?).'; return; }
  $('addMsg').textContent = r.status === 'ready' ? `Planted ${r.id}. A farmhand will pick it up soon.` : `Saved ${r.id} as an idea (add a "Done when" line to start it).`;
  const agent = $('fAgent').value;
  e.target.reset(); $('fAgent').value = agent;
  refresh();
});

// ---------- silo / summary ----------
$('refreshSummary').addEventListener('click', async () => { $('summary').dataset.manual = '1'; $('summary').textContent = (await call('refreshSummary')) || 'No summary yet.'; });

// ---------- settings ----------
for (let r = 25; r <= 70; r += 5) { const o = el('option', null, `${r}%`); o.value = r; $('sReserve').append(o); }
$('sStartup').addEventListener('change', async e => { const v = await call('setStartWithWindows', e.target.checked); if (typeof v === 'boolean') e.target.checked = v; });
$('sReserve').addEventListener('change', async e => { await call('setConfig', { weeklyReserve: Number(e.target.value) }); refresh(); });
$('sMax').addEventListener('change', async e => { await call('setConfig', { maxFarmhands: Number(e.target.value) }); refresh(); });
document.querySelectorAll('[data-open]').forEach(b => b.addEventListener('click', () => call('openFolder', b.dataset.open)));
function renderSettings() {
  const s = M.s;
  if (document.activeElement !== $('sStartup')) $('sStartup').checked = !!s.startWithWindows;
  if (document.activeElement !== $('sReserve')) $('sReserve').value = String(s.config.weeklyReserve);
  if (document.activeElement !== $('sMax')) $('sMax').value = String(s.config.maxFarmhands);
  $('farmDir').textContent = `Farm folder: ${s.farmDir || '?'}`;
}

// ---------- building interiors ----------
const ROOM_INFO = {
  farmhouse: ['The farmhouse', 'The Foreman\'s office: reads your goals, plans tasks, keeps the farm on track.'],
  workshop: ['The workshop', 'The coders build and test here. Code tasks only touch a copy of the repo (its own folder).'],
  library: ['The library', 'The researcher reads, compares and writes notes.'],
  market: ['The market', 'The marketer writes drafts. Nothing is posted or sent by the farm.'],
  tower: ['The watchtower', 'The inspector runs every "Done when" check before a task reaches the harvest.'],
};
let openRoom = null;
function renderRoom() {
  if (!openRoom || !M) return;
  const [title, sub] = ROOM_INFO[openRoom] || [openRoom, ''];
  $('roomTitle').textContent = title; $('roomSub').textContent = sub;
  const who = M.agents.filter(a => a.home === openRoom || (!ROOM_INFO[a.home] && openRoom === 'workshop' && a.role === 'worker'));
  Game.drawRoom($('roomCanvas'), openRoom, who);
  const list = $('roomList'); list.replaceChildren();
  if (!who.length) list.append(el('p', 'muted', 'Nobody lives here yet.'));
  for (const a of who) {
    const r = el('div', 'room-row');
    const dot = el('i', 'r-dot'); dot.style.background = a.color;
    const txt = el('div', 'room-text');
    const n = el('b', null, `${a.name} · ${a.title || a.role}`); 
    txt.append(n, el('div', 'small', `${statusLine(a)}${a.taskTitle ? ': ' + a.taskTitle : ''}`));
    if (a.doing) txt.append(el('div', 'small muted', a.doing));
    r.append(dot, txt, btn(`Talk to ${a.name}`, async () => go(`agent=${encodeURIComponent(a.id)}`), 'gbtn small primary'));
    list.append(r);
  }
  const act = $('roomActions'); act.replaceChildren();
  if (openRoom === 'farmhouse') act.append(btn('Open the Command Center', async () => go('ops'), 'gbtn'));
  act.append(btn('Task board', async () => go('board')));
}

// ---------- routing (#agent=bolt, #board, #mail, #plant, #settings, #silo, #ops, #room=library; "demo" anywhere = demo data) ----------
function parseHash() {
  const p = {};
  for (const part of location.hash.replace(/^#/, '').split('&').filter(Boolean)) {
    const i = part.indexOf('=');
    try { if (i < 0) p[decodeURIComponent(part)] = true; else p[decodeURIComponent(part.slice(0, i))] = decodeURIComponent(part.slice(i + 1)); } catch { /* ignore bad parts */ }
  }
  return p;
}
function go(route) {
  const p = parseHash();
  const keep = [];
  if (p.demo) keep.push('demo');
  if (p.hour) keep.push(`hour=${p.hour}`);
  if (route) keep.push(route);
  const h = keep.join('&');
  if (('#' + h) === location.hash || (!h && !location.hash)) applyRoute(); else location.hash = h;
}
const OVERLAYS = ['ov-ops', 'ov-mail', 'ov-plant', 'ov-silo', 'ov-settings', 'ov-room', 'ov-biz'];
function showOverlay(id) {
  for (const o of OVERLAYS) $(o).hidden = o !== id;
  $('scrim').hidden = !id;
  document.body.classList.toggle('modal', !!id);
}
function applyRoute() {
  const p = parseHash();
  const wantDemo = !!p.demo;
  if (wantDemo !== demo) {
    demo = wantDemo; api = demo ? demoApi : realApi;
    if (demo && !D) demoInit();
    seenAttn = null; prevAgentState = null; observed.length = 0;
    refresh();
  }
  tickClock();
  if (p.hour !== undefined) Game.setHour(Number(p.hour) % 24); else Game.setHour(null);
  if (p.agent && p.agent !== true) { showOverlay(null); openDrawer(p.agent); return; }
  closeDrawer();
  openRoom = null;
  if (p.board) { showOverlay('ov-ops'); showOpsTab('tasks'); }
  else if (p.ops) { showOverlay('ov-ops'); showOpsTab(p.ops === true ? 'mission' : p.ops); }
  else if (p.timeline) { showOverlay('ov-ops'); showOpsTab('timeline'); }
  else if (p.mail) { showOverlay('ov-mail'); renderMail(); }
  else if (p.biz) { showOverlay('ov-biz'); Biz.show(p.biz === true ? 'today' : p.biz); }
  else if (p.plant) { showOverlay('ov-plant'); fillAgentSelect(p.for && p.for !== true ? p.for : ''); $('addMsg').textContent = ''; setTimeout(() => $('fTitle').focus({ preventScroll: true }), 30); }
  else if (p.settings) { showOverlay('ov-settings'); }
  else if (p.silo) { showOverlay('ov-silo'); }
  else if (p.room && ROOM_INFO[p.room]) { openRoom = p.room; showOverlay('ov-room'); renderRoom(); }
  else showOverlay(null);
}
window.addEventListener('hashchange', applyRoute);
document.addEventListener('keydown', e => { if (e.key === 'Escape') { $('tip').hidden = true; go(''); } });
document.querySelectorAll('[data-close]').forEach(b => b.addEventListener('click', () => go('')));
$('scrim').addEventListener('click', () => go(''));
document.querySelectorAll('[data-plant]').forEach(b => b.addEventListener('click', () => go('plant')));
$('plantBtn').addEventListener('click', () => go('plant'));
$('boardBtn').addEventListener('click', () => go('board'));
$('mailBtn').addEventListener('click', () => go('mail'));
$('opsBtn').addEventListener('click', () => go('ops'));
$('bizBtn').addEventListener('click', () => go('biz'));
$('gearBtn').addEventListener('click', () => go('settings'));
$('usageBox').addEventListener('click', () => go('silo'));
$('pauseBtn').addEventListener('click', async () => { await call('pause', !(snap && snap.paused)); refresh(); });
$('powerBtn').addEventListener('click', async () => {
  if (snap && snap.manager && snap.manager.running) await call('stop'); else await call('start');
  setTimeout(refresh, 1500);
});

// ---------- loop ----------
let refreshing = false;
async function refresh() {
  if (refreshing) return;
  refreshing = true;
  try {
    const s = await call('snapshot');
    if (!s) throw new Error('no answer from the farm');
    snap = s;
    M = normalize(s);
    renderHud(); renderRoster(); renderAttention(); pushGame(); renderSettings(); renderDrawer(); renderOps(); renderMail(); renderRoom();
    const bizN = M.attention.filter(a => a.kind === 'biz' && a.level === 'urgent').length;
    $('bizHudBadge').hidden = !bizN; $('bizHudBadge').textContent = String(bizN);
    if (s.summary && $('summary').dataset.manual !== '1') $('summary').textContent = s.summary;
  } catch (e) {
    $('statusText').textContent = `Could not read the farm folder: ${e.message}`;
  } finally { refreshing = false; }
}
function layoutInsets() {
  const lc = $('leftcol').getBoundingClientRect(), hud = $('hud').getBoundingClientRect();
  Game.setInsets({ left: lc.right, top: hud.bottom, bottom: 8 });
}
Game.init($('world'), { onClick: onWorldClick, onHover });
layoutInsets();
window.addEventListener('resize', layoutInsets);
applyRoute();
refresh();
tickClock();
setInterval(() => { tickClock(); if (M) { renderHud(); renderRoster(); renderDrawer(); } }, 15000);
let pollTimer = setInterval(refresh, 2000);
document.addEventListener('visibilitychange', () => {
  clearInterval(pollTimer);
  pollTimer = setInterval(refresh, document.hidden ? 10000 : 2000);
  if (!document.hidden) refresh();
});
