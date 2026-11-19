#!/usr/bin/env node
// Farm guard: a PreToolUse hook loaded into every farm run (worker and checker).
// It blocks anything that leaves the computer, spends money, commits/pushes, or writes outside the task's
// own worktree. Exit code 2 = blocked; the message on stderr is shown to the farmhand so it can carry on.
'use strict';
const fs = require('fs');
const path = require('path');

// The (?:^|[;&|(]\s*|npx\s+) prefix means "at the start of a command", so `grep stripe src/` stays allowed.
const BLOCKED_COMMANDS = [
  [/\bgit\s+(commit|push|merge|rebase|reset\s+--hard|clean|switch|checkout\s+(-b\s+)?(main|master)\b|branch\s+-[dD]|worktree|remote|config|tag|stash|filter-branch|update-ref|am|cherry-pick|revert|pull|fetch)\b/i, 'git commits, pushes and branch changes are for Nirav; leave your changes uncommitted'],
  [/(?:^|[;&|(]\s*)gh\s+\w/im, 'the GitHub CLI is not allowed on the farm'],
  [/\bnpm\s+(publish|login|adduser|token)\b|\byarn\s+publish\b|\bpnpm\s+publish\b/i, 'publishing packages is not allowed'],
  [/(?:^|[;&|(]\s*|npx\s+)wrangler(\.cmd)?\s+(?!(dev|--version|types)\b)/im, 'deploying to Cloudflare is not allowed (wrangler dev is fine)'],
  [/(?:^|[;&|(]\s*)stripe(\.exe)?\s/im, 'Stripe commands are not allowed on the farm'],
  [/(?:^|[;&|(]\s*)(ssh|scp|sftp|ftp|rsync|telnet)(\.exe)?\s/im, 'remote connections are not allowed'],
  [/\b(curl|wget)\b[^|;&]*\s(-X\s*(POST|PUT|PATCH|DELETE)|--request\s+(POST|PUT|PATCH|DELETE)|-d\b|--data|-F\b|--form|-T\b|--upload-file)/i, 'sending data to websites is not allowed (reading with GET is fine)'],
  [/\b(Invoke-WebRequest|Invoke-RestMethod|iwr|irm)\b[^|;&]*-(Method\s+(Post|Put|Patch|Delete)|Body|InFile)/i, 'sending data to websites is not allowed (reading with GET is fine)'],
  [/\b(Send-MailMessage|sendmail)\b/i, 'sending email is not allowed; write a draft file instead'],
  [/\b(shutdown|Restart-Computer|Stop-Computer|format\s+[a-z]:|diskpart|bcdedit)\b/i, 'system commands are not allowed'],
  [/\b(netsh|Disable-NetAdapter|Restart-NetAdapter|Enable-NetAdapter|Set-NetAdapter)\b/i, 'network settings are not allowed'],
  [/\b(reg\s+(add|delete|import)|Set-ItemProperty\s+[^|;&]*HK(LM|CU)|New-ItemProperty\s+[^|;&]*HK(LM|CU))\b/i, 'registry changes are not allowed'],
  [/\b(schtasks|Register-ScheduledTask|Unregister-ScheduledTask|crontab)\b/i, 'scheduled tasks are not allowed'],
  [/\b(Set-ExecutionPolicy|Add-MpPreference|Set-MpPreference)\b/i, 'security settings are not allowed'],
  [/\bclaude(\.exe)?\s+(-p|--print|-c|--continue|-r|--resume)\b/i, 'farmhands may not start other Claude runs'],
  [/\b(winget|choco|scoop)\s+(install|uninstall|upgrade)\b|\bnpm\s+(install|i)\s+(-g|--global)\b|\bpip\s+install\b[^|;&]*--user\b/i, 'installing programs system-wide is not allowed (project-local installs are fine)'],
];

const OUTWARD_TOOL = /^mcp__|send|post|publish|tweet|reply|forward|create_draft|purchase|checkout|pay/i;

function norm(p) {
  // Git Bash writes C:\Users as /c/Users; turn that back into a Windows path first.
  let s = String(p);
  if (process.platform === 'win32') s = s.replace(/^\/([a-zA-Z])(\/|$)/, '$1:\\');
  return path.resolve(s).replace(/[\\/]+$/, '').toLowerCase();
}

function inside(child, root) {
  const c = norm(child);
  const r = norm(root);
  return c === r || c.startsWith(r + path.sep);
}

function decide(input, env = process.env) {
  const tool = String(input.tool_name || '');
  const ti = input.tool_input || {};
  const root = env.PROOFLINE_FIELD || input.cwd || process.cwd();

  if (OUTWARD_TOOL.test(tool)) return `"${tool}" can reach outside services; the farm only writes drafts. Put the draft in .farm/drafts/ instead.`;

  if (tool === 'Bash' || tool === 'PowerShell') {
    const cmd = String(ti.command || '');
    for (const [re, why] of BLOCKED_COMMANDS) if (re.test(cmd)) return `Blocked: ${why}.`;
    // Recursive deletes must stay inside the worktree.
    if (/\b(rm\s+-[a-z]*r|Remove-Item\b[^|;&]*-Recurse|rmdir\s+\/s|rd\s+\/s)/i.test(cmd)) {
      const abs = cmd.match(/(?:[A-Za-z]:[\\/][^\s'"|;&]*|(?:^|\s)\/[^\s'"|;&]+|~[\\/]?[^\s'"|;&]*)/g) || [];
      for (const a of abs) {
        const p = a.trim().replace(/^~/, env.USERPROFILE || env.HOME || '~');
        if (!inside(p, root)) return `Blocked: deleting outside your worktree (${a.trim()}).`;
      }
    }
    return null;
  }

  if (['Edit', 'Write', 'MultiEdit', 'NotebookEdit'].includes(tool)) {
    const p = ti.file_path || ti.notebook_path || '';
    if (!p) return null;
    const full = path.isAbsolute(p) ? p : path.join(input.cwd || root, p);
    if (!inside(full, root)) return `Blocked: you may only change files inside your worktree (${root}).`;
    if (/[\\/]\.git([\\/]|$)/i.test(full)) return 'Blocked: do not touch .git.';
    return null;
  }
  return null;
}

function logBlock(env, input, why) {
  const dir = env.PROOFLINE_FARM_DIR;
  if (!dir) return;
  try {
    fs.mkdirSync(path.join(dir, 'logs'), { recursive: true });
    const what = input.tool_name + ' ' + JSON.stringify(input.tool_input || {}).slice(0, 300);
    fs.appendFileSync(path.join(dir, 'logs', 'guard.log'),
      `${new Date().toISOString()} ${env.PROOFLINE_TASK_ID || '?'} ${why} | ${what}\n`);
  } catch { /* logging is best effort */ }
}

if (require.main === module) {
  let raw = '';
  process.stdin.setEncoding('utf8');
  process.stdin.on('data', d => { raw += d; });
  process.stdin.on('end', () => {
    let input = {};
    try { input = JSON.parse(raw || '{}'); } catch { process.exit(0); }
    const why = decide(input);
    if (why) {
      logBlock(process.env, input, why);
      process.stderr.write(why + '\n');
      process.exit(2);
    }
    process.exit(0);
  });
}

module.exports = { decide };
