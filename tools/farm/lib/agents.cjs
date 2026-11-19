// The farm's characters: who they are (agents.json), what they remember (memory.md), and what was said (chat.jsonl).
// Everything lives in <farm>/agents/<id>/ so you can read or edit it by hand.
'use strict';
const fs = require('fs');
const path = require('path');

const DEFAULT_ROSTER = [
  { id: 'fern', name: 'Fern', role: 'foreman', title: 'Foreman: plans the work', kinds: [], color: '#e57373', home: 'farmhouse',
    persona: 'calm, organised, thinks about what matters most for the business this week' },
  { id: 'bolt', name: 'Bolt', role: 'worker', title: 'Coder', kinds: ['code'], color: '#64b5f6', home: 'workshop',
    persona: 'careful programmer, small clean changes, always runs the checks' },
  { id: 'rivet', name: 'Rivet', role: 'worker', title: 'Coder (firmware & hardware)', kinds: ['code', 'hardware-check'], color: '#4db6ac', home: 'workshop',
    persona: 'firmware and hardware-minded, double-checks pins, sizes and part numbers' },
  { id: 'poppy', name: 'Poppy', role: 'worker', title: 'Marketer (drafts only)', kinds: ['marketing'], color: '#f06292', home: 'market',
    persona: 'friendly, honest marketing writer; writes drafts only, never posts or sends' },
  { id: 'sage', name: 'Sage', role: 'worker', title: 'Researcher', kinds: ['research'], color: '#ba68c8', home: 'library',
    persona: 'curious researcher who reads sources and writes short, sourced notes' },
  { id: 'hawk', name: 'Hawk', role: 'inspector', title: 'Inspector: checks every harvest', kinds: [], color: '#ffb74d', home: 'tower',
    persona: 'strict but fair inspector who explains plainly what passed and what did not' },
  { id: 'penny', name: 'Penny', role: 'clerk', title: 'Postmaster: drafts customer replies', kinds: ['mail'], color: '#7986cb', home: 'postoffice',
    persona: 'warm, exact customer-service writer; drafts only, never sends; never promises what the facts do not say' },
];
// Crew members added after a farm was set up join existing farms once (deleting them later sticks).
const LATE_JOINERS = ['penny'];

const DEFAULT_GOALS = `# Farm goals (edit me: the Foreman plans from this)

Owner: Nirav, founder of Proofline (AI calculator startup, budget about $2,000). New to programming: keep everything plain.

## This month
1. v15 colour-LCD calculator firmware: get the prototype firmware solid and tested before the boards arrive.
2. Launch website + waitlist: clear, honest copy; no vendor names in public copy.
3. Marketing: honest positioning, waitlist-first, Kickstarter planned for spring 2027. Drafts only, Nirav approves.
4. Tests and real fixes: tests that run on Windows and catch real bugs. NO README or documentation-only work (Nirav said so).

## Later
- When orders start coming in: order handling, shipping updates, customer replies (drafts).

## Rules
- Small tasks (under an hour) with concrete "Done when" checks.
- Nothing is posted, sent, bought or committed by the farm.
- The farm runs on one Windows PC; checks run in cmd.exe; no system-wide installs.
`;

function dir(farm, id) { return path.join(farm, 'agents', id); }
function readText(f) { try { return fs.readFileSync(f, 'utf8'); } catch { return ''; } }

function roster(farm) {
  const f = path.join(farm, 'agents.json');
  let list = null;
  try { list = JSON.parse(fs.readFileSync(f, 'utf8')); } catch { /* make it */ }
  if (!Array.isArray(list) || !list.length) {
    list = DEFAULT_ROSTER;
    fs.mkdirSync(farm, { recursive: true });
    fs.writeFileSync(f, JSON.stringify(list, null, 2));
  }
  const joined = path.join(farm, 'agents', '.joined');
  const done = readText(joined).split(/\r?\n/).filter(Boolean);
  const add = LATE_JOINERS.filter(id => !done.includes(id));
  if (add.length) {
    for (const id of add) if (!list.some(a => a.id === id)) list.push(DEFAULT_ROSTER.find(a => a.id === id));
    try {
      fs.writeFileSync(f, JSON.stringify(list, null, 2));
      fs.mkdirSync(path.dirname(joined), { recursive: true });
      fs.writeFileSync(joined, [...done, ...add].join('\n') + '\n');
    } catch { /* read-only farm folder: still show them */ }
  }
  return list;
}

