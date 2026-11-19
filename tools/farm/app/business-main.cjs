// Business HQ, main-process side: keeps the Worker admin token, talks to the Worker's back office for the screen,
// and pops a Windows notification when a reply waits for Nirav's OK or a new order comes in.
//
// The token is stored encrypted with Electron safeStorage (Windows DPAPI: only this Windows user can read it) in
// <farm>/business-link.json. If none is saved yet, %USERPROFILE%\proofline-admin-token.txt is used when it exists.
// The screen never sees the token; the manager gets it in PROOFLINE_ADMIN_TOKEN and strips it from every AI run.
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');

module.exports = function setupBusiness({ ipcMain, safeStorage, shell, FARM, FARM_TOOLS, notify, restartManager }) {
  const office = require(path.join(FARM_TOOLS, 'lib', 'office.cjs'));
  const penny = require(path.join(FARM_TOOLS, 'lib', 'penny.cjs'));
  const LINK = path.join(FARM, 'business-link.json');
  const TOKEN_FILE = path.join(os.homedir(), 'proofline-admin-token.txt');
  const readJson = (f, d = null) => { try { return JSON.parse(fs.readFileSync(f, 'utf8').replace(/^﻿/, '')); } catch { return d; } };

  function link() {
    const saved = readJson(LINK, {}) || {};
    let token = '', source = 'none';
    if (saved.token && safeStorage.isEncryptionAvailable()) {
      try { token = safeStorage.decryptString(Buffer.from(saved.token, 'base64')); source = 'saved'; } catch { token = ''; }
    }
    if (!token) {
      try { token = fs.readFileSync(TOKEN_FILE, 'utf8').split(/\r?\n/)[0].trim(); if (token) source = 'file'; } catch { /* none */ }
    }
    return { apiUrl: String(saved.apiUrl || office.DEFAULT_API).replace(/\/+$/, ''), token, source };
  }
  /** Extra environment for the manager process. */
  function managerEnv() {
    const l = link();
    return l.token ? { PROOFLINE_API_URL: l.apiUrl, PROOFLINE_ADMIN_TOKEN: l.token } : { PROOFLINE_API_URL: l.apiUrl };
  }
  const api = () => { const l = link(); if (!l.token) throw new Error('Add the Worker admin token in Business HQ -> Settings first.'); return office.client(l); };
  const out = r => (r.ok ? r.data : { error: r.error || 'failed', status: r.status });

  ipcMain.handle('biz:link', () => {
    const l = link();
    return { apiUrl: l.apiUrl, hasToken: !!l.token, source: l.source, canEncrypt: safeStorage.isEncryptionAvailable(), tokenFile: TOKEN_FILE };
  });
  ipcMain.handle('biz:setLink', (_e, c = {}) => {
    const saved = readJson(LINK, {}) || {};
    if (c.apiUrl !== undefined) {
      const u = String(c.apiUrl).trim().replace(/\/+$/, '');
      if (!/^https:\/\/[A-Za-z0-9.-]+(:\d+)?$/.test(u) && !/^http:\/\/(localhost|127\.0\.0\.1)(:\d+)?$/.test(u)) throw new Error('The address must look like https://your-site.com');
      saved.apiUrl = u;
    }
    if (c.token !== undefined) {
      const t = String(c.token).trim();
      if (t === '') delete saved.token;
      else {
        if (t.length < 32 || /\s/.test(t)) throw new Error('That token is too short. It is the ADMIN_TOKEN you set on the Worker (32+ characters).');
        if (!safeStorage.isEncryptionAvailable()) throw new Error('Windows encryption is not available, so the token was not saved.');
        saved.token = safeStorage.encryptString(t).toString('base64');
      }
    }
    fs.mkdirSync(FARM, { recursive: true });
    fs.writeFileSync(LINK, JSON.stringify(saved, null, 2));
    restartManager();   // the manager reads the link at start
    return true;
  });

  const GETS = {
    health: () => api().health(),
    business: async () => { const r = await api().business(); if (r.ok && r.data) r.data.steps = office.setupSteps(r.data); return r; },
    stripe: () => api().stripe(),
    orders: view => api().orders(view),
    mail: view => api().mail(view),
    mailGet: id => api().mailGet(id),
  };
  ipcMain.handle('biz:get', async (_e, what, arg) => {
    if (!GETS[what]) throw new Error('not allowed');
    try { return out(await GETS[what](arg)); } catch (e) { return { error: e.message }; }
  });

  ipcMain.handle('biz:act', async (_e, what, a = {}) => {
    try {
      switch (what) {
        case 'ship': return out(await api().ship(a.id, String(a.carrier || ''), String(a.tracking || ''), a.notify !== false));
        case 'saveDraft': return out(await api().mailDraft(a.id, { draft: String(a.text || '') }));
        case 'ignore': return out(await api().mailIgnore(a.id));
        case 'reopen': return out(await api().mailReopen(a.id));
        case 'send': {
          // Last guard on the way out: Penny's [[ask Nirav: ...]] notes must be filled in first.
          if (penny.hasPlaceholders(a.text)) return { error: 'The reply still has a [[...]] note from Penny. Replace it with the real answer first.' };
          return out(await api().mailSend(a.id, String(a.text || '')));
        }
        case 'redraft': {
          if (!/^mail_[0-9a-f]{16}$/.test(String(a.id))) throw new Error('bad mail id');
          fs.mkdirSync(path.join(FARM, 'inbox'), { recursive: true });
          fs.writeFileSync(path.join(FARM, 'inbox', `redraft-${a.id}.json`), JSON.stringify({ note: String(a.note || '').slice(0, 1000) }));
          return { ok: true };
        }
        case 'getFacts': return { text: penny.ensureFacts(FARM) };
        case 'setFacts': fs.writeFileSync(path.join(FARM, 'penny-facts.md'), String(a.text || '').slice(0, 20000)); return { ok: true };
        case 'open': {
          const URLS = {
            stripe: 'https://dashboard.stripe.com/', stripePayments: 'https://dashboard.stripe.com/payments', stripePayouts: 'https://dashboard.stripe.com/payouts',
            cloudflare: 'https://dash.cloudflare.com/', resend: 'https://resend.com/emails', pirateship: 'https://www.pirateship.com/', site: link().apiUrl,
          };
          if (!URLS[a.to]) throw new Error('not allowed');
          await shell.openExternal(URLS[a.to]);
          return { ok: true };
        }
        default: throw new Error('not allowed');
      }
    } catch (e) { return { error: e.message }; }
  });

  // Windows notifications from the manager's business feed: a reply to approve, a new order.
  let seen = null;
  setInterval(() => {
    const feed = (readJson(path.join(FARM, 'state.json'), {}) || {}).feed || {};
    const n = (feed.business || {}).numbers;
    if (!n) return;
    const now = { drafted: (n.mail || {}).drafted || 0, orders: ((n.sales || {}).all || {}).orders || 0, cents: ((n.sales || {}).all || {}).cents || 0 };
    if (seen) {
      if (now.drafted > seen.drafted) notify('Penny drafted a reply', 'A customer reply waits for your OK in Business HQ.');
      if (now.orders > seen.orders) notify(`New order! (${now.orders - seen.orders})`, `Total sales now ${office.usd(now.cents)}. Business HQ -> Orders.`);
    }
    seen = now;
  }, 20000);

  return { managerEnv, link };
};
