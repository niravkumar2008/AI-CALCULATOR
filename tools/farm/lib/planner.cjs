// Self-prompting: how the farm decides its own next work.
// - The Foreman plans for the whole farm from goals.md (every planEveryMinutes, or when the seed bag runs low).
// - Each worker has its own cycle: when it is idle and has nothing ready in its line of work, it proposes 1-2 tasks
//   in its specialty.
// What keeps it good, not just busy: real goals, a look at the actual code, the list of existing tasks (no repeats),
// past failures and known limits (no repeating impossible work), each agent's memory, concrete "Done when" checks,
// a value score, and hard caps per cycle and per day. Every new task is still checked by the inspector after the work.
'use strict';

const STOP = new Set('a an the to of in on for and or with from by is be as at it its this that into add make write update create new file files task tasks readme section'.split(' '));
const words = s => new Set(String(s || '').toLowerCase().replace(/[^a-z0-9 ]+/g, ' ').split(/\s+/).filter(w => w.length > 2 && !STOP.has(w)));

function similarity(a, b) {
  const A = words(a), B = words(b);
  if (!A.size || !B.size) return 0;
  let inter = 0;
  for (const w of A) if (B.has(w)) inter++;
  return inter / (A.size + B.size - inter);
}

function isDuplicate(title, existing, threshold = 0.5) {
  return existing.find(t => similarity(title, t.meta ? t.meta.title : t.title) >= threshold) || null;
}

function parseDoneWhen(list) {
  return (Array.isArray(list) ? list : []).map(s => String(s).match(/^\s*[-*]?\s*(cmd|judge)\s*:\s*(.+)$/i)).filter(Boolean)
    .map(m => ({ type: m[1].toLowerCase(), text: m[2].trim() }));
}

const RULES = [
  'Each task must be small (under an hour for one AI farmhand), specific, and useful toward a goal.',
  'Each task needs "done_when": at least one "cmd: ..." check when possible (it runs in cmd.exe on Windows from the repository root and must exit 0: use node -e "..." or Windows paths; no ./ and no Linux-only tools) plus "judge: ..." lines for what a reviewer should confirm.',
  'Farmhands work in a fresh copy of the main branch: they only see committed code. They cannot install programs system-wide, commit, push, post, send email, or spend money. Marketing and email work = drafts in .farm/drafts/ only.',
  'Do not propose anything already in the task list (same or very similar), anything the lessons say is impossible here, or busywork (re-checking work that already passed, cosmetic churn).',
  'Never propose README, documentation, guide, comment or formatting-only tasks: Nirav does not want them.',
  'Prefer tasks that unblock a goal, find real bugs, add tests that run on Windows, or make the business ready to launch.',
];

function context({ goals, list, lessons, timeline, roster, business }) {
  const open = list.filter(t => t.meta.status !== 'done').map(t => `- ${t.meta.id} [${t.meta.status}] ${t.meta.title}${t.meta.agent ? ` (agent ${t.meta.agent})` : ''}`);
  const done = list.filter(t => t.meta.status === 'done').slice(-30).map(t => `- ${t.meta.id} ${t.meta.title}`);
  const team = roster.filter(a => a.role === 'worker').map(a => `- ${a.id} (${a.name}): ${a.title}; kinds: ${(a.kinds || []).join(', ')}`);
  return [
    `=== GOALS (from the owner) ===\n${goals || '(none written yet)'}`,
    business ? `=== THE BUSINESS RIGHT NOW (live numbers from the website's back office) ===\n${business}\nUse these to pick the most valuable work (e.g. sales are slow -> marketing drafts; many questions about one thing -> a clearer FAQ draft; orders waiting -> nothing for the farm, that is Nirav's packing). Never plan tasks that touch customers' data, send anything, or change prices.` : '',
    `=== TEAM ===\n${team.join('\n')}`,
    `=== OPEN TASKS (do not repeat these) ===\n${open.join('\n') || '(none)'}`,
    `=== DONE RECENTLY ===\n${done.join('\n') || '(none)'}`,
    `=== LESSONS: known limits and past failures (respect these) ===\n${lessons || '(none yet)'}`,
    `=== RECENT FARM EVENTS ===\n${(timeline || []).slice(-20).map(e => `- ${e.at.slice(0, 16)} ${e.type} ${e.task || ''} ${e.text || ''}`.slice(0, 200)).join('\n') || '(none)'}`,
  ].filter(Boolean).join('\n\n');
}