function ensureGoals(farm) {
  const f = path.join(farm, 'goals.md');
  if (!fs.existsSync(f)) fs.writeFileSync(f, DEFAULT_GOALS);
  return readText(f);
}

function memory(farm, id) { return readText(path.join(dir(farm, id), 'memory.md')).trim(); }

// Keeps the newest lines only, so memories stay short enough to send with every task.
function remember(farm, id, lines, maxLines = 40) {
  const add = (Array.isArray(lines) ? lines : [lines]).map(l => String(l).trim()).filter(Boolean);
  if (!add.length) return;
  fs.mkdirSync(dir(farm, id), { recursive: true });
  const f = path.join(dir(farm, id), 'memory.md');
  const old = readText(f).split(/\r?\n/).filter(l => l.startsWith('- '));
  const next = [...old, ...add.map(l => (l.startsWith('- ') ? l : `- ${l}`))].slice(-maxLines);
  fs.writeFileSync(f, `# What ${id} has learned (newest last)\n${next.join('\n')}\n`);
}

function say(farm, id, from, text) {
  if (!text || !String(text).trim()) return;
  fs.mkdirSync(dir(farm, id), { recursive: true });
  fs.appendFileSync(path.join(dir(farm, id), 'chat.jsonl'), JSON.stringify({ at: new Date().toISOString(), from, text: String(text).slice(0, 4000) }) + '\n');
}

function chatLog(farm, id, limit = 80) {
  return readText(path.join(dir(farm, id), 'chat.jsonl')).split(/\r?\n/).filter(Boolean)
    .map(l => { try { return JSON.parse(l); } catch { return null; } }).filter(Boolean).slice(-limit);
}

function chatSession(farm, id) { return readText(path.join(dir(farm, id), 'chat-session.txt')).trim() || null; }
function setChatSession(farm, id, sid) { if (sid) { fs.mkdirSync(dir(farm, id), { recursive: true }); fs.writeFileSync(path.join(dir(farm, id), 'chat-session.txt'), sid); } }

// Lessons the whole farm keeps: past failures and known limits (the planner reads these).
function lessons(farm) { return readText(path.join(farm, 'lessons.md')).trim(); }
function addLesson(farm, line, maxLines = 60) {
  const f = path.join(farm, 'lessons.md');
  const old = readText(f).split(/\r?\n/).filter(l => l.startsWith('- '));
  const l = `- ${String(line).replace(/\r?\n/g, ' ').slice(0, 300)}`;
  if (old.includes(l)) return;
  fs.writeFileSync(f, `# Farm lessons: known limits and past failures (newest last)\n${[...old, l].slice(-maxLines).join('\n')}\n`);
}

// Pulls the "## Lessons" bullets out of a farmhand's REPORT.md.
function lessonsFromReport(report) {
  const part = String(report || '').split(/^#{2,3}\s+/m).find(p => /^lessons?\b/i.test(p));
  if (!part) return [];
  return part.split(/\r?\n/).slice(1).map(l => l.replace(/^\s*[-*\d.]+\s*/, '').trim()).filter(l => l.length > 3).slice(0, 3);
}

// A chat reply may end with a ```farm {json}``` block (plant tasks, remember something, add to goals).
function splitFarmBlock(text) {
  const s = String(text || '');
  const m = s.match(/```farm\s*([\s\S]*?)```/i);
  if (!m) return { reply: s.trim(), actions: null };
  let actions = null;
  try { actions = JSON.parse(m[1]); } catch { actions = null; }
  return { reply: s.replace(m[0], '').trim(), actions };
}

function pickAgentFor(list, task, busy) {
  const wanted = task.meta.agent && list.find(a => a.id === task.meta.agent && a.role === 'worker');
  if (wanted) return busy.has(wanted.id) ? null : wanted;
  const kind = task.meta.kind || 'code';
  const free = list.filter(a => a.role === 'worker' && !busy.has(a.id));
  return free.find(a => (a.kinds || []).includes(kind)) || (list.some(a => a.role === 'worker' && (a.kinds || []).includes(kind)) ? null : free[0]) || null;
}

module.exports = {
  DEFAULT_ROSTER, roster, ensureGoals, memory, remember, say, chatLog, chatSession, setChatSession,
  lessons, addLesson, lessonsFromReport, splitFarmBlock, pickAgentFor,
};
