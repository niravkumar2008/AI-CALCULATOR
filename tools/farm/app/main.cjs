// Proofline Farm desktop app (Electron).
// - Runs the farm manager in the background and keeps it running (restarts it if it stops by accident).
// - Closing the window only hides it: the farm keeps working, with an icon in the tray (by the clock).
//   "Quit" in the tray menu stops the farm.
// - "Start with Windows" (in Settings) opens the app hidden at login, so the farm is always on.
'use strict';
const { app, BrowserWindow, Tray, Menu, ipcMain, shell, nativeImage, Notification, safeStorage } = require('electron');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawn, spawnSync } = require('child_process');
const icon = require('./icon.cjs');

const FARM_TOOLS = path.join(__dirname, '..');
const FARM = process.env.PROOFLINE_FARM_DIR || path.join(os.homedir(), 'ProoflineFarm');
const MANAGER = path.join(FARM_TOOLS, 'manager.cjs');
const tasks = require(path.join(FARM_TOOLS, 'lib', 'tasks.cjs'));
const crew = require(path.join(FARM_TOOLS, 'lib', 'agents.cjs'));
const PREFS = path.join(FARM, 'app-prefs.json');
// Started at login = open hidden in the tray. Unpackaged (electron.exe + this folder), the folder must be passed too.
const LOGIN_ARGS = app.isPackaged ? ['--hidden'] : [app.getAppPath(), '--hidden'];

let win = null;
let tray = null;
let child = null;          // the manager this app started
let quitting = false;
let wantRunning = true;    // the user's on/off choice for the farm
let restarts = [];
let toldAboutTray = false;

const readJson = (f, d = null) => { try { return JSON.parse(fs.readFileSync(f, 'utf8').replace(/^﻿/, '')); } catch { return d; } };
const prefs = () => ({ farmOn: true, ...readJson(PREFS, {}) });
const savePrefs = p => { fs.mkdirSync(FARM, { recursive: true }); fs.writeFileSync(PREFS, JSON.stringify({ ...prefs(), ...p }, null, 2)); };

// One app at a time (a test snapshot run may start beside the real one).
if (!process.env.PROOFLINE_APP_SNAPSHOT && !app.requestSingleInstanceLock()) { app.quit(); }
app.on('second-instance', () => showWindow());

// ---------- the farm manager process ----------
function pidAlive(pid) { try { process.kill(pid, 0); return true; } catch { return false; } }

function managerInfo() {
  const lock = readJson(path.join(FARM, 'manager.lock'));
  const running = !!(lock && lock.pid && pidAlive(lock.pid));
  return { running, ours: running && child && lock.pid === child.pid, pid: running ? lock.pid : null };
}

function nodeExe() {
  const r = spawnSync(process.platform === 'win32' ? 'where' : 'which', ['node'], { encoding: 'utf8', windowsHide: true });
  const first = (r.stdout || '').split(/\r?\n/).find(Boolean);
  return first || null;
}

function cleanEnv() {
  const env = { ...process.env };
  for (const k of Object.keys(env)) {
    if (/^(CLAUDE_CODE_|CLAUDECODE$|CLAUDE_AGENT_SDK|CLAUDE_PID$|CLAUDE_EFFORT$|CLAUDE_PREVIEW|ANTHROPIC_BASE_URL$|ELECTRON_)/.test(k)) delete env[k];
  }
  return env;
}

function startFarm() {
  wantRunning = true;
  savePrefs({ farmOn: true });
  if (managerInfo().running) return;
  const node = nodeExe();
  const env = { ...cleanEnv(), ...business.managerEnv() };
  let exe = node;
  if (!node) { exe = process.execPath; env.ELECTRON_RUN_AS_NODE = '1'; }
  fs.mkdirSync(path.join(FARM, 'logs'), { recursive: true });
  const out = fs.openSync(path.join(FARM, 'logs', 'manager-console.txt'), 'a');
  child = spawn(exe, [MANAGER, 'run'], { cwd: FARM_TOOLS, env, windowsHide: true, stdio: ['ignore', out, out] });
  child.on('exit', code => {
    child = null;
    if (quitting || !wantRunning) return;
    // Restart after an accidental stop, but not in a crash loop.
    restarts = restarts.filter(t => Date.now() - t < 10 * 60000);
    if (restarts.length >= 5) { notify('The farm keeps stopping', 'It stopped 5 times in 10 minutes. Open the app, check logs\\farm.log.'); return; }
    restarts.push(Date.now());
    setTimeout(() => { if (wantRunning && !quitting) startFarm(); }, 15000);
  });
  updateTray();
}

