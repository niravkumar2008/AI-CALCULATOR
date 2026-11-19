// Task files: one Markdown file per task in <farm>/tasks, with a small front-matter block.
// Only the front matter and the "## Farm log" section are written by the farm; the rest is yours.
'use strict';
const fs = require('fs');
const path = require('path');

// Local time, e.g. 2026-10-18 22:15 (the farm log is for you to read).
function stampNow(d = new Date()) {
  const p = n => String(n).padStart(2, '0');
  return `${d.getFullYear()}-${p(d.getMonth() + 1)}-${p(d.getDate())} ${p(d.getHours())}:${p(d.getMinutes())}`;
}

const STATUSES = ['idea', 'ready', 'working', 'checking', 'harvest', 'failed', 'needs-you', 'done'];
const FIELD_ORDER = ['id', 'title', 'kind', 'repo', 'priority', 'status', 'tries', 'budget', 'needs_approval', 'blocked_by'];

function parseValue(raw) {
  const v = raw.trim();
  if (v === '') return '';
  if (v === 'true') return true;
  if (v === 'false') return false;
  if (/^-?\d+(\.\d+)?$/.test(v)) return Number(v);
  if (v.startsWith('[') && v.endsWith(']')) {
    return v.slice(1, -1).split(',').map(s => s.trim()).filter(Boolean);
  }
  return v.replace(/^["']|["']$/g, '');
}

function formatValue(v) {
  if (Array.isArray(v)) return `[${v.join(', ')}]`;
  return String(v);
}

function parse(text, file) {
  const m = text.replace(/^﻿/, '').match(/^---\r?\n([\s\S]*?)\r?\n---\r?\n?([\s\S]*)$/);
  if (!m) throw new Error(`${path.basename(file)}: no front matter (--- block at the top)`);
  const meta = {};
  const comments = {};
  for (const line of m[1].split(/\r?\n/)) {
    const kv = line.match(/^([A-Za-z_][\w-]*)\s*:\s*(.*)$/);
    if (!kv) continue;
    let value = kv[2];
    const hash = value.search(/\s#/);
    if (hash >= 0) { comments[kv[1]] = value.slice(hash).trim(); value = value.slice(0, hash); }
    meta[kv[1]] = parseValue(value);
  }
  const body = m[2];
  return { file, raw: text, meta, comments, body, ...sections(body) };
}

// Pulls "## What to do", "## Done when" and "## Farm log" out of the body.
function sections(body) {
  const out = { todo: '', doneWhen: [], farmLog: '' };
  const parts = body.split(/^##\s+/m);
  for (const part of parts) {
    const nl = part.indexOf('\n');
    const head = (nl < 0 ? part : part.slice(0, nl)).trim().toLowerCase();
    const text = nl < 0 ? '' : part.slice(nl + 1);
    if (head.startsWith('what to do')) out.todo = text.trim();
    else if (head.startsWith('done when')) {
      for (const line of text.split(/\r?\n/)) {
        const c = line.match(/^\s*[-*]\s*(cmd|judge)\s*:\s*(.+?)\s*$/i);
        if (!c) continue;
        let value = c[2];
        if (c[1].toLowerCase() === 'cmd') value = value.replace(/\s+#\s.*$/, '');
        out.doneWhen.push({ type: c[1].toLowerCase(), text: value.trim() });
      }
    } else if (head.startsWith('farm log')) out.farmLog = text.trim();
  }
  return out;
}

function serialize(task) {
  const keys = [...FIELD_ORDER.filter(k => k in task.meta), ...Object.keys(task.meta).filter(k => !FIELD_ORDER.includes(k))];
  const lines = keys.map(k => {
    const c = task.comments[k];
    const line = `${k}: ${formatValue(task.meta[k])}`;
    return c ? `${line.padEnd(24)}${c}` : line;
  });
  return `---\n${lines.join('\n')}\n---\n${task.body.replace(/^\n+/, '')}`;
}

function load(file) {
  return parse(fs.readFileSync(file, 'utf8'), file);
}

function loadAll(dir) {
  let files = [];
  try { files = fs.readdirSync(dir).filter(f => /\.md$/i.test(f) && !f.startsWith('_')); } catch { return { tasks: [], errors: [] }; }
  const tasks = [];
  const errors = [];
  for (const f of files) {
    try { tasks.push(load(path.join(dir, f))); } catch (e) { errors.push(e.message); }
  }
  return { tasks, errors };
}

// Re-reads the file before writing so edits you made meanwhile (other fields, the body) are kept.
function update(task, changes, logLine) {
  let fresh;
  try { fresh = load(task.file); } catch { fresh = task; }
  Object.assign(fresh.meta, changes);
  if (logLine) {
    const stamp = stampNow();
    const entry = `- ${stamp} ${logLine.replace(/\r?\n/g, ' ')}`;
    if (/^##\s+Farm log\s*$/mi.test(fresh.body)) fresh.body = fresh.body.replace(/\s*$/, '') + `\n${entry}\n`;
    else fresh.body = fresh.body.replace(/\s*$/, '') + `\n\n## Farm log\n${entry}\n`;
  }
  const text = serialize(fresh);
  fs.writeFileSync(task.file, text, 'utf8');
  return Object.assign(task, fresh, { raw: text }, sections(fresh.body));
}

function nextId(dir) {
  const { tasks } = loadAll(dir);
  let max = 0;
  for (const t of tasks) {
    const n = Number(String(t.meta.id || '').replace(/\D/g, ''));
    if (n > max) max = n;
  }
  return `T-${String(max + 1).padStart(4, '0')}`;
}

function create(dir, { title, todo = '', doneWhen = [], kind = 'code', repo = 'AI-CALCULATOR', priority = 3, budget = 'small', status, from = '' }) {
  fs.mkdirSync(dir, { recursive: true });
  const id = nextId(dir);
  const hasDone = doneWhen.length > 0;
  const meta = {
    id, title: String(title).replace(/\r?\n/g, ' ').slice(0, 120), kind, repo, priority,
    status: status || (hasDone ? 'ready' : 'idea'), tries: 0, budget, needs_approval: false, blocked_by: [],
  };
  const done = hasDone ? doneWhen.map(d => `- ${d.type}: ${d.text}`).join('\n') : '- cmd: \n- judge: ';
  const body = `## What to do\n${todo || title}\n\n## Done when\n${done}\n` + (from ? `\n## Farm log\n- ${stampNow()} added by ${from}\n` : '');
  const task = { file: path.join(dir, `${id}.md`), meta, comments: {}, body };
  task.raw = serialize(task);
  fs.writeFileSync(task.file, task.raw, 'utf8');
  return task;
}

module.exports = { STATUSES, parse, load, loadAll, update, create, nextId, serialize };
