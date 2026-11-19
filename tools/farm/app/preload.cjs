// The only things the app's screen may ask the main process to do.
'use strict';
const { contextBridge, ipcRenderer } = require('electron');

const call = (name, ...args) => ipcRenderer.invoke(`farm:${name}`, ...args);

contextBridge.exposeInMainWorld('farm', {
  snapshot: () => call('snapshot'),
  start: () => call('start'),
  stop: () => call('stop'),
  pause: on => call('pause', on),
  stopTask: id => call('stopTask', id),
  setStatus: (id, status) => call('setStatus', id, status),
  addTask: t => call('addTask', t),               // { title, todo, doneWhen, kind, priority, budget, agent? }
  openField: id => call('openField', id),
  openTask: id => call('openTask', id),
  openFolder: sub => call('openFolder', sub),
  refreshSummary: () => call('refreshSummary'),
  setStartWithWindows: on => call('setStartWithWindows', on),
  setConfig: c => call('setConfig', c),
  // agents
  sendMessage: (agentId, text) => call('sendMessage', agentId, text),   // talk to one agent (live if it is working)
  chatLog: agentId => call('chatLog', agentId),                        // [{ at, from: 'you'|'agent'|'system', text }]
  getGoals: () => call('getGoals'),                                     // the Foreman's goals.md text
  setGoals: text => call('setGoals', text),
  planNow: () => call('planNow'),                                       // ask the Foreman to plan at the next tick
});

// Business HQ: the Worker's back office. The admin token stays in the main process.
contextBridge.exposeInMainWorld('biz', {
  link: () => ipcRenderer.invoke('biz:link'),                           // { apiUrl, hasToken, source, canEncrypt }
  setLink: c => ipcRenderer.invoke('biz:setLink', c),                   // { apiUrl?, token? }
  get: (what, arg) => ipcRenderer.invoke('biz:get', what, arg),          // health | business | stripe | orders | mail | mailGet
  act: (what, args) => ipcRenderer.invoke('biz:act', what, args),        // ship | saveDraft | ignore | reopen | send | redraft | getFacts | setFacts | open
});