function stopFarm() {
  wantRunning = false;
  savePrefs({ farmOn: false });
  const info = managerInfo();
  if (child) killTree(child.pid);
  else if (info.running) killTree(info.pid);
  updateTray();
}

function killTree(pid) {
  if (process.platform === 'win32') spawnSync('taskkill', ['/pid', String(pid), '/T', '/F'], { stdio: 'ignore', windowsHide: true });
  else try { process.kill(pid, 'SIGTERM'); } catch { /* gone */ }
}

// ---------- window + tray ----------
function showWindow() {
  if (!win) createWindow();
  win.show();
  win.focus();
}

function createWindow() {
  win = new BrowserWindow({
    width: 1180, height: 820, minWidth: 760, minHeight: 560, show: false, title: 'Proofline Farm',
    backgroundColor: '#1d2a1f', autoHideMenuBar: true,
    icon: nativeImage.createFromBuffer(icon.png(256)),
    webPreferences: { preload: path.join(__dirname, 'preload.cjs'), contextIsolation: true, nodeIntegration: false, sandbox: true },
  });
  win.loadFile(path.join(__dirname, 'index.html'));
  win.once('ready-to-show', () => {
    if (process.argv.includes('--hidden') || process.env.PROOFLINE_APP_SNAPSHOT) return;
    win.maximize();
    win.show();
  });
  // Test hook: PROOFLINE_APP_SNAPSHOT=<base.png> saves a picture for each view in PROOFLINE_APP_SNAPSHOT_VIEWS
  // (comma-separated URL hashes, e.g. "#,#agent=bolt,#board"), then quits. Size: PROOFLINE_APP_SNAPSHOT_SIZE=1600x900.
  if (process.env.PROOFLINE_APP_SNAPSHOT) {
    const [w, h] = (process.env.PROOFLINE_APP_SNAPSHOT_SIZE || '1600x900').split('x').map(Number);
    win.setContentSize(w, h);
    win.webContents.once('did-finish-load', async () => {
      const base = process.env.PROOFLINE_APP_SNAPSHOT.replace(/\.png$/i, '');
      const views = (process.env.PROOFLINE_APP_SNAPSHOT_VIEWS || '#').split(',');
      for (let i = 0; i < views.length; i++) {
        await win.webContents.executeJavaScript(`location.hash = ${JSON.stringify(views[i])}`);
        await new Promise(r => setTimeout(r, Number(process.env.PROOFLINE_APP_SNAPSHOT_WAIT || 3000)));
        fs.writeFileSync(`${base}-${i}.png`, (await win.webContents.capturePage()).toPNG());
      }
      quitting = true; stopFarmOnQuit(); app.quit();
    });
  }
  win.on('close', e => {
    if (quitting) return;
    e.preventDefault();
    win.hide();
    if (!toldAboutTray) {
      toldAboutTray = true;
      notify('Proofline Farm is still working', 'It keeps farming in the tray (by the clock). Right-click the sprout icon to quit.');
    }
  });
  win.webContents.setWindowOpenHandler(({ url }) => { shell.openExternal(url); return { action: 'deny' }; });
}

// Business HQ (the Worker's back office): token, IPC for the screen, notifications.
const business = require('./business-main.cjs')({
  ipcMain, safeStorage, shell, FARM, FARM_TOOLS, notify: (t, b) => notify(t, b),
  restartManager: () => { const info = managerInfo(); if (child) killTree(child.pid); else if (info.running && wantRunning) { killTree(info.pid); setTimeout(startFarm, 3000); } },
});

function notify(title, body) {
  if (Notification.isSupported()) new Notification({ title, body, icon: nativeImage.createFromBuffer(icon.png(64)) }).show();
}

