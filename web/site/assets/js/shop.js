// Proofline shop pages. Small, deferred, no dependencies; loaded after site.js (window.SiteCart).
// Without it the pages still read correctly: the gallery shows every image, the store shows
// "Join the waitlist", and the cart explains that it needs JavaScript.
(() => {
  "use strict";

  const $ = (sel, root = document) => root.querySelector(sel);
  const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel));
  const Cart = window.SiteCart;
  if (!Cart) return; // site.js didn't run: keep the no-JS view
  const CONTACT = "hello@proofline.example"; // TODO(contact): same as site.js
  const PRICE = 225; // the calculator, US dollars
  const money = (n) =>
    new Intl.NumberFormat("en-US", { style: "currency", currency: "USD", maximumFractionDigits: 0 }).format(n);
  const say = (el, text, tone) => {
    if (!el) return;
    el.textContent = text;
    if (tone) el.dataset.tone = tone;
    else delete el.dataset.tone;
  };

  // Re-announce a status message even when the text is the same as last time.
  const announce = (el, build, tone) => {
    if (!el) return;
    el.textContent = "";
    if (tone) el.dataset.tone = tone;
    else delete el.dataset.tone;
    setTimeout(() => build(el), 60);
  };

  // Controls that only work with JS start hidden in the HTML.
  $$("[data-js-only]").forEach((el) => (el.hidden = false));

  // ------------------------------------------------------------ cart storage
  // site.js keeps the cart in localStorage. If the browser blocks storage (some private modes,
  // strict settings), keep the count in memory for this page, and carry it to the cart page in
  // the address (?qty=) so Preorder still works.
  const storageOK = (() => {
    try {
      const k = "__cart_test";
      localStorage.setItem(k, "1");
      localStorage.removeItem(k);
      return true;
    } catch {
      return false;
    }
  })();
  let memQty = 0;
  const count = () => (storageOK ? Cart.count() : memQty);
  const setCount = (n) => {
    const q = Cart.set(n);
    if (!storageOK) memQty = q;
    return q;
  };
  const clampQty = (v) => Math.max(0, Math.min(Cart.max, Math.floor(Number(v) || 0)));
  const plural = (n) => `${n} calculator${n === 1 ? "" : "s"}`;

  // ------------------------------------------------------------ store status
  // GET /api/store/status -> { open, max_quantity }. Anything else (no API, error, slow) = closed.
  let store = { open: false, max: Cart.max };
  const applyStore = (s) => {
    store = s;
    $$("[data-store-open]").forEach((el) => (el.hidden = !s.open));
    $$("[data-store-closed]").forEach((el) => (el.hidden = s.open));
    document.dispatchEvent(new CustomEvent("store:status", { detail: s }));
  };
  const loadStore = async () => {
    const ctl = new AbortController();
    const timer = setTimeout(() => ctl.abort(), 6000);
    try {
      const res = await fetch("/api/store/status", {
        headers: { Accept: "application/json" },
        credentials: "same-origin",
        signal: ctl.signal,
      });
      if (!res.ok) throw new Error("HTTP " + res.status);
      const d = await res.json();
      const max = Math.max(1, Math.min(Cart.max, Math.floor(Number(d && d.max_quantity) || Cart.max)));
      return { open: !!d && d.open === true, max };
    } catch {
      return { open: false, max: Cart.max };
    } finally {
      clearTimeout(timer);
    }
  };
  if ($("[data-store-open], [data-store-closed]")) loadStore().then(applyStore);

  // ------------------------------------------------------------ quantity steppers
  // <div class="qty" data-qty> <button data-qty-step="-1"> <input type=number> <button data-qty-step="1"> </div>
  const steppers = [];
  const setupQty = (root, onChange) => {
    const input = $("input", root);
    const dec = $('[data-qty-step="-1"]', root);
    const inc = $('[data-qty-step="1"]', root);
    const limit = () => Math.min(Cart.max, store.max);
    const get = () => Math.max(1, Math.min(limit(), Math.floor(Number(input.value) || 1)));
    const sync = () => {
      const v = get();
      input.value = String(v);
      input.max = String(limit());
      dec.disabled = v <= 1;
      inc.disabled = v >= limit();
      return v;
    };
    const set = (v, fire = true) => {
      input.value = String(v);
      const q = sync();
      if (fire && onChange) onChange(q);
    };
    dec.addEventListener("click", () => set(get() - 1));
    inc.addEventListener("click", () => set(get() + 1));
    input.addEventListener("change", () => set(input.value));
    sync();
    const api = { get, set, sync };
    steppers.push(api);
    return api;
  };
  document.addEventListener("store:status", () => steppers.forEach((s) => s.sync()));

  // ------------------------------------------------------------ gallery
  // Without JS: every figure is shown. With JS: one at a time, thumbnails swap it.
  $$("[data-gallery]").forEach((gal) => {
    const slides = $$(".gallery__slide", gal);
    const thumbs = $$(".gallery__thumb", gal);
    const strip = $(".gallery__thumbs", gal);
    if (slides.length < 2 || thumbs.length !== slides.length) return;
    const select = (i, focus) => {
      slides.forEach((s, j) => {
        if (j === i) s.dataset.active = "";
        else delete s.dataset.active;
        // hidden slides leave the accessibility tree too
        s.setAttribute("aria-hidden", String(j !== i));
      });
      thumbs.forEach((t, j) => {
        t.setAttribute("aria-pressed", String(j === i));
        t.tabIndex = j === i ? 0 : -1;
      });
      if (focus) thumbs[i].focus();
    };
    thumbs.forEach((t, i) => {
      t.addEventListener("click", () => select(i, false));
      t.addEventListener("keydown", (ev) => {
        const n = thumbs.length;
        const to = { ArrowRight: i + 1, ArrowDown: i + 1, ArrowLeft: i - 1, ArrowUp: i - 1, Home: 0, End: n - 1 }[ev.key];
        if (to === undefined) return;
        ev.preventDefault();
        select((to + n) % n, true);
      });
    });
    gal.dataset.ready = "";
    strip.hidden = false;
    select(0, false);
  });

  // ------------------------------------------------------------ product page: add to cart / preorder
  const buy = $("[data-buy]");
  if (buy) {
    const qty = setupQty($("[data-qty]", buy));
    const msg = $("[data-cart-msg]", buy);
    const addBtn = $("[data-add-to-cart]", buy);
    const preBtn = $("[data-preorder]", buy);

    addBtn.addEventListener("click", () => {
      const before = count();
      const want = before + qty.get();
      const now = setCount(want);
      const added = now - before;
      const text = !storageOK
        ? `Your browser is blocking site storage, so the cart can’t be saved.${store.open ? " Preorder still works." : ""} `
        : added <= 0
          ? `Your cart already has ${plural(now)}, the most for one order. `
          : want > now
            ? `Added ${added}. Your cart now has ${plural(now)}, the most for one order. `
            : `Added. Your cart has ${plural(now)}. `;
      announce(
        msg,
        (el) => {
          el.append(text);
          if (!storageOK) return;
          const a = document.createElement("a");
          a.href = "/cart/";
          a.textContent = "View cart";
          el.append(a);
        },
        storageOK ? "ok" : "error"
      );
    });

    // Preorder = buy this many now: the cart holds exactly the chosen quantity, then the cart page.
    let leaving = false;
    preBtn.addEventListener("click", () => {
      if (leaving) return;
      leaving = true;
      const n = setCount(qty.get());
      location.assign(storageOK ? "/cart/" : `/cart/?qty=${n}`);
    });
    window.addEventListener("pageshow", (e) => {
      if (e.persisted) leaving = false;
    });
  }

  // ------------------------------------------------------------ cart page
  const cartPage = $("[data-cart-page]");
  if (cartPage) {
    const full = $("[data-cart-full]", cartPage);
    const empty = $("[data-cart-empty]", cartPage);
    const lineTotal = $("[data-line-total]", cartPage);
    const subtotals = $$("[data-subtotal]", cartPage);
    const countEl = $("[data-item-count]", cartPage);
    const lineMsg = $("[data-line-msg]", cartPage);
    const checkoutBtn = $("[data-checkout]", cartPage);
    const checkoutMsg = $("[data-checkout-msg]", cartPage);
    const checkoutLabel = checkoutBtn.innerHTML;
    let busy = false;

    const undoMsg = $("[data-undo-msg]", cartPage);
    const qtyInput = $("[data-qty] input", cartPage);
    let typed = "";
    qtyInput.addEventListener("input", () => (typed = qtyInput.value));
    const qty = setupQty($("[data-qty]", cartPage), (q) => {
      setCount(q);
      render();
      const over = typed !== "" && Number(typed) > Cart.max;
      typed = "";
      announce(lineMsg, (el) =>
        el.append(over ? `You can order up to ${Cart.max}. Quantity set to ${q}.` : `Quantity updated to ${q}.`)
      );
    });

    // Storage blocked: take the quantity from the address (set by Preorder) and say the cart won't keep.
    const params = new URLSearchParams(location.search);
    if (!storageOK) {
      memQty = clampQty(params.get("qty"));
      const note = $("[data-cancelled]", cartPage);
      const noteText = $("[data-cancelled-text]", cartPage);
      if (note && noteText) {
        note.hidden = false;
        noteText.textContent = "Your browser is blocking site storage, so this cart won’t be kept if you leave the page.";
      }
    }

    const render = () => {
      const n = count();
      full.hidden = n === 0;
      empty.hidden = n !== 0;
      if (!n) return;
      qty.set(n, false);
      lineTotal.textContent = money(PRICE * n);
      subtotals.forEach((el) => (el.textContent = money(PRICE * n)));
      countEl.textContent = `${n} calculator${n === 1 ? "" : "s"}`;
    };

    $("[data-remove]", cartPage).addEventListener("click", () => {
      const was = count();
      setCount(0);
      render();
      const h = $("h2", empty);
      h.tabIndex = -1;
      h.focus();
      // Offer an undo, like most stores do.
      announce(undoMsg, (el) => {
        el.append(`Removed ${plural(was)} from your cart. `);
        const b = document.createElement("button");
        b.type = "button";
        b.className = "link-btn";
        b.textContent = "Undo";
        b.addEventListener("click", () => {
          setCount(was);
          say(undoMsg, "");
          render();
          qtyInput.focus();
          announce(lineMsg, (m) => m.append(`Back in your cart: ${plural(was)}.`));
        });
        el.append(b);
      });
    });

    // Another tab changed the cart.
    window.addEventListener("storage", (e) => e.key === "cart.v1" && render());

    // Back from the payment page with "cancel". The text goes in after load so screen readers hear it.
    if (params.get("checkout") === "cancelled" || params.has("qty")) {
      const note = $("[data-cancelled]", cartPage);
      const noteText = $("[data-cancelled-text]", cartPage);
      if (params.get("checkout") === "cancelled" && note && noteText) {
        note.hidden = false;
        setTimeout(() => {
          noteText.textContent = "Checkout was cancelled. You haven’t been charged, and your cart is saved.";
        }, 100);
      }
      params.delete("checkout");
      params.delete("qty");
      const q = params.toString();
      history.replaceState(null, "", location.pathname + (q ? "?" + q : "") + location.hash);
    }

    const resetButton = () => {
      busy = false;
      checkoutBtn.removeAttribute("aria-busy");
      checkoutBtn.innerHTML = checkoutLabel;
    };

    checkoutBtn.addEventListener("click", async () => {
      if (busy) return;
      const quantity = count();
      if (!quantity) return render();
      busy = true;
      checkoutBtn.setAttribute("aria-busy", "true");
      checkoutBtn.innerHTML = '<span class="spinner" aria-hidden="true"></span>Opening secure checkout…';
      say(checkoutMsg, "");
      try {
        const res = await fetch("/api/store/checkout", {
          method: "POST",
          credentials: "same-origin",
          headers: { "Content-Type": "application/json", Accept: "application/json" },
          body: JSON.stringify({ quantity }),
        });
        let data = null;
        try {
          data = await res.json();
        } catch {
          /* not JSON (e.g. no API on this host) */
        }
        if (res.ok && data && typeof data.url === "string") {
          const url = new URL(data.url, location.href);
          // Local testing only: the fake checkout page runs on plain http at 127.0.0.1.
          const localPage = location.hostname === "127.0.0.1" || location.hostname === "localhost";
          const localFake = localPage && url.protocol === "http:" && url.hostname === "127.0.0.1";
          if (url.protocol === "https:" || url.origin === location.origin || localFake) {
            location.assign(url.href);
            return; // keep the spinner while the browser leaves
          }
        }
        const apiMsg = data && data.error && typeof data.error.message === "string" ? data.error.message : "";
        const fallback =
          res.status === 429
            ? "Too many tries for now. Wait a few minutes and try again."
            : `Checkout isn’t available right now. Try again in a minute, or email ${CONTACT}.`;
        say(checkoutMsg, apiMsg || fallback, "error");
        // Not on sale (503 sold_out): show the waitlist instead of a button that can't work.
        if (data && data.error && data.error.type === "sold_out") applyStore({ open: false, max: store.max });
      } catch {
        say(checkoutMsg, "We couldn’t reach the store. Check your connection and try again.", "error");
      }
      resetButton();
    });

    // Coming back with the browser's back button from the payment page.
    window.addEventListener("pageshow", (e) => {
      if (e.persisted) {
        resetButton();
        render();
      }
    });

    render();
  }

  // ------------------------------------------------------------ plan picker (/shop/ai-tutor-plan/)
  // The choice travels to the account page's Subscribe button as ?plan=year|month.
  $$("[data-plan-pick]").forEach((pick) => {
    const sync = () => {
      const r = pick.querySelector('input[name="interval"]:checked');
      const v = r && r.value === "month" ? "month" : "year";
      $$("[data-plan-link]").forEach((a) => (a.href = `/account/?plan=${v}`));
    };
    pick.addEventListener("change", sync);
    sync();
  });

  // ------------------------------------------------------------ order complete
  if ($("[data-clear-cart]")) Cart.set(0);
})();
