#!/usr/bin/env node
// A stand-in for `claude -p` so the farm can be tested without using any Claude usage.
// FAKE_MODE: pass (default) | badwork (cmd check fails) | fail-verdict | limit | network | login | stuck
// The worker writes hello.txt + .farm/REPORT.md; the inspector answers with a verdict JSON.
'use strict';
const fs = require('fs');
const path = require('path');

const args = process.argv.slice(2);
const mode = process.env.FAKE_MODE || 'pass';
const isChecker = args.includes('Read,Glob,Grep');
const sid = `fake-${isChecker ? 'chk' : 'wrk'}-${process.pid}`;
const out = o => process.stdout.write(JSON.stringify(o) + '\n');

let prompt = '';
const streamIn = args.includes('stream-json') && args[args.indexOf('--input-format') + 1] === 'stream-json';
let started = false;
process.stdin.setEncoding('utf8');
process.stdin.on('data', d => {
  prompt += d;
  if (streamIn && !started && prompt.includes('\n')) { started = true; prompt = JSON.parse(prompt.split('\n')[0]).message.content; work(); }
});
process.stdin.on('end', () => { if (streamIn) process.exit(0); work(); });
function work() {
  out({ type: 'system', subtype: 'init', session_id: sid, cwd: process.cwd(), tools: [], model: 'fake' });
  // Report usage like the usage mod would.
  if (process.env.PROOFLINE_FARM_DIR && process.env.FAKE_WEEKLY) {
    fs.writeFileSync(path.join(process.env.PROOFLINE_FARM_DIR, 'usage.json'), JSON.stringify({
      at: Date.now(), source: 'fake', rateLimits: [
        { kind: 'five_hour', percentUsed: Number(process.env.FAKE_FIVE || 5), resetsAt: new Date(Date.now() + 3 * 3600e3).toISOString() },
        { kind: 'seven_day', percentUsed: Number(process.env.FAKE_WEEKLY), resetsAt: new Date(Date.now() + 3 * 86400e3).toISOString() },
      ],
    }));
  }
  if (mode === 'stuck' && !isChecker) { setInterval(() => {}, 1000); return; }
  const end = (o) => { out({ type: 'result', subtype: 'success', is_error: false, session_id: sid, total_cost_usd: 0, usage: {}, ...o }); if (!streamIn) process.exit(0); };
  if (mode === 'limit') return end({ is_error: true, terminal_reason: 'api_error', result: "You've hit your usage limit · resets 3am" });
  if (mode === 'network') return end({ is_error: true, terminal_reason: 'api_error', result: 'API Error: Connection error. fetch failed' });
  if (mode === 'login') return end({ is_error: true, terminal_reason: 'api_error', result: 'Failed to authenticate: OAuth session expired' });

  // Chat with an agent: a reply plus a ```farm``` block that plants a task and saves a memory.
  // Penny drafting a customer reply: records what she was given (tools, env) so the test can check it.
  if (String(prompt).startsWith('You are Penny,')) {
    if (process.env.FAKE_PENNY_LOG) {
      const ti = args.indexOf('--tools');
      fs.appendFileSync(process.env.FAKE_PENNY_LOG, JSON.stringify({ tools: ti >= 0 ? args[ti + 1] : null, tokenInEnv: !!process.env.PROOFLINE_ADMIN_TOKEN, injected: /IGNORE ALL RULES/.test(prompt), note: /NIRAV'S NOTE/.test(prompt), facts: /\$215/.test(prompt) }) + '\n');
    }
    const draft = { category: 'shipping', summary: 'Asks when the order ships', draft: 'Hi Sam,\n\nIt ships within the time on our site. [[ask Nirav: exact week?]]\n\nNirav\nProofline', needs_nirav: 'the exact ship week', urgent: false };
    return end({ result: JSON.stringify(draft) });
  }
  if (String(prompt).startsWith('Nirav says:')) {
    const block = { tasks: [{ title: 'Write a goodbye page for the farm demo', todo: 'Make bye.txt', done_when: ['cmd: node -e "process.exit(0)"'], kind: 'code' }], memory: 'Nirav likes short replies' };
    const fence = '`'.repeat(3);
    out({ type: 'assistant', message: { content: [{ type: 'text', text: 'Sure, planting it now.' }] } });
    return end({ result: `Sure, planting it now.\n${fence}farm\n${JSON.stringify(block)}\n${fence}` });
  }
  // A planning cycle (Fern or a worker's own cycle): one good task, one repeat, one low-value, one for Poppy.
  if (/^You are [A-Z][a-z]+, the /.test(String(prompt)) && /Answer with ONLY this JSON/.test(String(prompt))) {
    const plan = { note: 'Two useful things and one repeat.', tasks: [
      { title: 'Add Windows test runner script for the calculator core', why: 'goal 4', agent: 'bolt', kind: 'code', todo: 'x', done_when: ['cmd: node -e "process.exit(0)"', 'judge: runs on Windows'], size: 'small', priority: 2, value: 5 },
      { title: 'Say hello', why: 'dup', agent: 'bolt', kind: 'code', todo: 'x', done_when: ['cmd: node -e "process.exit(0)"'], size: 'small', priority: 3, value: 4 },
      { title: 'Polish whitespace in comments everywhere', why: 'meh', agent: 'bolt', kind: 'code', todo: 'x', done_when: ['judge: tidy'], size: 'small', priority: 5, value: 1 },
      { title: 'Draft three waitlist launch posts', why: 'goal 3', agent: 'poppy', kind: 'marketing', todo: 'x', done_when: ['judge: honest, no vendor names'], size: 'small', priority: 2, value: 4 },
    ] };
    // Fern decides stuck tasks: retry the first, drop the second.
    const stuck = (String(prompt).split('=== STUCK TASKS ===')[1] || '').match(/T-\d{4}/g) || [];
    if (stuck.length) plan.decisions = [...new Set(stuck)].map((task, i) => ({ task, action: i === 0 ? 'retry' : 'drop', instructions: i === 0 ? 'use node instead of cmake' : 'not possible on this PC' }));
    return end({ result: JSON.stringify(plan) });
  }
  if (!isChecker) {
    out({ type: 'assistant', message: { content: [{ type: 'tool_use', name: 'Write', input: { file_path: 'hello.txt' } }] } });
    fs.writeFileSync('hello.txt', mode === 'badwork' ? 'wrong\n' : 'hello farm\n');
    fs.mkdirSync('.farm', { recursive: true });
    fs.writeFileSync(path.join('.farm', 'REPORT.md'), 'Wrote hello.txt. Checked it with the cmd check.\n');
    return end({ result: 'Done.' });
  }
  const verdict = mode === 'fail-verdict'
    ? { verdict: 'fail', why: 'The greeting is too short.', fix_notes: 'Make it longer.', question: '', next_tasks: [] }
    : { verdict: 'pass', why: 'hello.txt says hello farm, as asked.', fix_notes: '', question: '', next_tasks: [{ title: 'Add a goodbye file', todo: 'Write bye.txt', done_when: ['cmd: node -e "process.exit(0)"'], kind: 'code', priority: 4 }] };
  end({ result: 'Here is my verdict:\n' + JSON.stringify(verdict) });
}