function trayStatus() {
  const s = readJson(path.join(FARM, 'state.json'), {}) || {};
  const f = s.feed || {};
  const info = managerInfo();
  if (!info.running) return { dot: 'off', text: 'Farm is off' };
  if (f.loginNeeded) return { dot: 'alert', text: 'Claude needs a sign-in' };
  const n = (f.farmhands || []).length;
  if (n) return { dot: 'ok', text: `${n} farmhand${n > 1 ? 's' : ''} working` };
  return { dot: 'wait', text: f.reason ? `Waiting: ${f.reason}`.slice(0, 120) : 'Waiting for ready tasks' };
}

let lastDot = null;
function updateTray() {
  if (!tray) return;
  const st = trayStatus();
  if (st.dot !== lastDot) { tray.setImage(nativeImage.createFromBuffer(icon.png(32, st.dot))); lastDot = st.dot; }
  tray.setToolTip(`Proofline Farm: ${st.text}`.slice(0, 127));
  const info = managerInfo();
  const paused = fs.existsSync(path.join(FARM, 'PAUSE'));
  tray.setContextMenu(Menu.buildFromTemplate([
    { label: st.text, enabled: false },
    { type: 'separator' },
    { label: 'Open Proofline Farm', click: showWindow },
    info.running
      ? { label: paused ? 'Resume farm' : 'Pause farm', click: () => setPaused(!paused) }
      : { label: 'Start farm', click: startFarm },
    { label: 'Open farm folder', click: () => shell.openPath(FARM) },
    { type: 'separator' },
    { label: 'Quit (stops the farm)', click: () => { quitting = true; stopFarmOnQuit(); app.quit(); } },
  ]));
}

function stopFarmOnQuit() {
  if (child) killTree(child.pid);
}

function setPaused(on) {
  const f = path.join(FARM, 'PAUSE');
  if (on) fs.writeFileSync(f, 'Paused from the Proofline Farm app. Delete this file (or press Resume) to continue.\n');
  else fs.rmSync(f, { force: true });
  updateTray();
}

// ---------- what the screen asks for ----------
function snapshot() {
  const s = readJson(path.join(FARM, 'state.json'), {}) || {};
  const list = tasks.loadAll(path.join(FARM, 'tasks')).tasks.map(t => {
    const log = (t.farmLog || '').split(/\r?\n/).filter(Boolean);
    return {
      id: t.meta.id, title: t.meta.title, status: t.meta.status, priority: t.meta.priority, kind: t.meta.kind,
      tries: t.meta.tries, from: t.meta.from || null, branch: t.meta.branch || null,
      last: (log.pop() || '').replace(/^-\s*/, ''), hasField: fs.existsSync(path.join(FARM, 'fields', String(t.meta.id))),
      todo: t.todo, doneWhen: t.doneWhen, agent: t.meta.agent || null, fernReviewed: !!t.meta.fern_reviewed,
    };
  });
  let summary = '';
  try { summary = fs.readFileSync(path.join(FARM, 'summary.md'), 'utf8'); } catch { /* none yet */ }
  const cfg = readJson(path.join(FARM, 'farm.config.json'), {}) || {};
  return {
    farmDir: FARM, manager: managerInfo(), wantRunning, paused: fs.existsSync(path.join(FARM, 'PAUSE')),
    feed: s.feed || null, usage: readJson(path.join(FARM, 'usage.json')), tasks: list, summary,
    config: { weeklyReserve: cfg.weeklyReserve ?? 30, maxFarmhands: cfg.maxFarmhands ?? 3 },
    startWithWindows: app.getLoginItemSettings({ args: LOGIN_ARGS }).openAtLogin,
    loginNeeded: fs.existsSync(path.join(FARM, 'LOGIN-NEEDED')),
    agents: agentsNow(s.feed),
    foreman: (s.feed && s.feed.foreman) || { nextCycleAt: null, lastCycleAt: null, lastNote: null },
    timeline: timeline(60),
    attention: attention(s.feed, list),
    goals: readText(path.join(FARM, 'goals.md')),
  };
}

function readText(f) { try { return fs.readFileSync(f, 'utf8'); } catch { return ''; } }

// The characters: live from the manager's feed, or the roster (all asleep/idle) when the manager isn't running.
function agentsNow(feed) {
  const fresh = feed && feed.at && Date.now() - feed.at < 60000 && Array.isArray(feed.agents);
  if (fresh) return feed.agents;
  const off = !managerInfo().running;
  return crew.roster(FARM).map(a => ({ id: a.id, name: a.name, title: a.title, role: a.role, color: a.color, home: a.home, kinds: a.kinds || [],
    status: off ? 'asleep' : 'idle', taskId: null, taskTitle: null, doing: null, since: null,
    lastLine: (crew.chatLog(FARM, a.id, 1)[0] || {}).text || null, nextCycleAt: null, cycleNote: null }));
}

