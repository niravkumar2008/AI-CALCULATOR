// Business HQ: the screen for running Proofline (orders, shipping, customer mail, money, the website, launch steps).
// Data comes from the Worker's back office through the main process (window.biz). Customer text is only ever set
// with textContent. In #demo mode it shows made-up numbers.
'use strict';
const Biz = (() => {
  const TABS = ['today', 'inbox', 'orders', 'money', 'cloud', 'launch', 'settings'];
  let tab = 'today', data = { at: 0 }, loading = false, openMail = null, mailView = 'open', ordersView = 'to_ship', timer = null;
  const usd = c => `$${(Number(c || 0) / 100).toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`;
  const date = sec => sec ? new Date(sec * 1000).toLocaleDateString(undefined, { month: 'short', day: 'numeric' }) : '–';
  const dateTime = sec => sec ? new Date(sec * 1000).toLocaleString(undefined, { month: 'short', day: 'numeric', hour: 'numeric', minute: '2-digit' }) : '–';
  const daysAgo = sec => sec ? Math.floor((Date.now() / 1000 - sec) / 86400) : 0;
  const isDemo = () => (typeof demo !== 'undefined' && demo) || !window.biz;

  // ---------- data ----------
  const bapi = {
    get: (what, arg) => isDemo() ? Promise.resolve(DEMO.get(what, arg)) : window.biz.get(what, arg),
    act: (what, a) => isDemo() ? Promise.resolve(DEMO.act(what, a)) : window.biz.act(what, a),
    link: () => isDemo() ? Promise.resolve({ apiUrl: 'https://ai-calc-api.ai-calc-api.workers.dev', hasToken: true, source: 'saved', canEncrypt: true }) : window.biz.link(),
    setLink: c => isDemo() ? Promise.resolve(true) : window.biz.setLink(c),
  };
  async function load(force = false) {
    if (loading || (!force && Date.now() - data.at < 25000)) return;
    loading = true;
    try {
      const l = await bapi.link();
      data.link = l;
      if (!l.hasToken) { data.at = Date.now(); data.noLink = true; return; }
      data.noLink = false;
      const [health, business, mail, orders, stripe] = await Promise.all([
        bapi.get('health'), bapi.get('business'), bapi.get('mail', mailView), bapi.get('orders', ordersView),
        tab === 'money' || !data.stripe ? bapi.get('stripe') : Promise.resolve(data.stripe),
      ]);
      Object.assign(data, { health, business, mail, orders, stripe, at: Date.now() });
    } finally { loading = false; render(); }
  }

  // ---------- small UI bits ----------
  const h = (tag, cls, text) => el(tag, cls, text);
  function kpi(label, value, sub, cls = '') {
    const k = h('div', `kpi ${cls}`);
    k.append(h('div', 'kpi-label', label), h('div', 'kpi-value', value));
    if (sub) k.append(h('div', 'kpi-sub', sub));
    return k;
  }
  function section(title, ...kids) { const s = h('section', 'biz-sec'); s.append(h('h3', null, title), ...kids); return s; }
  function linkBtn(label, to) { return btn(label, () => bapi.act('open', { to }), 'gbtn small'); }
  function note(text, cls = 'muted small') { return h('p', cls, text); }
  function errBox(r) { return r && r.error ? h('div', 'biz-err', r.error) : null; }
  function row(...kids) { const r = h('div', 'row wrap'); r.append(...kids.filter(Boolean)); return r; }

  // ---------- tabs ----------
  function render() {
    if ($('ov-biz').hidden) return;
    document.querySelectorAll('#bizTabs button').forEach(b => b.classList.toggle('active', b.dataset.btab === tab));
    const body = $('bizBody');
    const keep = body.scrollTop;
    body.replaceChildren();
    const inbox = data.business && data.business.mail ? (data.business.mail.new || 0) + (data.business.mail.drafted || 0) : 0;
    $('bizInboxBadge').hidden = !inbox; $('bizInboxBadge').textContent = String(inbox);
    const ship = data.business && data.business.orders ? data.business.orders.to_ship.orders : 0;
    $('bizShipBadge').hidden = !ship; $('bizShipBadge').textContent = String(ship);
    $('bizUpdated').textContent = data.at ? `Updated ${new Date(data.at).toLocaleTimeString(undefined, { hour: 'numeric', minute: '2-digit' })}${isDemo() ? ' · demo numbers' : ''}` : 'Loading…';
    if (data.noLink && tab !== 'settings') {
      body.append(section('Connect the farm to your website',
        note('Business HQ reads orders, mail and money from your Cloudflare Worker. It needs the Worker\'s admin token once.', 'small'),
        btn('Open Settings', () => go('biz=settings'), 'gbtn primary')));
      return;
    }
    const fn = { today: renderToday, inbox: renderInbox, orders: renderOrders, money: renderMoney, cloud: renderCloud, launch: renderLaunch, settings: renderSettings }[tab];
    if (data.at || tab === 'settings') fn(body); else body.append(note('Loading the business numbers…'));
    body.scrollTop = keep;
  }

  function renderToday(body) {
    const b = data.business;
    if (!b || b.error) { body.append(errBox(b) || note('No numbers yet.')); return; }
    const s = b.sales, st = b.store, pl = b.plan, w = b.waitlist;
    const grid = h('div', 'kpis');
    grid.append(
      kpi('Sales today', usd(s.day.cents), `${s.day.orders} order${s.day.orders === 1 ? '' : 's'}`),
      kpi('Last 7 days', usd(s.week.cents), `${s.week.orders} orders`),
      kpi('All time', usd(s.all.cents), `${s.all.orders} orders${s.all.refunded_cents ? ` · ${usd(s.all.refunded_cents)} refunded` : ''}`),
      kpi('To ship', String(b.orders.to_ship.units || 0), b.orders.to_ship.orders ? `${b.orders.to_ship.orders} orders · oldest ${daysAgo(b.orders.to_ship.oldest)} d` : 'nothing waiting', b.orders.to_ship.orders ? 'warn' : ''),
      kpi('Store', st.open ? 'Open' : 'Closed', st.units_left != null ? `${st.units_left} of ${st.stock} left` : 'no stock limit', st.open ? 'good' : ''),
      kpi('AI plan', String(pl.active), `${pl.monthly} monthly · ${pl.yearly} yearly · ~${usd((pl.monthly * 15 + pl.yearly * 10) * 100)}/mo`),
      kpi('Waitlist', String(w.total), `+${w.week} this week`),
      kpi('Inbox', String((b.mail.new || 0) + (b.mail.drafted || 0)), `${b.mail.drafted || 0} drafts to approve`, b.mail.drafted ? 'warn' : ''),
    );
    body.append(grid);
    const alerts = (M && M.attention || []).filter(a => a.kind === 'biz');
    const list = h('div', 'biz-alerts');
    if (!alerts.length) list.append(note('Nothing urgent. Nice.'));
    for (const a of alerts) { const r = h('button', `biz-alert ${a.level || ''}`, a.text); r.addEventListener('click', () => go(`biz=${a.go || 'today'}`)); list.append(r); }
    body.append(section('Needs you', list));
    if (w.sources_30d && w.sources_30d.length) {
      const t = h('div', 'bars');
      const max = Math.max(...w.sources_30d.map(x => x.n));
      for (const x of w.sources_30d) { const r = h('div', 'bar-row'); const bar = h('div', 'bar'); bar.style.width = `${Math.max(4, x.n / max * 100)}%`; r.append(h('span', 'bar-label', x.source), bar, h('span', 'bar-n', String(x.n))); t.append(r); }
      body.append(section('Where waitlist sign-ups come from (30 days)', t, note('Add ?utm_source=tiktok (or youtube, a creator code…) to the links you post, and they show up here.')));
    }
    body.append(section('Quick links', row(linkBtn('Your website', 'site'), linkBtn('Stripe payments', 'stripePayments'), linkBtn('Cloudflare', 'cloudflare'), linkBtn('Pirate Ship (labels)', 'pirateship'), linkBtn('Sent e-mail (Resend)', 'resend'))));
  }

  // ----- inbox -----
  function renderInbox(body) {
    const r = data.mail;
    const top = row(
      h('span', 'muted small', 'Customer e-mail to your hello@ address. Penny drafts a reply; nothing is sent until you press Approve & send.'),
      selectView(), btn("Penny's facts", editFacts, 'gbtn small'), btn('Refresh', () => load(true), 'gbtn small'),
    );
    body.append(top);
    if (r && r.error) { body.append(errBox(r)); return; }
    const list = (r && r.mail) || [];
    const wrap = h('div', 'inbox');
    const left = h('div', 'inbox-list');
    if (!list.length) left.append(note(mailView === 'all' ? 'No mail yet.' : 'Inbox zero. New customer mail shows up here within a minute.'));
    for (const m of list) {
      const item = h('button', `mail-item${openMail && openMail.mail.id === m.id ? ' active' : ''} st-${m.status}`);
      const tag = m.status === 'drafted' ? (m.draft ? 'DRAFT READY' : 'NO REPLY?') : m.status === 'new' ? 'PENNY IS ON IT' : m.status.toUpperCase();
      item.append(h('span', 'mail-tag', tag), h('b', null, m.from_name || m.from), h('span', 'mail-subj', m.subject || '(no subject)'), h('span', 'muted small', `${dateTime(m.received_at)}${m.category ? ' · ' + m.category : ''}`));
      if (m.summary) item.append(h('span', 'mail-sum', m.summary));
      item.addEventListener('click', () => openOne(m.id));
      left.append(item);
    }
    const right = h('div', 'inbox-detail');
    if (openMail) renderMailDetail(right); else right.append(note(list.length ? 'Pick an e-mail on the left.' : ''));
    wrap.append(left, right);
    body.append(wrap);
  }
  function selectView() {
    const s = h('select', 'small');
    for (const [v, t] of [['open', 'Waiting for me'], ['all', 'All mail']]) { const o = h('option', null, t); o.value = v; s.append(o); }
    s.value = mailView;
    s.addEventListener('change', () => { mailView = s.value; openMail = null; load(true); });
    return s;
  }
  async function openOne(id) {
    const d = await bapi.get('mailGet', id);
    if (d && !d.error) openMail = d; else toast(d.error || 'Could not open that mail.');
    render();
  }
  function renderMailDetail(box) {
    const { mail: m, thread, customer } = openMail;
    box.append(h('h3', null, m.subject || '(no subject)'), h('div', 'muted small', `From ${m.from_name ? m.from_name + ' <' + m.from + '>' : m.from} · ${dateTime(m.received_at)}`));
    if (m.summary) box.append(h('div', 'mail-penny', `Penny: ${m.summary}${m.category ? ` (${m.category})` : ''}`));
    const ctx = h('div', 'mail-ctx');
    const orders = (customer && customer.orders) || [];
    ctx.append(h('b', null, 'This customer: '), h('span', null, orders.length ? orders.map(o => `${o.quantity}× ${o.status}${o.shipped_at ? `, shipped ${date(o.shipped_at)} (${o.carrier} ${o.tracking})` : ', not shipped'} · ${date(o.created_at)}`).join(' | ') : 'no orders under this address'),
      h('span', null, customer && customer.plan ? ` · AI plan ${customer.plan.status}` : ''), h('span', null, customer && customer.on_waitlist_since ? ` · waitlist since ${date(customer.on_waitlist_since)}` : ''));
    box.append(ctx);
    for (const t of thread || []) {
      const old = h('details', 'mail-old');
      old.append(h('summary', null, `Earlier: ${dateTime(t.received_at)}${t.reply_text ? ' (you replied)' : ''}`), h('pre', 'mail-body', t.body || ''));
      if (t.reply_text) old.append(h('pre', 'mail-body reply', `You: ${t.reply_text}`));
      box.append(old);
    }
    box.append(h('pre', 'mail-body', m.body || '(empty)'));
    if ((m.attachments || []).length) box.append(note(`Attachments (not kept, see the copy in your Gmail): ${m.attachments.map(a => a.name).join(', ')}`));
    if (m.status === 'replied') { box.append(h('div', 'mail-sent', `Sent ${dateTime(m.replied_at)}:`), h('pre', 'mail-body reply', m.reply_text || '')); return; }
    if (m.status === 'ignored') { box.append(row(btn('Reopen', async () => { await act('reopen', { id: m.id }); }, 'gbtn small'))); return; }
    if (m.auto) { box.append(note('This is an automatic mail (auto-reply, bounce or newsletter), so it can\'t be answered here.')); return; }
    box.append(h('h4', null, m.draft ? 'Penny\'s draft (edit it freely):' : m.status === 'new' ? 'Penny is writing a draft… (or write your own)' : 'Penny suggests no reply. Write one if you like:'));
    const ta = h('textarea', 'mail-draft'); ta.rows = 10; ta.maxLength = 8000; ta.value = m.draft || '';
    const warn = h('div', 'biz-err'); warn.hidden = true;
    const check = () => { const ph = /\[\[[^\]]*\]\]/.test(ta.value); warn.hidden = !ph; warn.textContent = ph ? 'Replace the [[...]] notes with real answers before sending.' : ''; send.disabled = ph || !ta.value.trim(); };
    ta.addEventListener('input', check);
    const send = btn('Approve & send', async () => {
      if (!confirm(`Send this reply to ${m.from}?`)) return;
      const r = await bapi.act('send', { id: m.id, text: ta.value });
      if (r && r.error) { toast(r.error); return; }
      toast(`Sent to ${m.from}.`); openMail = null; await load(true);
    }, 'gbtn primary');
    const noteIn = h('input', 'small'); noteIn.placeholder = 'Note for Penny (e.g. "offer a replacement")'; noteIn.maxLength = 300;
    box.append(ta, warn, row(send,
      btn('Save my edit', async () => { await act('saveDraft', { id: m.id, text: ta.value }); toast('Saved.'); }, 'gbtn small'),
      btn('Ignore', async () => { await act('ignore', { id: m.id }); openMail = null; }, 'gbtn small')),
      row(noteIn, btn('Ask Penny to redo it', async () => { await bapi.act('redraft', { id: m.id, note: noteIn.value }); toast('Penny will redo it within a minute.'); }, 'gbtn small')));
    check();
  }
  async function act(what, a) {
    const r = await bapi.act(what, a);
    if (r && r.error) { toast(r.error); return r; }
    if (r && r.mail && openMail && openMail.mail.id === r.mail.id) openMail.mail = { ...openMail.mail, ...r.mail };
    await load(true);
    return r;
  }
  async function editFacts() {
    const r = await bapi.act('getFacts');
    const dlg = h('div', 'facts');
    const ta = h('textarea', 'mono'); ta.rows = 18; ta.value = (r && r.text) || '';
    dlg.append(h('h3', null, "Penny's facts"), note('Penny only uses what is written here (plus each customer\'s order). Keep it true: prices, ship time, refunds, warranty.'), ta,
      row(btn('Save', async () => { await bapi.act('setFacts', { text: ta.value }); toast('Saved. Penny uses it from the next draft.'); render(); }, 'gbtn primary'), btn('Close', () => render(), 'gbtn small')));
    $('bizBody').replaceChildren(dlg);
  }

  // ----- orders -----
  function renderOrders(body) {
    const r = data.orders;
    const s = h('select', 'small');
    for (const [v, t] of [['to_ship', 'To ship'], ['recent', 'All recent orders']]) { const o = h('option', null, t); o.value = v; s.append(o); }
    s.value = ordersView;
    s.addEventListener('change', () => { ordersView = s.value; load(true); });
    body.append(row(s, h('span', 'muted small', 'Print the label in Pirate Ship, then paste the tracking number here: the customer gets a "your calculator shipped" e-mail with the tracking link.'), linkBtn('Open Pirate Ship', 'pirateship')));
    if (r && r.error) { body.append(errBox(r)); return; }
    const list = (r && r.orders) || [];
    if (!list.length) { body.append(note(ordersView === 'to_ship' ? 'Nothing to ship right now.' : 'No orders yet.')); return; }
    for (const o of list) {
      const card = h('div', `order-card${o.shipped_at ? ' shipped' : ''}`);
      const age = daysAgo(o.created_at);
      card.append(
        h('div', 'order-head', `${o.quantity} × Proofline · ${usd(o.cents)} · ${date(o.created_at)}${o.shipped_at ? '' : ` · waiting ${age} day${age === 1 ? '' : 's'}`}`),
        h('div', null, o.ship_to ? `${o.ship_to.name}, ${o.ship_to.address}` : '(no address on this order)'),
        h('div', 'muted small', `${o.email || ''} · ${o.status}${o.refunded_cents ? ` · refunded ${usd(o.refunded_cents)}` : ''} · ${o.id.slice(-10)}`));
      if (o.shipped_at) card.append(h('div', 'mail-sent', `Shipped ${date(o.shipped_at)} · ${String(o.carrier || '').toUpperCase()} ${o.tracking || ''}${o.notified_at ? ' · customer e-mailed' : ' · customer NOT e-mailed'}`));
      if (!o.shipped_at && ['paid', 'partially_refunded'].includes(o.status)) {
        const car = h('select', 'small');
        for (const c of ['usps', 'ups', 'fedex', 'dhl', 'other']) { const op = h('option', null, c.toUpperCase()); op.value = c; car.append(op); }
        const tr = h('input', 'small'); tr.placeholder = 'Tracking number'; tr.maxLength = 60;
        card.append(row(car, tr, btn('Mark shipped & e-mail tracking', async () => {
          const res = await bapi.act('ship', { id: o.id, carrier: car.value, tracking: tr.value });
          if (res && res.error) { toast(res.error); return; }
          toast(res.mailed ? 'Marked shipped. The customer got the tracking e-mail.' : `Marked shipped, but the e-mail didn't go out: ${res.mail_problem}`);
          await load(true);
        }, 'gbtn primary small')));
      }
      body.append(card);
    }
  }

  // ----- money -----
  function renderMoney(body) {
    const st = data.stripe, b = data.business;
    const grid = h('div', 'kpis');
    if (st && st.ok) {
      grid.append(kpi('Ready to pay out', usd(st.available_cents), `Stripe ${st.mode} mode`, 'good'), kpi('On its way', usd(st.pending_cents), 'pending (clears in ~2 business days)'));
    } else body.append(errBox({ error: (st && (st.why || st.error)) || 'Stripe numbers are not available yet.' }));
    if (b && b.sales) grid.append(kpi('Sales, 30 days', usd(b.sales.month.cents), `${b.sales.month.orders} orders`), kpi('Refunded (all time)', usd(b.sales.all.refunded_cents), ''));
    if (b && b.plan) grid.append(kpi('AI plan, monthly value', usd((b.plan.monthly * 15 + b.plan.yearly * 10) * 100), `${b.plan.active} active · ${b.plan.ending} ending · ${b.plan.past_due} payment problems`));
    body.append(grid);
    if (st && st.ok) {
      const t = h('div', 'tbl');
      if (!st.payouts.length) t.append(note('No payouts yet. Stripe sends money to your bank on its schedule (usually daily, after a first wait of about a week).'));
      for (const p of st.payouts) t.append(row(h('span', null, date(p.arrival)), h('b', null, usd(p.cents)), h('span', 'muted', p.status)));
      if (st.payouts_error) t.append(note(st.payouts_error));
      body.append(section('Payouts to your bank', t));
    }
    body.append(note('Totals are before Stripe fees (about 2.9% + 30¢ per card payment) and before sales tax you owe the state. Keep 25–30% aside for taxes.'),
      row(linkBtn('Stripe payments', 'stripePayments'), linkBtn('Stripe payouts', 'stripePayouts')));
  }

  // ----- cloud -----
  function renderCloud(body) {
    const hl = data.health, b = data.business;
    const grid = h('div', 'kpis');
    grid.append(kpi('Website & API', hl && !hl.error ? 'Up' : 'DOWN', hl && !hl.error ? 'answering' : (hl && hl.error) || '', hl && !hl.error ? 'good' : 'bad'));
    if (b && b.ai) {
      grid.append(
        kpi('AI solves today', String(b.ai.solves_today), `${usd(b.ai.cost_today_usd * 100)} of ${usd(b.ai.daily_cap_usd * 100)} daily cap`, b.ai.cost_today_usd > b.ai.daily_cap_usd * 0.8 ? 'bad' : ''),
        kpi('AI this month', String(b.ai.solves_month), `${usd(b.ai.cost_month_usd * 100)} AI cost`),
        kpi('Accounts', String(b.people.accounts), `${b.people.calculators_linked} calculators linked`),
        kpi('Calculators active', String(b.people.calculators_active_7d), 'used in the last 7 days'));
    }
    body.append(grid);
    if (b && b.setup) body.append(note(`Site address: ${b.setup.public_url}${b.setup.custom_domain ? '' : ' (free workers.dev address; move to your own domain before marketing)'}`));
    body.append(note('Free Workers plan: 100,000 requests a day. Upgrade to Workers Paid ($5/month) before the Thanksgiving push so a busy day can\'t hit the cap.'),
      row(linkBtn('Cloudflare dashboard', 'cloudflare'), linkBtn('Open the website', 'site')));
  }

  // ----- launch -----
  function renderLaunch(body) {
    const steps = (data.business && data.business.steps) || [];
    const done = steps.filter(s => s.done).length;
    body.append(note(`${done} of ${steps.length} launch steps done (read from your Worker's real settings). The full guide is docs/web/BACK_OFFICE.md and the launch checklist page.`, 'small'));
    const list = h('div', 'steps');
    for (const s of steps) { const r = h('div', `step ${s.done ? 'done' : ''}`); r.append(h('span', 'step-box', s.done ? '✓' : ''), h('span', null, s.text)); list.append(r); }
    body.append(list);
  }

  // ----- settings -----
  async function renderSettings(body) {
    const l = data.link || await bapi.link();
    const url = h('input'); url.value = l.apiUrl || '';
    const tok = h('input'); tok.type = 'password'; tok.placeholder = l.hasToken ? '(saved — type a new one to replace it)' : 'Paste the ADMIN_TOKEN you set on the Worker'; tok.autocomplete = 'off';
    const msg = h('span', 'muted small', '');
    const status = l.hasToken ? (l.source === 'file' ? `Using the token from ${l.tokenFile || 'proofline-admin-token.txt'}. Save it here to keep it encrypted instead.` : 'Token saved (encrypted with your Windows login).') : 'No token yet: Business HQ can\'t read your business.';
    body.append(section('Connect to your website (the Cloudflare Worker)',
      h('label', null, 'Worker address'), url,
      h('label', null, 'Admin token'), tok,
      note(status),
      row(btn('Save & reconnect', async () => {
        try { await bapi.setLink({ apiUrl: url.value, ...(tok.value ? { token: tok.value } : {}) }); tok.value = ''; msg.textContent = 'Saved. The farm restarts its link in a few seconds.'; data.at = 0; setTimeout(() => load(true), 4000); }
        catch (e) { msg.textContent = String(e.message || e).replace(/^Error invoking remote method '[^']+': (Error: )?/, ''); }
      }, 'gbtn primary'), msg),
      note('The token never goes to Penny or any AI run: only this app and the farm manager use it to call your Worker. Your own domain later? Change the address here.')));
  }

  // ---------- open / close ----------
  function show(t) {
    tab = TABS.includes(t) ? t : 'today';
    if (tab !== 'inbox') openMail = null;
    render();
    load(true);
    clearInterval(timer);
    timer = setInterval(() => { if ($('ov-biz').hidden) { clearInterval(timer); return; } if (tab !== 'inbox' || !openMail) load(); }, 30000);
  }
  document.querySelectorAll('#bizTabs button').forEach(b => b.addEventListener('click', () => go(`biz=${b.dataset.btab}`)));

  // ---------- demo numbers (#demo) ----------
  const DEMO = (() => {
    const t = Math.floor(Date.now() / 1000), d = 86400;
    const mails = [
      { id: 'mail_00000000000000a1', received_at: t - 3600, from: 'jordan.lee@example.com', from_name: 'Jordan Lee', subject: 'When will my calculator ship?', thread_id: 'mail_00000000000000a1', auto: false, status: 'drafted', category: 'shipping', summary: 'Asks when order placed 9 days ago ships', draft: 'Hi Jordan,\n\nThanks for your order! Each Proofline is made to order and ships within 3 to 5 weeks of ordering, so yours is on track. I\'ll email you the tracking number the day it ships.\n\nNirav\nProofline', body: 'Hi! I ordered a Proofline last week. Any idea when it ships? I have a calc exam on Dec 12.\n\nThanks,\nJordan' },
      { id: 'mail_00000000000000a2', received_at: t - 900, from: 'mia.r@example.com', from_name: 'Mia R.', subject: 'Does it work on the SAT?', thread_id: 'mail_00000000000000a2', auto: false, status: 'drafted', category: 'sales', summary: 'Asks if the calculator is allowed on the SAT | Needs you: confirm the exam-mode wording', draft: 'Hi Mia,\n\nGreat question. [[ask Nirav: which exams is exam mode approved for?]]\n\nNirav\nProofline', body: 'Is this allowed on the SAT? My school is strict about calculators.' },
      { id: 'mail_00000000000000a3', received_at: t - 120, from: 'sam@example.com', from_name: 'Sam', subject: 'Change of address', thread_id: 'mail_00000000000000a3', auto: false, status: 'new', category: null, summary: null, draft: null, body: 'I moved. Can you ship to 22 Oak Ave instead?' },
    ];
    const orders = [
      { id: 'cs_live_demoA1', created_at: t - 9 * d, email: 'jordan.lee@example.com', status: 'paid', quantity: 1, cents: 23345, refunded_cents: 0, ship_to: { name: 'Jordan Lee', address: '14 Elm St, Carmel, IN 46032, US' }, shipped_at: null, carrier: null, tracking: null, notified_at: null },
      { id: 'cs_live_demoB2', created_at: t - 2 * d, email: 'pat@example.com', status: 'paid', quantity: 2, cents: 46690, refunded_cents: 0, ship_to: { name: 'Pat Kim', address: '9 Lake Rd, Austin, TX 78701, US' }, shipped_at: null, carrier: null, tracking: null, notified_at: null },
    ];
    const business = {
      at: t,
      store: { open: true, stock: 20, units_left: 14, ship_note: 'Made to order: ships in 3 to 5 weeks', shipping_rates: 2, promo_codes: true, tax: false },
      sales: { day: { orders: 1, cents: 23345 }, week: { orders: 3, cents: 70035 }, month: { orders: 6, cents: 140070 }, all: { orders: 6, cents: 140070, refunded_cents: 0 } },
      orders: { by_status: {}, to_ship: { orders: 2, units: 3, oldest: t - 9 * d } },
      plan: { active: 5, monthly: 4, yearly: 1, past_due: 0, ending: 1, canceled: 0 },
      waitlist: { total: 412, today: 9, week: 61, sources_30d: [{ source: 'tiktok', n: 120 }, { source: 'youtube', n: 64 }, { source: 'hero', n: 40 }, { source: 'c-alex', n: 22 }] },
      people: { accounts: 8, calculators_linked: 5, calculators_active_7d: 4 },
      ai: { solves_today: 37, cost_today_usd: 1.12, solves_month: 640, cost_month_usd: 19.4, daily_cap_usd: 50 },
      mail: { new: 1, drafted: 2, replied: 7, ignored: 3, auto: 1 },
      setup: { public_url: 'https://ai-calc-api.ai-calc-api.workers.dev', custom_domain: false, stripe_key: 'live', stripe_webhook: true, stripe_prices: { calculator: true, monthly: true, yearly: true }, resend: true, mail_from_own_domain: false, mail_reply_from: false, mail_forward_to: false, turnstile: false, anthropic: true },
    };
    business.steps = [
      { id: 'stripe', done: true, text: 'Stripe live key' }, { id: 'webhook', done: true, text: 'Stripe webhook secret' }, { id: 'prices', done: true, text: 'Stripe prices' },
      { id: 'shipping', done: true, text: 'Shipping rates' }, { id: 'store', done: true, text: 'Store open' }, { id: 'tax', done: false, text: 'Turn on sales tax (Indiana certificate, then STRIPE_TAX = "1").' },
      { id: 'domain', done: false, text: 'Put the site on your own domain (still on workers.dev).' }, { id: 'mail_in', done: false, text: 'Set up Email Routing for hello@your-domain (BACK_OFFICE guide, step 2).' },
      { id: 'mail_out', done: false, text: 'Send mail from your domain: verify it in Resend, set MAIL_FROM and MAIL_REPLY_FROM.' }, { id: 'turnstile', done: false, text: 'Turn on the bot check (Turnstile) before the Thanksgiving push.' },
    ];
    return {
      get(what, arg) {
        if (what === 'health') return { ok: true };
        if (what === 'business') return business;
        if (what === 'stripe') return { ok: true, mode: 'live', available_cents: 61210, pending_cents: 45370, payouts: [{ cents: 40990, arrival: t - 2 * d, status: 'paid' }], payouts_error: null };
        if (what === 'orders') return { view: arg, orders: arg === 'recent' ? orders.concat([{ ...orders[0], id: 'cs_live_demoC3', shipped_at: t - 3 * d, carrier: 'usps', tracking: '9400111899223344556677', notified_at: t - 3 * d, created_at: t - 20 * d }]) : orders };
        if (what === 'mail') return { view: arg, mail: mails.map(m => ({ ...m, preview: m.body.slice(0, 160) })) };
        if (what === 'mailGet') {
          const m = mails.find(x => x.id === arg) || mails[0];
          return { mail: { ...m, attachments: [] }, thread: [], customer: { orders: orders.filter(o => o.email === m.from).map(o => ({ ...o, ship_to: { name: o.ship_to.name } })), plan: null, on_waitlist_since: t - 40 * d } };
        }
        return { error: 'demo' };
      },
      act(what) {
        if (what === 'getFacts') return { text: '# Facts Penny may use in replies (demo)\n- Price: $215 + tax + shipping\n- Ships in 3 to 5 weeks' };
        if (what === 'ship') return { mailed: true };
        return { ok: true };
      },
    };
  })();

  return { show, render, load };
})();
