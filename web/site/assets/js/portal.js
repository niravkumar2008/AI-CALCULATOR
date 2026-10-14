// Proofline account portal: sign in, confirm, link a calculator, account.
// One deferred script, no dependencies. Same origin as the API (/api/*), so the session
// cookie (HttpOnly) travels on its own. User data is only ever written with textContent.
(() => {
  "use strict";

  const $ = (sel, root = document) => root.querySelector(sel);
  const page = document.body.dataset.page;

  const PATHS = { login: "/login/", link: "/link/", account: "/account/" };
  const PENDING_KEY = "portal.pendingLink";
  const PENDING_TTL_MS = 30 * 60 * 1000; // a pairing code lives 15 minutes; this is generous
  const CODE_RE = /^[A-HJ-NP-Z2-9]{8}$/;
  const NET_MSG = "Can't reach the server. Check your connection and try again.";
  const isLocal = location.hostname === "127.0.0.1" || location.hostname === "localhost";

  // ------------------------------------------------------------ API
  /**
   * Calls the API. Never throws. Resolves to {ok, status, data} or {ok:false, status, type, message}.
   * Error messages from the API are user-safe (contract), so they're shown as they are.
   */
  async function api(method, path, body) {
    const headers = { accept: "application/json" };
    const opts = { method, headers, credentials: "same-origin", cache: "no-store" };
    if (method !== "GET") {
      headers["content-type"] = "application/json";
      opts.body = JSON.stringify(body || {});
    }
    const ctl = "AbortController" in window ? new AbortController() : null;
    let timer = 0;
    if (ctl) {
      opts.signal = ctl.signal;
      timer = setTimeout(() => ctl.abort(), 20000);
    }
    let res;
    try {
      res = await fetch(path, opts);
    } catch {
      return { ok: false, status: 0, type: "network", message: NET_MSG };
    } finally {
      clearTimeout(timer);
    }
    let data = null;
    if (res.status !== 204) {
      try {
        data = await res.json();
      } catch {
        data = null;
      }
    }
    if (res.ok) return { ok: true, status: res.status, data };
    const err = data && data.type === "error" && data.error ? data.error : {};
    return {
      ok: false,
      status: res.status,
      type: typeof err.type === "string" ? err.type : "error",
      message: typeof err.message === "string" && err.message ? err.message : fallbackMsg(res.status),
    };
  }

  function fallbackMsg(status) {
    if (status === 401) return "Please sign in.";
    if (status === 429) return "Too many requests. Please try again later.";
    if (status >= 500) return "Something went wrong on our side. Please try again later.";
    return "That didn't work. Please try again.";
  }

  // ------------------------------------------------------------ small helpers
  function setMsg(el, text, tone) {
    if (!el) return;
    el.textContent = text || "";
    if (tone) el.dataset.tone = tone;
    else delete el.dataset.tone;
  }

  function busy(btn, on, label) {
    if (!btn) return;
    if (on) {
      if (!btn.dataset.label) btn.dataset.label = btn.textContent;
      btn.disabled = true;
      btn.setAttribute("aria-busy", "true");
      btn.textContent = "";
      const s = document.createElement("span");
      s.className = "spinner";
      s.setAttribute("aria-hidden", "true");
      btn.append(s, document.createTextNode(label || "Working…"));
    } else {
      btn.disabled = false;
      btn.removeAttribute("aria-busy");
      btn.textContent = btn.dataset.label || btn.textContent;
      delete btn.dataset.label;
    }
  }

  function fieldError(input, errEl, text) {
    setMsg(errEl, text);
    if (input) {
      if (text) input.setAttribute("aria-invalid", "true");
      else input.removeAttribute("aria-invalid");
    }
  }

  function focusEl(el) {
    if (!el) return;
    if (!el.hasAttribute("tabindex") && !/^(A|BUTTON|INPUT|SELECT|TEXTAREA)$/.test(el.tagName)) {
      el.setAttribute("tabindex", "-1");
    }
    el.focus({ preventScroll: false });
  }

  function show(el, on) {
    if (el) el.hidden = !on;
  }

  const fmtDate = (sec) => {
    try {
      return new Date(sec * 1000).toLocaleDateString(undefined, { day: "numeric", month: "long", year: "numeric" });
    } catch {
      return new Date(sec * 1000).toDateString();
    }
  };

  function isEmail(s) {
    return typeof s === "string" && s.length <= 254 && /^[^\s@<>]+@[^\s@<>]+\.[^\s@<>]+$/.test(s);
  }

  function normCode(s) {
    return String(s || "").toUpperCase().replace(/[\s-]/g, "");
  }
  function fmtCode(c) {
    return c.length > 4 ? c.slice(0, 4) + "-" + c.slice(4, 8) : c;
  }

  // Pending link: remembered across the sign-in round trip. The sign-in link opens in a new
  // tab from the email, where sessionStorage is empty, so this uses localStorage with an
  // expiry and is cleared as soon as it's used. Storage may be blocked: then it's skipped.
  function savePending(code) {
    try {
      localStorage.setItem(PENDING_KEY, JSON.stringify({ code: code || "", t: Date.now() }));
    } catch {
      /* storage blocked: the user can type the code again */
    }
  }
  function readPending() {
    try {
      const raw = localStorage.getItem(PENDING_KEY);
      if (!raw) return null;
      const v = JSON.parse(raw);
      if (!v || typeof v.t !== "number" || Date.now() - v.t > PENDING_TTL_MS) {
        localStorage.removeItem(PENDING_KEY);
        return null;
      }
      const code = normCode(v.code);
      return { code: CODE_RE.test(code) ? code : "" };
    } catch {
      return null;
    }
  }
  function clearPending() {
    try {
      localStorage.removeItem(PENDING_KEY);
    } catch {
      /* nothing to clear */
    }
  }

  function linkUrl(code) {
    return code ? `${PATHS.link}?code=${encodeURIComponent(fmtCode(code))}` : PATHS.link;
  }

  function stripQuery(keys) {
    try {
      const u = new URL(location.href);
      keys.forEach((k) => u.searchParams.delete(k));
      history.replaceState(null, "", u.pathname + (u.search || "") + u.hash);
    } catch {
      /* not important */
    }
  }

  // ------------------------------------------------------------ dialogs
  // <dialog> when supported (focus trap, Escape and backdrop for free); otherwise a fixed
  // panel with a scrim, Escape to close and focus returned to the opener.
  const hasDialog = typeof HTMLDialogElement === "function" && "showModal" in HTMLDialogElement.prototype;
  let scrim = null;
  let lastOpener = null;

  function openDialog(dlg, opener) {
    lastOpener = opener || document.activeElement;
    if (hasDialog) {
      dlg.showModal();
    } else {
      dlg.classList.add("is-fallback");
      dlg.setAttribute("role", "dialog");
      dlg.setAttribute("aria-modal", "true");
      dlg.setAttribute("open", "");
      scrim = document.createElement("div");
      scrim.className = "dialog-scrim";
      scrim.addEventListener("click", () => closeDialog(dlg));
      document.body.append(scrim);
      dlg.addEventListener("keydown", fallbackKeys);
    }
    const first = dlg.querySelector("[autofocus]") || dlg.querySelector("input, button");
    if (first) first.focus();
  }

  function fallbackKeys(e) {
    if (e.key === "Escape") closeDialog(e.currentTarget);
  }

  function closeDialog(dlg) {
    if (hasDialog) {
      if (dlg.open) dlg.close();
    } else {
      dlg.removeAttribute("open");
      dlg.removeEventListener("keydown", fallbackKeys);
      if (scrim) scrim.remove();
      scrim = null;
    }
    if (lastOpener && document.contains(lastOpener)) lastOpener.focus();
    lastOpener = null;
  }

  document.addEventListener("click", (e) => {
    const t = e.target instanceof Element ? e.target.closest("[data-close-dialog]") : null;
    if (t) {
      const dlg = t.closest("dialog, .dialog");
      if (dlg) closeDialog(dlg);
    }
  });
  // Native Escape closes the dialog itself; return focus to the opener.
  if (hasDialog) {
    document.addEventListener(
      "close",
      () => {
        if (lastOpener && document.contains(lastOpener)) lastOpener.focus();
        lastOpener = null;
      },
      true
    );
  }

  // ------------------------------------------------------------ sign out (any page)
  document.querySelectorAll("[data-sign-out]").forEach((btn) => {
    btn.addEventListener("click", async () => {
      busy(btn, true, "Signing out…");
      const r = await api("POST", "/api/auth/logout", {});
      if (r.ok || r.status === 401) {
        location.assign(PATHS.login + "?signed_out=1");
        return;
      }
      busy(btn, false);
      setMsg($("#status"), r.message, "error");
    });
  });

  // ============================================================ /login/
  function initLogin() {
    const form = $("#login-form");
    const email = $("#email");
    const emailErr = $("#email-error");
    const age = $("#age");
    const ageErr = $("#age-error");
    const submit = $("#login-submit");
    const status = $("#status");
    const done = $("#login-done");
    const sentTo = $("#sent-to");
    const params = new URLSearchParams(location.search);

    if (params.get("signed_out") === "1") setMsg(status, "You're signed out.", "ok");
    if (params.get("deleted") === "1") setMsg(status, "Your account has been deleted.", "ok");
    if (params.get("expired") === "1") setMsg(status, "Please sign in to continue.");
    stripQuery(["signed_out", "deleted", "expired"]);

    const pending = readPending();
    const pendingNote = $("#pending-note");
    if (pending && pendingNote) {
      pendingNote.textContent = pending.code
        ? `After you sign in, we'll link calculator ${fmtCode(pending.code)}.`
        : "After you sign in, you'll go back to linking your calculator.";
      show(pendingNote, true);
    }

    // Already signed in? Offer the way through, quietly.
    api("GET", "/api/me").then((r) => {
      if (r.ok && r.data && typeof r.data.email === "string") {
        const box = $("#already");
        const who = $("#already-email");
        const go = $("#already-go");
        if (who) who.textContent = r.data.email;
        if (go && pending) go.href = linkUrl(pending.code);
        show(box, true);
      }
    });

    email.addEventListener("input", () => fieldError(email, emailErr, ""));
    age.addEventListener("change", () => {
      setMsg(ageErr, "");
      delete age.closest(".check").dataset.invalid;
      age.removeAttribute("aria-invalid");
    });

    form.addEventListener("submit", async (e) => {
      e.preventDefault();
      setMsg(status, "");
      const addr = email.value.trim();
      let bad = null;
      if (!isEmail(addr)) {
        fieldError(email, emailErr, "Please enter a valid email address.");
        bad = bad || email;
      }
      if (!age.checked) {
        setMsg(ageErr, "Please confirm the account holder is 18 or older.");
        age.closest(".check").dataset.invalid = "true";
        age.setAttribute("aria-invalid", "true");
        bad = bad || age;
      }
      if (bad) {
        bad.focus();
        return;
      }
      busy(submit, true, "Sending…");
      const r = await api("POST", "/api/auth/login", { email: addr, age_confirmed: true });
      busy(submit, false);
      if (!r.ok) {
        if (r.status === 400 && /e-?mail/i.test(r.message)) {
          fieldError(email, emailErr, r.message);
          email.focus();
        } else {
          setMsg(status, r.message, "error");
        }
        return;
      }
      sentTo.textContent = addr;
      show(form, false);
      show(done, true);
      setupDevOutbox(addr);
      focusEl($("#done-title"));
    });

    $("#login-again").addEventListener("click", () => {
      show(done, false);
      show(form, true);
      email.focus();
      email.select();
    });
  }

  // Local testing only: open the captured sign-in email (the API's DEV_MODE outbox).
  // Does nothing unless the page itself is served from localhost or 127.0.0.1.
  function setupDevOutbox(addr) {
    const box = $("#dev-outbox");
    if (!box) return;
    if (!isLocal) {
      box.remove();
      return;
    }
    show(box, true);
    const btn = $("#dev-outbox-open");
    const msg = $("#dev-outbox-msg");
    btn.onclick = async () => {
      setMsg(msg, "");
      busy(btn, true, "Opening…");
      let link = "";
      try {
        const res = await fetch(`/__dev/outbox?email=${encodeURIComponent(addr)}`, { cache: "no-store" });
        const j = res.ok ? await res.json() : null;
        link = j && typeof j.link === "string" ? j.link : "";
      } catch {
        link = "";
      }
      busy(btn, false);
      let u = null;
      try {
        u = link ? new URL(link, location.href) : null;
      } catch {
        u = null;
      }
      // Only follow a link to this same local origin.
      if (u && u.origin === location.origin && u.pathname.startsWith("/login/confirm")) {
        location.assign(u.href);
      } else {
        setMsg(msg, "No captured email found. Is the API running with DEV_MODE=1?", "error");
      }
    };
  }

  // ============================================================ /login/confirm/
  function initConfirm() {
    const params = new URLSearchParams(location.search);
    const token = params.get("token") || "";
    const form = $("#confirm-form");
    const btn = $("#confirm-submit");
    const status = $("#status");
    const missing = $("#confirm-missing");
    // Keep the one-time token out of the address bar and history once it's been read.
    if (token) stripQuery(["token"]);

    if (!token || token.length < 32 || token.length > 128 || !/^[A-Za-z0-9_-]+$/.test(token)) {
      show(form, false);
      show(missing, true);
      return;
    }
    show(form, true);

    // No auto-submit: email link scanners open links too, and must not use the token up.
    form.addEventListener("submit", async (e) => {
      e.preventDefault();
      setMsg(status, "");
      busy(btn, true, "Signing in…");
      const r = await api("POST", "/api/auth/confirm", { token });
      if (r.ok) {
        setMsg(status, "Signed in. Taking you to your account…", "ok");
        const pending = readPending();
        if (pending) {
          clearPending();
          location.replace(linkUrl(pending.code));
        } else {
          location.replace(PATHS.account);
        }
        return;
      }
      busy(btn, false);
      if (r.type === "invalid_token" || r.status === 400) {
        show(form, false);
        const bad = $("#confirm-invalid");
        const text = $("#confirm-invalid-text");
        if (text) text.textContent = r.message;
        show(bad, true);
        focusEl($("#confirm-invalid-title"));
      } else {
        setMsg(status, r.message, "error");
        btn.focus();
      }
    });
  }

  // ============================================================ /link/
  function initLink() {
    const params = new URLSearchParams(location.search);
    const form = $("#link-form");
    const code = $("#code");
    const codeErr = $("#code-error");
    const label = $("#label");
    const labelErr = $("#label-error");
    const btn = $("#link-submit");
    const status = $("#status");
    const loading = $("#link-loading");
    const who = $("#signed-in-as");

    const fromUrl = normCode(params.get("code"));
    const pending = readPending();
    const initial = fromUrl || (pending && pending.code) || "";
    if (initial) code.value = fmtCode(initial.slice(0, 8));
    clearPending();

    const goSignIn = () => {
      savePending(CODE_RE.test(normCode(code.value)) ? normCode(code.value) : "");
      location.replace(PATHS.login);
    };

    api("GET", "/api/me").then((r) => {
      if (!r.ok && r.status === 401) {
        goSignIn();
        return;
      }
      show(loading, false);
      show(form, true);
      if (r.ok && r.data && who) {
        who.textContent = `Signed in as ${r.data.email}.`;
        const active = Array.isArray(r.data.devices) ? r.data.devices.filter((d) => d && !d.revoked).length : 0;
        if (active >= 3) {
          setMsg(status, "This account already has 3 calculators. Remove one on your account page first.", "error");
        }
      } else if (!r.ok) {
        setMsg(status, r.message, "error");
      }
      (code.value && CODE_RE.test(normCode(code.value)) ? label : code).focus();
    });

    // Format as XXXX-XXXX while typing (only when the caret is at the end, so editing works).
    code.addEventListener("input", () => {
      fieldError(code, codeErr, "");
      const atEnd = code.selectionStart === code.value.length;
      const c = normCode(code.value).slice(0, 8);
      if (atEnd) code.value = fmtCode(c);
    });
    label.addEventListener("input", () => fieldError(label, labelErr, ""));

    form.addEventListener("submit", async (e) => {
      e.preventDefault();
      setMsg(status, "");
      const c = normCode(code.value);
      const name = label.value.trim();
      if (!CODE_RE.test(c)) {
        fieldError(
          code,
          codeErr,
          /[01OI]/.test(c) && c.length === 8
            ? "Codes never use O, 0, I or 1. Check the code on the calculator."
            : "Enter the 8-character code shown on the calculator."
        );
        code.focus();
        return;
      }
      if (name && (name.length > 40 || /[<>\u0000-\u001f\u007f]/.test(name))) {
        fieldError(label, labelErr, "Name must be 1-40 characters, without < or >.");
        label.focus();
        return;
      }
      busy(btn, true, "Linking…");
      const body = { code: fmtCode(c) };
      if (name) body.label = name;
      const r = await api("POST", "/api/devices/claim", body);
      busy(btn, false);
      if (r.ok) {
        const doneName = $("#done-name");
        if (doneName) doneName.textContent = (r.data && r.data.label) || name || "Your calculator";
        show(form, false);
        show($("#link-done"), true);
        focusEl($("#link-done-title"));
        return;
      }
      if (r.status === 401) {
        goSignIn();
        return;
      }
      if (r.type === "invalid_code" || (r.status === 400 && /code/i.test(r.message))) {
        fieldError(code, codeErr, r.message);
        code.focus();
      } else if (r.status === 400 && /name/i.test(r.message)) {
        fieldError(label, labelErr, r.message);
        label.focus();
      } else {
        setMsg(status, r.message, "error");
      }
    });
  }

  // ============================================================ /account/
  function initAccount() {
    const params = new URLSearchParams(location.search);
    const checkout = params.get("checkout");
    const wantPlan = params.get("plan");
    stripQuery(["checkout", "plan"]);

    // Monthly / Yearly choice for Subscribe. Yearly is preselected in the HTML; ?plan=month from
    // the plan page keeps the visitor's choice.
    const pick = $("#plan-pick");
    const subBtn = $("#btn-subscribe");
    const SUB_LABEL = { year: "Subscribe yearly, $100/year", month: "Subscribe monthly, $10/month" };
    const chosenInterval = () => {
      const r = pick && pick.querySelector('input[name="interval"]:checked');
      return r && r.value === "month" ? "month" : "year";
    };
    const syncSubLabel = () => {
      if (subBtn && !subBtn.hasAttribute("aria-busy")) subBtn.textContent = SUB_LABEL[chosenInterval()];
    };
    if (pick) {
      if (wantPlan === "month" || wantPlan === "year") {
        const r = pick.querySelector(`input[value="${wantPlan}"]`);
        if (r) r.checked = true;
      }
      pick.addEventListener("change", syncSubLabel);
      syncSubLabel();
    }

    const loading = $("#account-loading");
    const view = $("#account-view");
    const loadErr = $("#account-error");
    const status = $("#status");
    const checkoutNote = $("#checkout-note");
    let me = null;

    if (checkout === "done") {
      checkoutNote.dataset.tone = "ok";
      checkoutNote.textContent = "Thanks. Your subscription can take a few seconds to show here.";
      show(checkoutNote, true);
    } else if (checkout === "cancelled") {
      checkoutNote.textContent = "Checkout cancelled. You haven't been charged.";
      show(checkoutNote, true);
    }

    async function load(quiet) {
      const r = await api("GET", "/api/me");
      if (!r.ok && r.status === 401) {
        location.replace(PATHS.login + "?expired=1");
        return null;
      }
      if (!r.ok || !r.data) {
        if (!quiet) {
          show(loading, false);
          show(view, false);
          $("#account-error-text").textContent = r.message || fallbackMsg(r.status);
          show(loadErr, true);
        }
        return null;
      }
      me = r.data;
      render(me);
      show(loading, false);
      show(loadErr, false);
      show(view, true);
      return me;
    }

    $("#account-retry").addEventListener("click", async (e) => {
      busy(e.currentTarget, true, "Trying again…");
      const ok = await load(false);
      busy(e.currentTarget, false);
      if (ok) focusEl($("#account-title"));
    });

    // ---------------------------------------------------- render
    function planWords(m) {
      const p = m.plan;
      if (p === "trial") {
        return {
          text: m.trial_ends_at ? `Free month, until ${fmtDate(m.trial_ends_at)}` : "Free month",
          sub: "Subscribe any time to keep AI Solve after it ends.",
        };
      }
      if (p === "active") {
        const yearly = m.plan_interval === "year";
        const name = yearly ? "Yearly plan" : m.plan_interval === "month" ? "Monthly plan" : "Subscribed";
        if (m.cancel_at_period_end && m.period_end) {
          return { text: `${name}, ending ${fmtDate(m.period_end)}`, sub: "It won't renew. You can restart it in Manage billing." };
        }
        return {
          text: m.period_end ? `${name}, renews ${fmtDate(m.period_end)}` : name,
          sub: yearly ? "$100 a year." : m.plan_interval === "month" ? "$10 a month." : "",
        };
      }
      if (p === "past_due") {
        return {
          text: "Payment problem — update your card in Manage billing to keep AI working.",
          sub: "Your last payment didn't go through.",
          bad: true,
        };
      }
      if (p === "inactive") {
        return { text: "Ended", sub: "AI Solve is off. Subscribe to turn it back on. The calculator still works as a calculator." };
      }
      return { text: "No plan yet", sub: "Linking a new calculator starts its free month. You can also subscribe." };
    }

    function render(m) {
      $("#acct-email").textContent = m.email;
      const pw = planWords(m);
      const planEl = $("#acct-plan");
      planEl.textContent = pw.text;
      planEl.classList.toggle("plan-bad", !!pw.bad);
      $("#acct-plan-sub").textContent = pw.sub;

      const u = m.usage || {};
      const used = Number(u.solves_this_month) || 0;
      const limit = Number(u.solves_limit) || 300;
      $("#acct-usage").textContent = `${used} of ${limit} solves this month`;
      const pct = Math.max(0, Math.min(100, (used / limit) * 100));
      const meter = $("#acct-meter");
      meter.querySelector(".meter__fill").style.width = pct + "%";
      meter.dataset.full = String(used >= limit || !!u.spend_limit_reached);
      $("#acct-usage-sub").textContent =
        used >= limit || u.spend_limit_reached
          ? "This month's AI limit is reached. It resets on the 1st."
          : "The count resets on the 1st of each month.";

      // Billing buttons
      const subscribed = m.plan === "active" || m.plan === "past_due";
      show($("#btn-subscribe"), !subscribed);
      show(pick, !subscribed);
      show($("#btn-billing"), m.has_billing === true || subscribed);
      // S3-10: subscribing during the free month starts billing today; say so plainly.
      const trialNote = $("#trial-note");
      if (m.plan === "trial" && !subscribed) {
        trialNote.textContent = m.trial_ends_at
          ? `Your free month runs until ${fmtDate(m.trial_ends_at)}. If you subscribe now, billing starts today.`
          : "Your free month is running. If you subscribe now, billing starts today.";
        show(trialNote, true);
      } else {
        trialNote.textContent = "";
        show(trialNote, false);
      }

      renderDevices(Array.isArray(m.devices) ? m.devices.filter((d) => d && !d.revoked) : []);
    }

    const list = $("#device-list");
    const tpl = $("#device-template");

    function renderDevices(devs) {
      $("#device-count").textContent = `${devs.length} of 3`;
      list.textContent = "";
      show($("#devices-empty"), devs.length === 0);
      show(list, devs.length > 0);
      const linkBtn = $("#btn-link");
      if (linkBtn) {
        const full = devs.length >= 3;
        if (full) linkBtn.setAttribute("aria-disabled", "true");
        else linkBtn.removeAttribute("aria-disabled");
        $("#device-full").hidden = !full;
      }
      for (const d of devs) list.append(deviceItem(d));
    }

    function deviceItem(d) {
      const li = tpl.content.firstElementChild.cloneNode(true);
      const id = String(d.id);
      li.dataset.id = id;
      const name = li.querySelector(".device-item__name");
      name.textContent = d.label || "Calculator";
      name.id = `dev-name-${id}`;
      li.setAttribute("aria-labelledby", name.id);
      li.querySelector(".device-item__meta").textContent =
        (d.last_seen_at ? `Last used ${fmtDate(d.last_seen_at)}` : "Not used yet") +
        (d.created_at ? ` · Linked ${fmtDate(d.created_at)}` : "");
      const tech = [];
      if (d.hw_id) tech.push(`hw ${d.hw_id}`);
      if (d.fw_version) tech.push(`fw ${d.fw_version}`);
      const techEl = li.querySelector(".device-item__tech");
      techEl.textContent = tech.join(" · ");
      show(techEl, tech.length > 0);

      const renameBtn = li.querySelector("[data-rename]");
      const removeBtn = li.querySelector("[data-remove]");
      const form = li.querySelector(".device-item__rename");
      const input = form.querySelector("input");
      const err = form.querySelector(".field-error");
      const inputId = `dev-label-${id}`;
      const errId = `dev-label-err-${id}`;
      input.id = inputId;
      err.id = errId;
      input.setAttribute("aria-describedby", errId);
      form.querySelector("label").htmlFor = inputId;
      renameBtn.setAttribute("aria-label", `Rename ${d.label || "Calculator"}`);
      removeBtn.setAttribute("aria-label", `Remove ${d.label || "Calculator"}`);
      renameBtn.setAttribute("aria-expanded", "false");

      renameBtn.addEventListener("click", () => {
        const open = form.hidden;
        show(form, open);
        renameBtn.setAttribute("aria-expanded", String(open));
        if (open) {
          input.value = d.label || "";
          fieldError(input, err, "");
          input.focus();
          input.select();
        }
      });
      form.querySelector("[data-cancel]").addEventListener("click", () => {
        show(form, false);
        renameBtn.setAttribute("aria-expanded", "false");
        renameBtn.focus();
      });
      input.addEventListener("input", () => fieldError(input, err, ""));
      form.addEventListener("submit", async (e) => {
        e.preventDefault();
        const v = input.value.trim();
        if (!v || v.length > 40 || /[<>\u0000-\u001f\u007f]/.test(v)) {
          fieldError(input, err, "Name must be 1-40 characters, without < or >.");
          input.focus();
          return;
        }
        const save = form.querySelector("[type=submit]");
        busy(save, true, "Saving…");
        const r = await api("PATCH", `/api/devices/${encodeURIComponent(id)}`, { label: v });
        busy(save, false);
        if (!r.ok) {
          if (r.status === 401) return location.replace(PATHS.login + "?expired=1");
          fieldError(input, err, r.message);
          input.focus();
          return;
        }
        d.label = (r.data && r.data.label) || v;
        name.textContent = d.label;
        renameBtn.setAttribute("aria-label", `Rename ${d.label}`);
        removeBtn.setAttribute("aria-label", `Remove ${d.label}`);
        show(form, false);
        renameBtn.setAttribute("aria-expanded", "false");
        setMsg(status, `Renamed to ${d.label}.`, "ok");
        renameBtn.focus();
      });

      removeBtn.addEventListener("click", () => {
        pendingRemove = d;
        $("#remove-name").textContent = d.label || "Calculator";
        setMsg($("#remove-msg"), "");
        openDialog($("#remove-dialog"), removeBtn);
      });
      return li;
    }

    // ---------------------------------------------------- remove dialog
    let pendingRemove = null;
    $("#remove-confirm").addEventListener("click", async (e) => {
      const btn = e.currentTarget;
      const d = pendingRemove;
      if (!d) return;
      busy(btn, true, "Removing…");
      const r = await api("POST", `/api/devices/${encodeURIComponent(d.id)}/revoke`, {});
      busy(btn, false);
      if (!r.ok && r.status !== 404) {
        if (r.status === 401) return location.replace(PATHS.login + "?expired=1");
        setMsg($("#remove-msg"), r.message, "error");
        return;
      }
      pendingRemove = null;
      const dlg = $("#remove-dialog");
      lastOpener = null; // the opener is about to disappear
      closeDialog(dlg);
      await load(true);
      setMsg(status, `${d.label || "Calculator"} was removed from your account.`, "ok");
      focusEl($("#devices-title"));
    });

    // ---------------------------------------------------- billing
    async function goBilling(btn, path, label, body) {
      if (btn.disabled) return; // a double click while the first request is out
      setMsg($("#billing-msg"), "");
      busy(btn, true, label);
      const r = await api("POST", path, body || {});
      if (r.ok && r.data && typeof r.data.url === "string") {
        let u = null;
        try {
          u = new URL(r.data.url, location.href);
        } catch {
          u = null;
        }
        if (u && (u.protocol === "https:" || (isLocal && u.protocol === "http:"))) {
          location.assign(u.href);
          return;
        }
      }
      busy(btn, false);
      syncSubLabel();
      if (r.status === 401) return location.replace(PATHS.login + "?expired=1");
      setMsg($("#billing-msg"), r.ok ? fallbackMsg(500) : r.message, "error");
      btn.focus();
    }
    // Back from the checkout page with the browser's back button: the page comes from the
    // back/forward cache with the spinner still on. Reset the buttons and refresh.
    window.addEventListener("pageshow", (e) => {
      if (!e.persisted) return;
      ["#btn-subscribe", "#btn-billing"].forEach((sel) => {
        const b = $(sel);
        if (b && b.hasAttribute("aria-busy")) busy(b, false);
      });
      syncSubLabel();
      load(true);
    });
    $("#btn-subscribe").addEventListener("click", (e) =>
      goBilling(e.currentTarget, "/api/billing/checkout", "Opening secure checkout…", { interval: chosenInterval() })
    );
    $("#btn-billing").addEventListener("click", (e) =>
      goBilling(e.currentTarget, "/api/billing/portal", "Opening billing…")
    );
    $("#btn-link").addEventListener("click", (e) => {
      if (e.currentTarget.getAttribute("aria-disabled") === "true") {
        e.preventDefault();
        setMsg(status, "This account already has 3 calculators. Remove one first.", "error");
      }
    });

    // ---------------------------------------------------- delete account
    const delInput = $("#delete-email");
    const delBtn = $("#delete-confirm");
    const delErr = $("#delete-email-error");
    const matches = () => !!me && delInput.value.trim().toLowerCase() === String(me.email).toLowerCase();
    $("#btn-delete").addEventListener("click", (e) => {
      delInput.value = "";
      fieldError(delInput, delErr, "");
      delBtn.disabled = true;
      openDialog($("#delete-dialog"), e.currentTarget);
    });
    delInput.addEventListener("input", () => {
      fieldError(delInput, delErr, "");
      delBtn.disabled = !matches();
    });
    $("#delete-form").addEventListener("submit", async (e) => {
      e.preventDefault();
      if (!matches()) {
        fieldError(delInput, delErr, "Type the email address of this account exactly.");
        delInput.focus();
        return;
      }
      busy(delBtn, true, "Deleting…");
      const r = await api("DELETE", "/api/me", { confirm_email: delInput.value.trim() });
      if (r.ok) {
        location.replace(PATHS.login + "?deleted=1");
        return;
      }
      busy(delBtn, false);
      delBtn.disabled = !matches();
      if (r.status === 401) return location.replace(PATHS.login + "?expired=1");
      fieldError(delInput, delErr, r.message);
      delInput.focus();
    });

    // ---------------------------------------------------- first load (+ checkout polling)
    load(false).then(async (m) => {
      if (!m || checkout !== "done") return;
      // The payment provider tells the API by webhook a few seconds after the redirect: re-check a few times.
      for (let i = 0; i < 6 && me && me.plan !== "active" && me.plan !== "past_due"; i++) {
        await new Promise((res) => setTimeout(res, 2500));
        await load(true);
      }
      if (me && (me.plan === "active" || me.plan === "past_due")) {
        checkoutNote.textContent = "Thanks. Your subscription is active.";
      } else {
        checkoutNote.textContent =
          "Thanks. Your payment is still being confirmed. Refresh this page in a minute to see it.";
      }
    });
  }

  // ------------------------------------------------------------ start
  const inits = { login: initLogin, confirm: initConfirm, link: initLink, account: initAccount };
  try {
    if (inits[page]) inits[page]();
  } catch (e) {
    const s = $("#status");
    setMsg(s, "Something went wrong on this page. Please reload it.", "error");
    if (window.console) console.error(e);
  }
})();
