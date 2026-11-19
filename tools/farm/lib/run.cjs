// Runs one headless Claude run (`claude -p`) and watches it: streams events, notices a stuck or looping run,
// enforces a time limit, and classifies how it ended (ok, usage limit, network, login, error).
'use strict';
const fs = require('fs');
const { spawn, spawnSync } = require('child_process');

const LIMIT_RE = /usage limit|hit your (usage )?limit|limit reached|rate[ _-]?limit|out of (extra )?usage|weekly limit|5-hour limit/i;
const NET_RE = /ECONNRESET|ENOTFOUND|ETIMEDOUT|EAI_AGAIN|ECONNREFUSED|getaddrinfo|socket hang up|fetch failed|network|connection error|unable to connect|timed? ?out/i;
const LOGIN_RE = /failed to authenticate|not logged in|oauth|please run \/login|invalid api key|authentication/i;

function killTree(child) {
  if (!child || child.exitCode !== null) return;
  if (process.platform === 'win32') spawnSync('taskkill', ['/pid', String(child.pid), '/T', '/F'], { stdio: 'ignore' });
  else try { process.kill(-child.pid, 'SIGKILL'); } catch { child.kill('SIGKILL'); }
}

// "resets 3am", "resets 11:30pm", "|1792348578" (epoch seconds) → Date, else null.
function parseReset(text, now = new Date()) {
  const epoch = String(text).match(/\|(\d{10})\b/);
  if (epoch) return new Date(Number(epoch[1]) * 1000);
  const iso = String(text).match(/\d{4}-\d{2}-\d{2}T\d{2}:\d{2}(:\d{2}(\.\d+)?)?(Z|[+-]\d{2}:?\d{2})?/);
  if (iso) { const d = new Date(iso[0]); if (!isNaN(d)) return d; }
  const t = String(text).match(/resets?\s+(?:at\s+)?(\d{1,2})(?::(\d{2}))?\s*(am|pm)/i);
  if (t) {
    let h = Number(t[1]) % 12;
    if (t[3].toLowerCase() === 'pm') h += 12;
    const d = new Date(now);
    d.setHours(h, Number(t[2] || 0), 0, 0);
    if (d <= now) d.setDate(d.getDate() + 1);
    return d;
  }
  return null;
}

function summarizeInput(name, input = {}) {
  const v = input.command || input.file_path || input.pattern || input.url || input.query || input.description || '';
  return `${name}${v ? ' · ' + String(v).replace(/\s+/g, ' ').slice(0, 90) : ''}`;
}

/**
 * opts: { cmd: [exe, ...prefixArgs], args, cwd, env, prompt, logFile, timeoutMin, stuckMin, repeatLimit, onEvent,
 *         interactive, onSpawn }
 * interactive: the run was started with --input-format stream-json; it stays open for live messages
 * (onSpawn gets { send(text), isOpen() }) and is closed 2.5 s after a result with no new activity.
 * resolves: { outcome: 'ok'|'limit'|'network'|'login'|'error'|'stuck'|'timeout'|'looping'|'killed',
 *             result, sessionId, usage, costUsd, toolCalls, resetAt, detail }
 */