function timeline(n) {
  return readText(path.join(FARM, 'logs', 'events.jsonl')).split(/\r?\n/).filter(Boolean).slice(-n)
    .map(l => {
      try {
        const e = JSON.parse(l);
        const text = `${e.task ? e.task + ' ' : ''}${e.type === 'new-task' && e.title ? e.title + ' · ' : ''}${e.text || ''}`;
        return { at: e.at, agent: e.agent || null, kind: e.type, task: e.task, text };
      } catch { return null; }
    })
    .filter(Boolean).reverse();
}

function attention(feed, list) {
  const out = [];
  if (!managerInfo().running) out.push({ kind: 'off', taskId: null, text: 'The farm is off.' });
  if (fs.existsSync(path.join(FARM, 'LOGIN-NEEDED'))) out.push({ kind: 'login', taskId: null, text: 'Claude needs a sign-in (Settings has the steps).' });
  if (feed && feed.limitUntil && Date.now() < new Date(feed.limitUntil).getTime()) out.push({ kind: 'limit', taskId: null, text: `Out of usage until ${new Date(feed.limitUntil).toLocaleString(undefined, { weekday: 'short', hour: 'numeric', minute: '2-digit' })}.` });
  for (const t of list.filter(x => x.status === 'harvest')) out.push({ kind: 'harvest', taskId: t.id, text: `${t.id} passed: review and commit "${t.title}"` });
  for (const t of list.filter(x => x.status === 'needs-you' && x.fernReviewed)) out.push({ kind: 'needs-you', taskId: t.id, text: `${t.id} needs you: ${t.title}` });
  // The business first: replies to approve and orders to ship matter more than code harvests.
  const biz = ((feed && feed.business) || {}).alerts || [];
  const urgent = biz.filter(a => a.level === 'urgent').map(a => ({ kind: 'biz', taskId: null, text: a.text, go: a.go, level: a.level }));
  const rest = biz.filter(a => a.level === 'todo').map(a => ({ kind: 'biz', taskId: null, text: a.text, go: a.go, level: a.level }));
  return [...urgent, ...out, ...rest];
}

function taskFile(id) {
  if (!/^T-\d+$/.test(id)) throw new Error('bad task id');
  return path.join(FARM, 'tasks', `${id}.md`);
}