const SHAPE = `{
  "note": "one plain sentence for Nirav: what you planned and why",
  "tasks": [ { "title": "...", "why": "which goal it serves", "agent": "<agent id>", "kind": "code|marketing|research|hardware-check",
               "todo": "exact instructions", "done_when": ["cmd: ...", "judge: ..."], "size": "small|medium",
               "priority": 1-5, "value": 1-5 } ]
}`;

// Stuck tasks (needs-you) the Foreman decides on, so Nirav only sees what truly needs him.
function stuckList(list) {
  return list.filter(t => t.meta.status === 'needs-you' && !t.meta.fern_reviewed).map(t => {
    const last = (t.farmLog || '').split(/\r?\n/).filter(Boolean).slice(-2).join(' | ').slice(0, 400);
    return `- ${t.meta.id} "${t.meta.title}" (Fern retries so far: ${t.meta.fern_retries || 0}): ${last}`;
  });
}

const DECISIONS = `"decisions": [ { "task": "T-0002", "action": "retry" | "drop" | "keep", "instructions": "for retry: what to do differently; for drop/keep: why" } ]`;

function foremanPrompt(ctx, max) {
  const stuck = stuckList(ctx.list);
  return [
    'You are Fern, the Foreman of Proofline Farm: a team of AI farmhands that works on Nirav\'s calculator business while he is busy. Your job right now: decide the next most valuable work.',
    stuck.length ? `First, decide each STUCK task below yourself (Nirav wants as few approvals as possible): "retry" with clear new instructions if another approach can work on this Windows PC (each task gets at most one retry from you), "drop" if it is impossible here or not worth it, or "keep" ONLY if it truly needs Nirav (money, accounts, physical hardware, a business decision). Add the decisions to your JSON as ${DECISIONS}.\n=== STUCK TASKS ===\n${stuck.join('\n')}` : '',
    'You may read the repository (read-only) to see what exists and what is missing. Look before you plan: open the README, list folders, skim the files the goals point at.',
    `Propose at most ${max} tasks, assigned to the best teammate. Fewer, better tasks beat many weak ones. If the seed bag already has enough good work, return an empty task list and say so in the note.`,
    ...RULES,
    `Answer with ONLY this JSON:\n${SHAPE}${stuck.length ? `\n(plus ${DECISIONS} at the top level)` : ''}`,
    '',
    context(ctx),
  ].filter(Boolean).join('\n\n');
}

function agentCyclePrompt(agent, memory, ctx, max) {
  return [
    `You are ${agent.name}, the ${agent.title} of Proofline Farm (${agent.persona}). You are between jobs, and it is your cycle to find your own next work in your line (${(agent.kinds || []).join(', ')}).`,
    'You may read the repository (read-only). Look at the actual files before proposing.',
    `Propose at most ${max} tasks for yourself (agent "${agent.id}"). Only propose work that clearly helps a goal. If there is nothing worth doing in your line, return an empty list and say so in the note.`,
    ...RULES,
    `=== YOUR MEMORY (what you learned on past jobs) ===\n${memory || '(nothing yet)'}`,
    `Answer with ONLY this JSON:\n${SHAPE}`,
    '',
    context(ctx),
  ].join('\n\n');
}

