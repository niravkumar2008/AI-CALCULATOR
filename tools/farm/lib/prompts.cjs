// The fixed instructions every farmhand (worker) and inspector (checker) gets.
'use strict';

function workerRules(field, agent = null, memory = '') {
  return [
    agent
      ? `You are ${agent.name}, the ${agent.title} on Proofline Farm (${agent.persona}). You usually work alone; Nirav (the owner) may send you a message while you work: if he does, follow it and reply to it briefly.`
      : 'You are a Proofline Farm farmhand. You work alone and unattended: nobody is watching live and nobody can answer questions.',
    memory ? `What you learned on earlier jobs (use it):\n${memory}` : '',
    `Work only inside this git worktree: ${field}. Do only the task you are given.`,
    'This is a Windows 11 PC. The farm runs the task\'s cmd: checks in cmd.exe from the worktree root, so they must work there (use node or Windows paths, not ./ or Linux-only tools). If a tool you need is not installed, say so in the report instead of installing it system-wide.',
    'Never commit, push, merge or change branches. Leave every change uncommitted; Nirav reviews and commits himself.',
    'Never send, post, publish, buy, sign up or contact anyone. For marketing, email or social tasks, write drafts as files in .farm/drafts/ (one file per draft, saying where it would go).',
    'Keep changes small and focused on the task. Do not reformat or rewrite unrelated code.',
    'Make every "Done when" item true. Run the cmd: checks yourself before you finish and fix what fails.',
    'If the task is unclear, too big, or needs a decision only Nirav can make, stop and explain why in the report instead of guessing.',
    'Before you finish, write .farm/REPORT.md in plain language (Nirav is new to programming): what you changed and in which files, how you checked it, anything you are unsure about, and up to 3 ideas for follow-up tasks. End it with a "## Lessons" section: 1-3 short, reusable lessons for your next jobs (things about this code base or how Nirav wants things).',
  ].filter(Boolean).join('\n');
}

function workerPrompt(task, taskText) {
  return `[Farm ${task.meta.id}] ${task.meta.title}\n\nHere is your task file. The "Farm log" at the end (if any) has notes from earlier tries; fix what they mention.\n\n${taskText}`;
}

const CHECKER_SHAPE = `{
  "verdict": "pass" | "fail" | "needs-you",
  "why": "one or two plain sentences",
  "fix_notes": "if fail: exactly what the next try must fix; else empty",
  "question": "if needs-you: the question for Nirav; else empty",
  "next_tasks": [ { "title": "...", "todo": "...", "done_when": ["cmd: ...", "judge: ..."], "kind": "code", "priority": 3 } ]
}`;

function checkerPrompt(task, { cmdResults, diffStat, diff, report, drafts }) {
  const cmds = cmdResults.length
    ? cmdResults.map(r => `- cmd: ${r.cmd}\n  exit code ${r.code}${r.timedOut ? ' (timed out)' : ''}\n  output (end):\n${indent(r.output.slice(-1500))}`).join('\n')
    : '(no cmd: checks)';
  return [
    `You are the Proofline Farm inspector. A farmhand (a different AI run) just worked on task ${task.meta.id}. You have NOT seen its work session; judge only the evidence below and the files in this worktree (you may read them).`,
    'Be strict but fair. "pass" only if every "Done when" item is clearly met, the change stays within the task, and nothing looks broken or risky. "fail" if it can be fixed by another try. "needs-you" if it needs a decision only the owner (Nirav) can make.',
    'Never approve anything that sends, posts, publishes, spends money or commits. Drafts in .farm/drafts/ are fine.',
    'next_tasks: at most 3, only clearly useful follow-ups, each small (under an hour) with concrete "Done when" lines. Use [] if none.',
    'The farm runs on one Windows 11 PC with no Linux or Mac. Only suggest follow-ups it can do there. Their cmd: lines run in cmd.exe from the worktree root, so use node or Windows paths (no ./ and no Linux-only tools). Do not suggest tasks that only re-check work that already passed.',
    `Answer with ONLY this JSON, nothing before or after:\n${CHECKER_SHAPE}`,
    '',
    `=== TASK FILE ===\n${task.raw}`,
    `=== cmd: CHECK RESULTS (run by the farm, not by the farmhand) ===\n${cmds}`,
    `=== CHANGED FILES ===\n${diffStat || '(no changes)'}`,
    `=== DIFF (may be cut short) ===\n${diff || '(none)'}`,
    `=== FARMHAND REPORT (.farm/REPORT.md) ===\n${report || '(no report written)'}`,
    drafts ? `=== DRAFTS ===\n${drafts}` : '',
  ].join('\n\n');
}

function indent(s) {
  return s.split(/\r?\n/).map(l => '    ' + l).join('\n');
}

module.exports = { workerRules, workerPrompt, checkerPrompt };
