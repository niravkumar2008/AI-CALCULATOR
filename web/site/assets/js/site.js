// Proofline launch site. Small, deferred, no dependencies. The page works without it:
// forms post normally, the video has a <noscript> fallback, the FAQ is <details>,
// and the calculator demo has a link to its own page.
(() => {
  "use strict";

  const $ = (sel, root = document) => root.querySelector(sel);
  const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel));
  const reduceMotion = matchMedia("(prefers-reduced-motion: reduce)");
  const saveData = !!(navigator.connection && navigator.connection.saveData);
  const EASE_OUT = "cubic-bezier(0.23, 1, 0.32, 1)";
  const CONTACT = "hello@proofline.example"; // TODO(contact): real address once the domain is bought

  // ------------------------------------------------------------ header menus
  // <details> menus work without JS; this adds: one open at a time, close on outside click,
  // close on Escape (focus back on the menu button), close the mobile sheet after a link.
  const menus = $$("details[data-menu], details[data-mobile-menu]");
  menus.forEach((d) => {
    d.addEventListener("toggle", () => {
      if (d.open) menus.forEach((o) => o !== d && (o.open = false));
    });
    d.addEventListener("click", (ev) => {
      if (ev.target.closest("a")) d.open = false;
    });
  });
  document.addEventListener("click", (ev) => {
    for (const d of menus) if (d.open && !d.contains(ev.target)) d.open = false;
  });
  document.addEventListener("keydown", (ev) => {
    if (ev.key !== "Escape") return;
    for (const d of menus) {
      if (d.open) {
        d.open = false;
        $("summary", d)?.focus();
      }
    }
  });

  // ------------------------------------------------------------ cart (shared by the shop pages)
  // Kept in this browser only. The real order happens on the secure checkout page.
  // Shape: { items: { calculator: <1..3> } }. Missing or blocked storage = empty cart.
  const CART_KEY = "cart.v1";
  const SiteCart = {
    max: 3,
    get() {
      try {
        const c = JSON.parse(localStorage.getItem(CART_KEY) || "null");
        const q = Math.floor(Number(c?.items?.calculator) || 0);
        return { items: { calculator: Math.max(0, Math.min(this.max, q)) } };
      } catch {
        return { items: { calculator: 0 } };
      }
    },
    set(qty) {
      const q = Math.max(0, Math.min(this.max, Math.floor(Number(qty) || 0)));
      try {
        if (q) localStorage.setItem(CART_KEY, JSON.stringify({ items: { calculator: q } }));
        else localStorage.removeItem(CART_KEY);
      } catch {
        /* storage blocked: the cart just won't persist */
      }
      this.render();
      document.dispatchEvent(new CustomEvent("cart:change", { detail: this.get() }));
      return q;
    },
    count() {
      return this.get().items.calculator;
    },
    render() {
      const n = this.count();
      $$("[data-cart-count]").forEach((b) => {
        b.textContent = String(n);
        b.hidden = n === 0;
      });
      $$("[data-cart-link]").forEach((a) => a.setAttribute("aria-label", n ? `Cart, ${n} item${n === 1 ? "" : "s"}` : "Cart"));
    },
  };
  window.SiteCart = SiteCart;
  SiteCart.render();
  window.addEventListener("storage", (e) => e.key === CART_KEY && SiteCart.render());

  // ------------------------------------------------------------ images that fail to load
  // Keep the reserved space, but show a quiet frame instead of a broken icon and alt-text spill.
  const markMissing = (img) => {
    const pic = img.closest("picture");
    if (pic) pic.classList.add("img-missing");
    else img.style.visibility = "hidden"; // decorative cover and video poster: the frame behind is enough
  };
  $$("img").forEach((img) => {
    if (img.complete && img.naturalWidth === 0 && img.currentSrc) markMissing(img);
    else img.addEventListener("error", () => markMissing(img), { once: true });
  });

  // ------------------------------------------------------------ header border on scroll
  const header = $(".site-header");
  if (header) {
    const sentinel = document.createElement("div");
    sentinel.style.cssText = "position:absolute;top:0;height:1px;width:1px";
    document.body.prepend(sentinel);
    new IntersectionObserver(([e]) => {
      header.dataset.scrolled = String(!e.isIntersecting);
    }).observe(sentinel);
  }

  // ------------------------------------------------------------ scroll reveals
  // Opt-in: hidden only once JS is running and motion is allowed.
  const revealables = $$("[data-reveal]");
  if (!reduceMotion.matches && "IntersectionObserver" in window && revealables.length) {
    document.documentElement.classList.add("js-motion");
    const io = new IntersectionObserver(
      (entries) => {
        // Stagger by position within this batch, not in the page: a row that scrolls in
        // on its own shouldn't wait behind rows above it. Capped so the tail never drags.
        let n = 0;
        for (const e of entries) {
          if (!e.isIntersecting) continue;
          e.target.style.setProperty("--i", String(Math.min(n++, 6)));
          e.target.classList.add("is-in");
          io.unobserve(e.target);
        }
      },
      { rootMargin: "0px 0px -8% 0px", threshold: 0.12 }
    );
    revealables.forEach((el) => io.observe(el));
    const showAll = () => revealables.forEach((el) => el.classList.add("is-in"));
    // If the user switches to reduced motion mid-visit, show everything.
    reduceMotion.addEventListener?.("change", (m) => m.matches && showAll());
    window.addEventListener("beforeprint", showAll);
    // Failsafe: a fast jump (anchor link, find-in-page, restored scroll) must never leave
    // content hidden, even if the observer hasn't caught up yet.
    let queued = false;
    const sweep = () => {
      queued = false;
      const limit = window.innerHeight;
      for (const el of revealables) {
        if (!el.classList.contains("is-in") && el.getBoundingClientRect().top < limit) el.classList.add("is-in");
      }
    };
    const onScroll = () => {
      if (!queued) {
        queued = true;
        setTimeout(sweep, 400);
      }
    };
    window.addEventListener("scroll", onScroll, { passive: true });
    window.addEventListener("hashchange", sweep);
    setTimeout(sweep, 1200);
  }

  // ------------------------------------------------------------ "Join the waitlist" links
  // Go to the nearest form and put the cursor in it, so it's one tap to type.
  $$("[data-focus-email]").forEach((link) => {
    link.addEventListener("click", (ev) => {
      const forms = $$("form.waitlist").filter((f) => f.dataset.state !== "done");
      if (!forms.length) return;
      ev.preventDefault();
      const mid = window.innerHeight / 2;
      const nearest = forms
        .map((f) => ({ f, d: Math.abs(f.getBoundingClientRect().top - mid) }))
        .sort((a, b) => a.d - b.d)[0].f;
      const input = $("input[type=email]", nearest);
      nearest.scrollIntoView({ behavior: reduceMotion.matches ? "auto" : "smooth", block: "center" });
      input.focus({ preventScroll: true });
    });
  });

  // ------------------------------------------------------------ waitlist tracking
  // Which link brought this visitor: ?utm_source=&utm_medium=&utm_campaign= (or the short
  // ?src=tt from the marketing guide) and ?ref= (a referral code). First touch wins for the
  // whole visit (sessionStorage), so clicking around the site doesn't lose it. Values must be
  // short and plain, the same rule as the API ([a-z0-9_-]{1,40}); anything else is ignored.
  const TRACK_KEYS = ["utm_source", "utm_medium", "utm_campaign", "ref"];
  const TRACK_STORE = "waitlist.track.v1";
  const TAG_RE = /^[a-z0-9_-]{1,40}$/;
  const cleanTag = (v) => {
    const s = String(v || "").trim().toLowerCase();
    return TAG_RE.test(s) ? s : "";
  };
  const tracking = (() => {
    const fromUrl = {};
    try {
      const q = new URLSearchParams(location.search);
      for (const k of TRACK_KEYS) {
        const v = cleanTag(q.get(k));
        if (v) fromUrl[k] = v;
      }
      if (!fromUrl.utm_source) {
        const src = cleanTag(q.get("src"));
        if (src) fromUrl.utm_source = src;
      }
    } catch {
      /* no URLSearchParams: no tracking */
    }
    let stored = null;
    try {
      stored = JSON.parse(sessionStorage.getItem(TRACK_STORE) || "null");
    } catch {
      stored = null;
    }
    if (stored && typeof stored === "object") {
      const kept = {};
      for (const k of TRACK_KEYS) if (cleanTag(stored[k])) kept[k] = cleanTag(stored[k]);
      if (Object.keys(kept).length) return kept;
    }
    if (Object.keys(fromUrl).length) {
      try {
        sessionStorage.setItem(TRACK_STORE, JSON.stringify(fromUrl));
      } catch {
        /* storage blocked: still used on this page */
      }
    }
    return fromUrl;
  })();

  // ------------------------------------------------------------ waitlist forms
  const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]{2,}$/;
  const AGE_MSG = "Please confirm you’re 13 or older to join.";

  $$("form.waitlist").forEach((form) => {
    const input = $("input[type=email]", form);
    const age = $("input[name=age_ok]", form);
    const button = $("button[type=submit]", form);
    const msg = $(".waitlist__msg", form);
    const label = button.textContent;

    // Without JS the browser checks `required`; with JS this script does, with clearer messages.
    form.noValidate = true;
    // The hidden fields go with the no-JS-style post too (if fetch is unavailable).
    $$("input[data-track]", form).forEach((h) => (h.value = tracking[h.dataset.track] || ""));

    if (age) {
      age.addEventListener("change", () => {
        if (age.checked && age.getAttribute("aria-invalid") === "true") {
          age.removeAttribute("aria-invalid");
          say("");
        }
      });
    }

    const say = (text, tone) => {
      msg.textContent = text;
      if (tone) msg.dataset.tone = tone;
      else delete msg.dataset.tone;
    };

    input.addEventListener("input", () => {
      if (input.getAttribute("aria-invalid") === "true" && EMAIL_RE.test(input.value.trim())) {
        input.removeAttribute("aria-invalid");
        say("");
      }
    });

    form.addEventListener("submit", async (ev) => {
      ev.preventDefault();
      if (form.dataset.state === "sending") return;

      const email = input.value.trim();
      if (!EMAIL_RE.test(email)) {
        input.setAttribute("aria-invalid", "true");
        say(email ? "That email doesn’t look right. Check it and try again." : "Enter your email to join the waitlist.", "error");
        input.focus();
        return;
      }
      input.removeAttribute("aria-invalid");

      if (age && !age.checked) {
        age.setAttribute("aria-invalid", "true");
        say(AGE_MSG, "error");
        age.focus();
        return;
      }
      if (age) age.removeAttribute("aria-invalid");

      // Honeypot filled in: act as if it worked, send nothing.
      const trap = $("input[name=company]", form);
      if (trap && trap.value) return done();

      form.dataset.state = "sending";
      button.setAttribute("aria-busy", "true");
      button.innerHTML = '<span class="spinner" aria-hidden="true"></span>Joining…';
      say("");

      let serverMsg = "";
      try {
        // POST /api/waitlist (web/api/src/waitlist.ts): 201 for a new or repeated address (same
        // answer, so nobody can check who signed up), 400 with a message for bad input, 429 when busy.
        const res = await fetch("/api/waitlist", {
          method: "POST",
          headers: { "Content-Type": "application/json", Accept: "application/json" },
          body: JSON.stringify({ email, age_ok: true, source: form.dataset.source || "site", ...tracking }),
        });
        if (res.ok) return done(false);
        // The API's own message is written for people (e.g. the 13+ rule, "busy today").
        if (res.status === 400 || res.status === 429) {
          try {
            serverMsg = String((await res.json())?.error?.message || "");
          } catch {
            serverMsg = "";
          }
        }
        throw new Error("HTTP " + res.status);
      } catch {
        form.dataset.state = "";
        button.removeAttribute("aria-busy");
        button.textContent = label;
        say(serverMsg || `We couldn’t add you just now. Try again in a minute, or email ${CONTACT}.`, "error");
      }
    });

    // Joined once: every waitlist form on the page shows it, not just the one used.
    function done(already) {
      $$("form.waitlist").forEach((f) => f !== form && f.dispatchEvent(new CustomEvent("waitlist:done", { detail: already })));
      showDone(already, true);
    }
    form.addEventListener("waitlist:done", (e) => form.dataset.state !== "done" && showDone(e.detail, false));

    function showDone(already, focus) {
      form.dataset.state = "done";
      const box = document.createElement("div");
      box.className = "waitlist__done";
      box.setAttribute("role", "status");
      box.innerHTML =
        '<svg width="22" height="22" aria-hidden="true"><use href="#i-check"/></svg><div><strong></strong><br><span></span></div>';
      $("strong", box).textContent = already ? "You’re already on the list." : "You’re on the list.";
      $("span", box).textContent = "We’ll email you when preorders open. Nothing else.";
      form.prepend(box);
      if (!reduceMotion.matches) {
        box.animate(
          [
            { opacity: 0, transform: "translateY(6px)" },
            { opacity: 1, transform: "none" },
          ],
          { duration: 320, easing: EASE_OUT }
        );
      }
      if (focus) {
        box.tabIndex = -1;
        box.focus({ preventScroll: true });
      } else {
        box.removeAttribute("role"); // only the form that was used announces it
      }
    }
  });

  // ------------------------------------------------------------ AI Solve video
  // Loaded only near the screen. Autoplays muted and looping unless the user asks for
  // reduced motion or data saving; then it waits for the play button.
  const frame = $("[data-video]");
  if (frame) {
    const btn = $("[data-video-toggle]", frame);
    const btnLabel = $("[data-video-label]", frame);
    const steps = $$("[data-steps] .step");
    // Scene starts in the video (seconds): Point 0, Press = 2.8, Learn 4.9.
    const STEP_AT = [0, 2.8, 4.9];
    let video = null;
    let userPaused = false;
    let inView = false;
    const allowAutoplay = () => !reduceMotion.matches && !saveData;

    const setPlaying = (playing) => {
      frame.dataset.playing = String(playing);
      btn.setAttribute("aria-label", playing ? "Pause the AI Solve video" : "Play the AI Solve video");
      btnLabel.textContent = playing ? "Pause" : "Play";
    };

    const markStep = (t) => {
      let idx = 0;
      STEP_AT.forEach((s, i) => {
        if (t >= s) idx = i;
      });
      steps.forEach((el, i) => {
        el.dataset.active = String(i === idx);
        el.dataset.done = String(i < idx);
      });
    };

    const syncSteps = () => {
      if (!video || video.paused) return;
      markStep(video.currentTime);
      if ("requestVideoFrameCallback" in video) video.requestVideoFrameCallback(syncSteps);
      else requestAnimationFrame(syncSteps);
    };

    const load = () => {
      if (video) return;
      video = document.createElement("video");
      video.muted = true;
      video.loop = true;
      video.playsInline = true;
      video.setAttribute("muted", "");
      video.setAttribute("playsinline", "");
      video.preload = "metadata";
      video.poster = $("img", frame).currentSrc || $("img", frame).src;
      video.setAttribute("aria-label", "AI Solve demo: snap a problem, press equals, see every step.");
      video.src = "/assets/video/ai-solve.mp4";
      video.width = 1600;
      video.height = 900;
      frame.insertBefore(video, btn);
      btn.hidden = false;
      video.addEventListener("playing", () => {
        frame.dataset.ready = "true";
        setPlaying(true);
        syncSteps();
      });
      video.addEventListener("pause", () => setPlaying(false));
      video.addEventListener("error", () => {
        // Keep the poster; hide the control that can't do anything.
        btn.hidden = true;
        video.remove();
      });
      if (allowAutoplay() && inView) play();
    };

    const play = () => {
      if (!video) return;
      const p = video.play();
      if (p && p.catch) p.catch(() => setPlaying(false));
    };

    btn.addEventListener("click", () => {
      if (!video) return;
      if (video.paused) {
        userPaused = false;
        play();
      } else {
        userPaused = true;
        video.pause();
      }
    });

    // Near the screen: load. On screen: play. Off screen: pause (saves battery).
    new IntersectionObserver(([e]) => e.isIntersecting && load(), { rootMargin: "400px 0px" }).observe(frame);
    new IntersectionObserver(
      ([e]) => {
        inView = e.isIntersecting;
        if (!video) return;
        if (inView && allowAutoplay() && !userPaused) play();
        else if (!inView && !video.paused) video.pause();
      },
      { threshold: 0.35 }
    ).observe(frame);

    document.addEventListener("visibilitychange", () => {
      if (!video) return;
      if (document.hidden) video.pause();
      else if (inView && allowAutoplay() && !userPaused) play();
    });

    reduceMotion.addEventListener?.("change", (m) => {
      if (m.matches && video && !video.paused) {
        userPaused = true;
        video.pause();
      }
    });

    markStep(0);
  }

  // ------------------------------------------------------------ calculator simulator
  const sim = $("[data-sim]");
  if (sim) {
    const loadBtn = $("[data-sim-load]", sim);
    const simMsg = $("[data-sim-msg]", sim);
    const tpl = $("[data-sim-template]", sim);
    let timer = 0;

    const fail = () => {
      sim.dataset.state = "error";
      loadBtn.textContent = "Try again";
      simMsg.innerHTML = 'The calculator didn’t load. Try again, or <a href="/try/">open it on its own page</a>.';
      const old = $("iframe", sim);
      if (old) old.remove();
    };

    const loadSim = () => {
      if (sim.dataset.state === "loading" || sim.dataset.state === "ready") return;
      sim.dataset.state = "loading";
      loadBtn.innerHTML = '<span class="spinner" aria-hidden="true"></span>Loading…';
      simMsg.textContent = "Loading the calculator engine…";
      const iframe = tpl.content.firstElementChild.cloneNode(true);
      iframe.loading = "eager";
      iframe.addEventListener("load", () => {
        clearTimeout(timer);
        try {
          // Same origin: size the frame to its content so there's no inner scrollbar.
          // A 404 page still fires "load", so check the calculator itself is there.
          const doc = iframe.contentDocument;
          if (!doc || !doc.querySelector(".calc")) return fail();
          const fit = () => {
            const h = Math.ceil(doc.documentElement.scrollHeight);
            if (h > 200) {
              iframe.style.height = h + "px";
              sim.style.minHeight = h + "px";
            }
          };
          fit();
          if ("ResizeObserver" in window) new ResizeObserver(fit).observe(doc.body);
        } catch {
          /* cross-origin in some previews: keep the reserved height */
        }
        // The cover fades out and becomes hidden: if the keyboard was on its button,
        // move focus into the calculator so it isn't left on an invisible control.
        const hadFocus = sim.contains(document.activeElement) && document.activeElement !== iframe;
        sim.dataset.state = "ready";
        simMsg.textContent = "";
        if (hadFocus) iframe.focus();
      });
      iframe.addEventListener("error", fail);
      timer = setTimeout(fail, 20000);
      sim.append(iframe);
    };

    loadBtn.addEventListener("click", () => {
      if (sim.dataset.state === "error") sim.dataset.state = "idle";
      loadSim();
    });

    if (!saveData) {
      new IntersectionObserver(
        ([e], obs) => {
          if (e.isIntersecting) {
            obs.disconnect();
            loadSim();
          }
        },
        { rootMargin: "200px 0px" }
      ).observe(sim);
    } else {
      simMsg.textContent = "About 500 KB. Data saver is on, so it waits for you.";
    }
  }

  // ------------------------------------------------------------ exam mode demo
  // Plays SHIFT, MODE, 8, = on the keycaps, then the status bar turns black,
  // the same way the firmware draws it (core/device.cpp, drawStatus).
  const demo = $("[data-exam-demo]");
  if (demo) {
    const toggle = $("[data-exam-toggle]", demo);
    const caption = $("[data-exam-caption]", demo);
    const keys = $$("[data-key]", demo);
    const views = { normal: $('[data-view="normal"]', demo), exam: $('[data-view="exam"]', demo) };
    const lcd = $("[data-lcd-label]", demo);
    const clock = $("[data-exam-clock]", demo);
    let busy = false;
    let tick = 0;

    const describe = (exam) => {
      lcd.setAttribute(
        "aria-label",
        exam
          ? "Calculator screen in exam mode. The status bar is black with white text and shows a padlock, EXAM and a timer counting up. The screen says: Exam mode on. AI, camera and Wi-Fi are off. Unlock: USB cable, or wait 12 hours."
          : "Calculator screen showing an AI Solve answer, 16.75, with three steps. The status bar shows Wi-Fi and AI."
      );
    };
    describe(false);

    const wait = (ms) => new Promise((r) => setTimeout(r, ms));

    const swapView = (exam) => {
      const show = exam ? views.exam : views.normal;
      const hide = exam ? views.normal : views.exam;
      hide.hidden = true;
      show.hidden = false;
      if (!reduceMotion.matches) {
        show.animate([{ opacity: 0 }, { opacity: 1 }], { duration: 220, easing: "ease-out" });
      }
    };

    const startClock = () => {
      const t0 = Date.now();
      clearInterval(tick);
      clock.textContent = "0:00";
      // Hours:minutes, like the firmware. It only shows the idea; it won't reach a minute often.
      tick = setInterval(() => {
        const m = Math.floor((Date.now() - t0) / 60000);
        clock.textContent = `${Math.floor(m / 60)}:${String(m % 60).padStart(2, "0")}`;
      }, 5000);
    };

    const setExam = async (on) => {
      if (busy) return;
      busy = true;
      toggle.setAttribute("aria-busy", "true");
      if (on && !reduceMotion.matches) {
        for (const k of keys) {
          k.dataset.pressed = "true";
          await wait(150);
          k.dataset.pressed = "false";
          await wait(110);
        }
      }
      demo.dataset.exam = String(on);
      if (!reduceMotion.matches) {
        // a quick redraw blink, like the real screen refreshing
        lcd.animate([{ opacity: 1 }, { opacity: 0.82 }, { opacity: 1 }], { duration: 180, easing: "ease-out" });
      }
      swapView(on);
      describe(on);
      if (on) startClock();
      else clearInterval(tick);
      toggle.textContent = on ? "Back to the answer" : "Show exam mode";
      toggle.setAttribute("aria-pressed", String(on));
      caption.textContent = on
        ? "On the real calculator, only a USB cable or 12 hours ends exam mode."
        : "A screen recreated from the calculator's firmware.";
      toggle.removeAttribute("aria-busy");
      busy = false;
    };

    toggle.addEventListener("click", () => setExam(demo.dataset.exam !== "true"));
  }

  // ------------------------------------------------------------ FAQ deep links
  // /#faq-exams opens that answer, so support replies can link straight to it.
  const openFromHash = () => {
    const id = decodeURIComponent(location.hash.slice(1));
    const d = id && document.getElementById(id);
    if (d && d.tagName === "DETAILS") d.open = true;
  };
  openFromHash();
  window.addEventListener("hashchange", openFromHash);

  // ------------------------------------------------------------ FAQ printing
  // A printed FAQ should show every answer; put the closed ones back afterwards.
  let printOpened = [];
  window.addEventListener("beforeprint", () => {
    printOpened = $$("[data-faq] details:not([open])");
    printOpened.forEach((d) => (d.open = true));
  });
  window.addEventListener("afterprint", () => {
    printOpened.forEach((d) => (d.open = false));
    printOpened = [];
  });

  // ------------------------------------------------------------ FAQ open/close
  // <details> works on its own; this adds a short height animation when motion is allowed.
  $$("[data-faq] details").forEach((d) => {
    const summary = $("summary", d);
    const answer = $(".faq__answer", d);
    let anim = null;

    summary.addEventListener("click", (ev) => {
      if (reduceMotion.matches || !answer.animate) return;
      ev.preventDefault();
      const opening = !d.open;
      const start = d.offsetHeight;
      if (anim) anim.cancel();
      if (opening) d.open = true;
      const end = opening ? summary.offsetHeight + answer.offsetHeight : summary.offsetHeight;
      d.style.overflow = "hidden";
      anim = d.animate([{ height: start + "px" }, { height: end + "px" }], {
        duration: opening ? 260 : 200,
        easing: EASE_OUT,
      });
      if (opening) {
        answer.animate(
          [
            { opacity: 0, transform: "translateY(-4px)" },
            { opacity: 1, transform: "none" },
          ],
          { duration: 260, easing: EASE_OUT }
        );
      }
      anim.onfinish = () => {
        if (!opening) d.open = false;
        d.style.overflow = "";
        anim = null;
      };
      anim.oncancel = () => {
        d.style.overflow = "";
      };
    });
  });
})();