function chatSystemPrompt(agent, memory, repoDir) {
  const extra = agent.role === 'foreman'
    ? 'As Foreman you may also add to the goals with "goals_append" (a short Markdown line), and plant tasks for any teammate.'
    : agent.role === 'inspector'
      ? 'As Inspector you explain past verdicts (the run records are in the farm folder\'s runs/ folder if Nirav asks) and may plant fix-up tasks.'
      : 'Plant tasks for yourself unless Nirav names someone else.';
  return [
    `You are ${agent.name}, the ${agent.title} on Proofline Farm, a team of AI farmhands working on Nirav's calculator business (Proofline). Personality: ${agent.persona}.`,
    'You are chatting with Nirav, the owner. He is new to programming: answer in short, plain, friendly sentences (2-6 sentences unless he asks for more). Be honest about what you can and cannot do.',
    `You can read the code (read-only) in ${repoDir} to answer questions. You cannot change files in this chat; real work happens as tasks.`,
    'When Nirav asks for work, plant tasks by ending your reply with exactly one fenced block like:',
    '```farm\n{"tasks":[{"title":"...","todo":"...","done_when":["cmd: ...","judge: ..."],"kind":"code","priority":2,"agent":"' + agent.id + '"}],"memory":"optional short thing to remember about how Nirav wants things"}\n```',
    'Only include the block when planting tasks or saving a memory. Task rules: ' + RULES.join(' '),
    extra,
    `=== YOUR MEMORY ===\n${memory || '(nothing yet)'}`,
  ].join('\n');
}

// Turns planner/chat output into task drafts the manager may create. Returns { accepted, rejected:[{title, why}] }.
// Nirav doesn't want README / documentation busywork (2026-10-18). Only tasks he asks for in chat may be docs.
const DOCS_ONLY = /\b(readme|documentation|docs?|guide|glossary|comments?|typos?|formatting|whitespace|blank line)\b/i;
const REAL_WORK = /\b(tests?|fix|bug|firmware|script|check|scanner)\b/i;

function filterTasks(raw, { list, roster, defaultAgent, max, minValue = 3, requireValue = true, userAsked = false }) {
  const accepted = [];
  const rejected = [];
  const ids = new Set(roster.map(a => a.id));
  for (const n of (Array.isArray(raw) ? raw : []).slice(0, Math.max(0, max))) {
    if (!n || !n.title) continue;
    const doneWhen = parseDoneWhen(n.done_when);
    if (!doneWhen.length) { rejected.push({ title: n.title, why: 'no Done-when checks' }); continue; }
    const dup = isDuplicate(n.title, [...list, ...accepted.map(a => ({ title: a.title }))]);
    if (dup) { rejected.push({ title: n.title, why: `too similar to ${dup.meta ? dup.meta.id : 'another new task'}` }); continue; }
    if (!userAsked && DOCS_ONLY.test(n.title) && !REAL_WORK.test(n.title)) { rejected.push({ title: n.title, why: 'docs/README-only work (Nirav does not want it)' }); continue; }
    if (requireValue && Number(n.value || 0) < minValue) { rejected.push({ title: n.title, why: `low value (${n.value || '?'})` }); continue; }
    const agent = ids.has(n.agent) && roster.find(a => a.id === n.agent).role === 'worker' ? n.agent : defaultAgent || null;
    accepted.push({
      title: String(n.title).slice(0, 120), todo: [n.todo || n.title, n.why ? `\nWhy: ${n.why}` : ''].join(''),
      doneWhen, kind: ['code', 'marketing', 'research', 'hardware-check'].includes(n.kind) ? n.kind : 'code',
      priority: Math.min(5, Math.max(1, Number(n.priority) || 3)), budget: n.size === 'medium' ? 'medium' : 'small', agent,
    });
  }
  return { accepted, rejected };
}

module.exports = { stuckList, similarity, isDuplicate, parseDoneWhen, foremanPrompt, agentCyclePrompt, chatSystemPrompt, filterTasks };