ipcMain.handle('farm:snapshot', () => snapshot());
ipcMain.handle('farm:start', () => { startFarm(); return snapshot(); });
ipcMain.handle('farm:stop', () => { stopFarm(); return snapshot(); });
ipcMain.handle('farm:pause', (_e, on) => { setPaused(!!on); return snapshot(); });
ipcMain.handle('farm:stopTask', (_e, id) => {
  fs.mkdirSync(path.join(FARM, 'inbox'), { recursive: true });
  fs.writeFileSync(path.join(FARM, 'inbox', `stop-${taskFile(id) && id}`), '');
  return true;
});
ipcMain.handle('farm:setStatus', (_e, id, status) => {
  if (!['idea', 'ready', 'done'].includes(status)) throw new Error('not allowed');
  const t = tasks.load(taskFile(id));
  const changes = { status };
  if (status === 'ready' && ['needs-you', 'failed'].includes(t.meta.status)) changes.tries = 0;
  tasks.update(t, changes, status === 'ready' ? 'set to ready by you in the app.' : status === 'done' ? 'marked done by you in the app.' : 'put on hold by you in the app.');
  return true;
});
ipcMain.handle('farm:addTask', (_e, t) => {
  const doneWhen = String(t.doneWhen || '').split(/\r?\n/).map(l => l.trim().replace(/^[-*]\s*/, '').match(/^(cmd|judge)\s*:\s*(.+)$/i))
    .filter(Boolean).map(m => ({ type: m[1].toLowerCase(), text: m[2].trim() }));
  const made = tasks.create(path.join(FARM, 'tasks'), {
    title: String(t.title || '').trim() || 'New task', todo: String(t.todo || '').trim(), doneWhen,
    kind: ['code', 'marketing', 'research', 'hardware-check'].includes(t.kind) ? t.kind : 'code',
    priority: Math.min(5, Math.max(1, Number(t.priority) || 3)), budget: t.budget === 'medium' ? 'medium' : 'small',
    status: doneWhen.length ? 'ready' : 'idea', from: 'you (app)',
  });
  const roster = crew.roster(FARM);
  if (t.agent && roster.some(a => a.id === t.agent && a.role === 'worker')) tasks.update(made, { agent: t.agent });
  return { id: made.meta.id, status: made.meta.status };
});
ipcMain.handle('farm:openField', (_e, id) => shell.openPath(path.join(FARM, 'fields', taskFile(id) && id)));
ipcMain.handle('farm:openTask', (_e, id) => shell.openPath(taskFile(id)));
ipcMain.handle('farm:openFolder', (_e, sub) => shell.openPath(path.join(FARM, ['logs', 'tasks', 'runs', 'summaries'].includes(sub) ? sub : '')));
ipcMain.handle('farm:refreshSummary', () => {
  const r = spawnSync(nodeExe() || process.execPath, [MANAGER, 'summary'], { encoding: 'utf8', env: { ...cleanEnv(), ...(nodeExe() ? {} : { ELECTRON_RUN_AS_NODE: '1' }) }, windowsHide: true });
  return r.stdout || '';
});
ipcMain.handle('farm:sendMessage', (_e, agentId, text) => {
  if (!crew.roster(FARM).some(a => a.id === agentId)) throw new Error('unknown agent');
  const msg = String(text || '').trim().slice(0, 4000);
  if (!msg) return false;
  fs.mkdirSync(path.join(FARM, 'inbox'), { recursive: true });
  fs.writeFileSync(path.join(FARM, 'inbox', `msg-${agentId}-${Date.now()}.json`), JSON.stringify({ text: msg }));
  if (!managerInfo().running) crew.say(FARM, agentId, 'system', 'The farm is off, so I will read this when it is turned on.');
  return true;
});
ipcMain.handle('farm:chatLog', (_e, agentId) => {
  const log = crew.chatLog(FARM, agentId, 100);
  // Messages still waiting in the inbox show up right away too.
  return log;
});
ipcMain.handle('farm:getGoals', () => readText(path.join(FARM, 'goals.md')));
ipcMain.handle('farm:setGoals', (_e, text) => { fs.writeFileSync(path.join(FARM, 'goals.md'), String(text || '')); return true; });
ipcMain.handle('farm:planNow', () => { fs.mkdirSync(path.join(FARM, 'inbox'), { recursive: true }); fs.writeFileSync(path.join(FARM, 'inbox', 'plan-now'), ''); return true; });
ipcMain.handle('farm:setStartWithWindows', (_e, on) => {
  app.setLoginItemSettings({ openAtLogin: !!on, args: LOGIN_ARGS });
  return app.getLoginItemSettings({ args: LOGIN_ARGS }).openAtLogin;
});
ipcMain.handle('farm:setConfig', (_e, c) => {
  const f = path.join(FARM, 'farm.config.json');
  const cfg = readJson(f, {}) || {};
  if (c.weeklyReserve !== undefined) cfg.weeklyReserve = Math.min(90, Math.max(25, Number(c.weeklyReserve) || 30));
  if (c.maxFarmhands !== undefined) cfg.maxFarmhands = Math.min(5, Math.max(1, Number(c.maxFarmhands) || 3));
  fs.writeFileSync(f, JSON.stringify(cfg, null, 2));
  // The manager reads its config at start: restart it so the change applies now.
  if (child) { killTree(child.pid); }
  return snapshot();
});

// ---------- start ----------
app.whenReady().then(() => {
  if (process.platform === 'win32') app.setAppUserModelId('Proofline Farm');
  tray = new Tray(nativeImage.createFromBuffer(icon.png(32, 'off')));
  tray.on('click', showWindow);
  createWindow();
  wantRunning = prefs().farmOn !== false;
  if (wantRunning) startFarm();
  updateTray();
  setInterval(updateTray, 5000);
});

app.on('window-all-closed', e => e.preventDefault());
app.on('before-quit', () => { quitting = true; stopFarmOnQuit(); });
