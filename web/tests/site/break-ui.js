// Dev-only worst-case harness for web/site/index.html. Not part of the site; never deploy it.
// Use: open the site locally, paste this into the browser console (or load it as a bookmarklet),
// then use the "Demo / Worst case / API down / API slow / Images gone / Sim 404" control.
// Every case enters through the same boundary real data does: text content, fetch, image URLs,
// and the iframe URL. No CSS is edited.
(() => {
  const BRAND_LONG = "Northwind Study Instruments"; // realistic three-word brand name
  const EMAIL_LONG = "bartholomew.fitzgerald@northwind-industries-holdings.example.com";
  const realFetch = window.fetch.bind(window);
  const snapshot = new Map();

  const remember = (el, key, value) => {
    if (!snapshot.has(el)) snapshot.set(el, {});
    const s = snapshot.get(el);
    if (!(key in s)) s[key] = value;
  };

  const reset = () => {
    for (const [el, s] of snapshot) {
      if ("text" in s) el.textContent = s.text;
      if ("src" in s) el.src = s.src;
      if ("srcset" in s) el.srcset = s.srcset;
      if ("value" in s) el.value = s.value;
    }
    snapshot.clear();
    window.fetch = realFetch;
    document.querySelectorAll(".img-missing").forEach((e) => e.classList.remove("img-missing"));
  };

  const cases = {
    Demo: () => {},
    "Worst case": () => {
      document.querySelectorAll(".brand span").forEach((s) => {
        remember(s, "text", s.textContent);
        s.textContent = BRAND_LONG;
      });
      document.querySelectorAll("input[type=email]").forEach((i) => {
        remember(i, "value", i.value);
        i.value = EMAIL_LONG;
      });
    },
    "API down": () => {
      window.fetch = () => Promise.reject(new TypeError("Failed to fetch"));
    },
    "API slow": () => {
      window.fetch = () => new Promise((r) => setTimeout(() => r(new Response("{}", { status: 201 })), 6000));
    },
    "Images gone": () => {
      document.querySelectorAll("picture source").forEach((s) => {
        remember(s, "srcset", s.srcset);
        s.srcset = "missing.avif";
      });
      document.querySelectorAll("img").forEach((i) => {
        remember(i, "src", i.src);
        i.src = "missing/" + Math.random() + ".png";
      });
    },
    "Sim 404": () => {
      const t = document.querySelector("[data-sim-template]");
      const f = t && t.content.firstElementChild;
      if (f) {
        remember(f, "src", f.getAttribute("src"));
        f.src = "try/does-not-exist/";
      }
    },
  };

  document.getElementById("__break-ui")?.remove();
  const bar = document.createElement("div");
  bar.id = "__break-ui";
  bar.style.cssText =
    "position:fixed;left:50%;bottom:12px;transform:translateX(-50%);z-index:9999;display:flex;gap:2px;padding:3px;border-radius:999px;background:#e5e5e5;font:12px system-ui;box-shadow:0 2px 8px rgba(0,0,0,.2);max-width:calc(100vw - 16px);overflow-x:auto";
  for (const name of Object.keys(cases)) {
    const b = document.createElement("button");
    b.textContent = name;
    b.style.cssText = "border:0;border-radius:999px;padding:6px 10px;background:transparent;cursor:pointer;white-space:nowrap";
    b.onclick = () => {
      reset();
      cases[name]();
      bar.querySelectorAll("button").forEach((x) => (x.style.background = "transparent"));
      b.style.background = "#fff";
      const u = new URL(location.href);
      u.searchParams.set("data", name.toLowerCase().replace(/\s+/g, "-"));
      history.replaceState(null, "", u);
    };
    bar.append(b);
  }
  document.body.append(bar);
  bar.firstChild.style.background = "#fff";
})();