function runClaude(opts) {
  return new Promise(resolve => {
    const [exe, ...pre] = opts.cmd;
    const child = spawn(exe, [...pre, ...opts.args], {
      cwd: opts.cwd, env: opts.env, stdio: ['pipe', 'pipe', 'pipe'], windowsHide: true,
      detached: process.platform !== 'win32',
    });
    const log = opts.logFile ? fs.createWriteStream(opts.logFile, { flags: 'a' }) : null;
    const st = { sessionId: null, result: null, toolCalls: 0, lastAt: Date.now(), recent: [], texts: [], stderr: '', ended: null };
    let buf = '';
    let finished = false;
    let stdinOpen = true;
    let closeTimer = null;
    const send = text => {
      if (!stdinOpen) return false;
      child.stdin.write(JSON.stringify({ type: 'user', message: { role: 'user', content: String(text) } }) + '\n');
      if (closeTimer) { clearTimeout(closeTimer); closeTimer = null; }
      return true;
    };
    const scheduleClose = () => {
      if (closeTimer) clearTimeout(closeTimer);
      closeTimer = setTimeout(() => { if (stdinOpen) { stdinOpen = false; child.stdin.end(); } }, 2500);
    };

    const finish = (outcome, detail = '') => {
      if (finished) return;
      finished = true;
      stdinOpen = false;
      if (closeTimer) clearTimeout(closeTimer);
      clearInterval(timer);
      killTree(child);
      const r = st.result || {};
      const text = [r.result, ...st.texts.slice(-3), st.stderr.slice(-2000), detail].filter(Boolean).join('\n');
      let o = outcome;
      if (o === 'ok' && (r.is_error || /^error/.test(r.subtype || '') || /error|limit|abort|auth/i.test(r.terminal_reason || ''))) o = 'error';
      if (o === 'error' || o === 'exit') {
        if (LIMIT_RE.test(text)) o = 'limit';
        else if (LOGIN_RE.test(text)) o = 'login';
        else if (NET_RE.test(text)) o = 'network';
        else o = 'error';
      }
      if (log) log.end();
      resolve({
        outcome: o, result: r.result || '', sessionId: st.sessionId, usage: r.usage || null,
        costUsd: r.total_cost_usd || 0, toolCalls: st.toolCalls, resetAt: o === 'limit' ? parseReset(text) : null,
        detail: (detail || text).slice(-1500),
      });
    };

    const onLine = line => {
      if (!line.trim()) return;
      if (log) log.write(line + '\n');
      let j;
      try { j = JSON.parse(line); } catch { return; }
      st.lastAt = Date.now();
      if (j.type === 'system' && j.subtype === 'init') st.sessionId = j.session_id;
      if (j.type === 'assistant' && j.message && Array.isArray(j.message.content)) {
        for (const c of j.message.content) {
          if (c.type === 'tool_use') {
            st.toolCalls++;
            const key = c.name + JSON.stringify(c.input || {});
            st.recent.push(key);
            if (st.recent.length > 8) st.recent.shift();
            opts.onEvent && opts.onEvent({ kind: 'tool', tool: c.name, doing: summarizeInput(c.name, c.input), toolCalls: st.toolCalls });
            if (st.recent.filter(k => k === key).length >= (opts.repeatLimit || 5)) finish('looping', `repeated the same ${c.name} call ${opts.repeatLimit || 5} times`);
          } else if (c.type === 'text' && c.text) {
            st.texts.push(c.text);
            if (st.texts.length > 5) st.texts.shift();
          }
        }
      }
      if (j.type === 'result') {
        st.result = j;
        opts.onEvent && opts.onEvent({ kind: 'result', result: j });
        if (opts.interactive) scheduleClose();
      } else if (opts.interactive && closeTimer) { clearTimeout(closeTimer); closeTimer = null; }
      if (j.type === 'assistant' && opts.onEvent) for (const c of (j.message && j.message.content) || []) if (c.type === 'text' && c.text) opts.onEvent({ kind: 'text', text: c.text });
      if (/rate_limit/i.test(j.type || '') && opts.onEvent) opts.onEvent({ kind: 'rate_limit', data: j });
    };

    child.stdout.setEncoding('utf8');
    child.stdout.on('data', d => {
      buf += d;
      let i;
      while ((i = buf.indexOf('\n')) >= 0) { onLine(buf.slice(0, i)); buf = buf.slice(i + 1); }
    });
    child.stderr.setEncoding('utf8');
    child.stderr.on('data', d => { st.stderr = (st.stderr + d).slice(-8000); st.lastAt = Date.now(); });
    child.on('error', e => finish('error', `could not start Claude: ${e.message}`));
    child.on('close', () => { if (buf.trim()) onLine(buf); finish(st.result ? 'ok' : 'exit', st.result ? '' : 'Claude exited without a result'); });

    const started = Date.now();
    const timer = setInterval(() => {
      if (opts.shouldStop && opts.shouldStop()) return finish('killed', 'stopped by the farm manager');
      if (Date.now() - started > (opts.timeoutMin || 30) * 60000) return finish('timeout', `ran longer than ${opts.timeoutMin} min`);
      if (Date.now() - st.lastAt > (opts.stuckMin || 10) * 60000) return finish('stuck', `no output for ${opts.stuckMin} min`);
    }, 2000);

    if (opts.interactive) {
      send(opts.prompt || '');
      opts.onSpawn && opts.onSpawn({ send, isOpen: () => stdinOpen && !finished });
    } else {
      stdinOpen = false;
      child.stdin.end(opts.prompt || '');
    }
    child.stdin.on('error', () => { stdinOpen = false; });
  });
}

module.exports = { runClaude, parseReset, summarizeInput, killTree };
