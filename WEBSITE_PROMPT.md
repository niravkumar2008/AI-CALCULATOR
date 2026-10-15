# Prompt: build the AI-CALCULATOR launch website

How to use: open a **new chat in the main AI-CALCULATOR folder**, change the "Decisions" lines if you want, then paste everything below the line.

---

Build the public launch website for my AI calculator. Take your time, use the skills I've installed, and work in the order below. Ask me before anything that costs money, publishes, deploys, or touches git (I commit and push myself).

## Decisions
- **Brand name:** none yet. Use "[Brand]" placeholders everywhere and propose 5 names in a separate note (check none is a Casio name or an obvious existing trademark; I'll do a real trademark search later). Never use "Casio" or a Casio model name as our brand.
- **Price:** $225 device + $15/month. Keep it as decided (D3 in `docs/web/PLAN.md`); don't argue it down on the site.
- **Screen:** describe it generically ("a crisp, sunlight-readable display") until I choose between e-paper (v14 board) and colour LCD (v15 board, in design).
- **Main call to action:** "Join the waitlist". We are not taking money yet. Preorders come later.
- **Domain:** none bought yet. Use relative links; where a full URL is unavoidable use `aicalc.example` as the placeholder.

## Approved stack (from my web plan, decision D1; do not change it)
- **Cloudflare Pages, static HTML/CSS plus a little vanilla JS. No framework, no build step.** The site lives in `web/site/` and will share a domain with the API (`/api/*`) and the account portal (`/account`). This is stage 4 (`ai_calc_web_4`) of `docs/web/PLAN.md`; read sections 0, 2, 8 and 9 of that plan first. If `docs/web/` isn't in this folder, it's in the worktree `.claude/worktrees/ai-calc-sell-subscribe-4bd464/`; read it there and don't edit anything in that worktree.
- **Spend $0.** Don't deploy, don't create accounts, don't buy anything. Run it locally only.
- Plain CSS with custom properties (design tokens) for colour, type and spacing, light and dark mode. Motion with CSS transitions and the Web Animations API, no animation library. Any JS is small, deferred, and the page works without it.

## What the product is (facts — use only these, invent nothing)
- An **AI study tutor that looks and feels like a real scientific calculator.** Our own circuit board inside a familiar fx-115ES-style shell: US-standard key layout and a real rubber keymat, so the keys feel like a real calculator.
- **A full calculator that works offline**, running our own calculator engine (fractions, trig, logs, memory, replay, and so on).
- **AI Solve (MODE 4):** a live camera viewfinder on the screen → press `=` to snap a photo of a problem → the answer and the steps, from Claude (Anthropic), shown on the calculator. Good for STEM and basic finance. If the handwriting is unclear it says so instead of guessing.
- **Exam mode (real, in the firmware):** `SHIFT MODE 8 =` switches the AI off and turns the status bar black so anyone can see it at a glance. It ends by itself after 12 hours, or a teacher unlocks it with a USB cable.
- **Setup:** it registers itself the first time it joins Wi-Fi; no codes or URLs to type. Works with 2.4 GHz Wi-Fi or a phone hotspot (school login pages aren't supported). Automatic firmware updates over Wi-Fi are **planned**, not done; say "planned" if you mention them.
- **Hardware:** ESP32-S3, 5 MP autofocus camera on the back, Wi-Fi, magnetic charging connector, 150 mAh rechargeable battery, no LED.
- **Subscription ($15/month):** covers the AI solving, our server and updates. The **first month is free and starts when you first pair the calculator, no card needed**; the calculator says plainly when the free month is over. Fair use: **300 solves a month**. Cancel anytime from the account page. There is no free tier after the first month. The account holder must be 18+ (a parent buys).
- **Privacy:** photos are sent to Claude to solve the problem and are **not stored**; we keep only usage counts. Anthropic is named as the processor. The device never stores an API key.
- **Status:** prototype boards were ordered on 2026-10-08. Working prototypes next, then preorders (planned for 2027). Don't claim it's shipping, tested at scale, certified (FCC is still to do), or reviewed. Battery life is "being measured on prototypes".

## Positioning and rules (important)
- Pitch it as **"the honest AI homework tutor"**: it shows the steps so students *learn*, not just copy answers. Exam mode is the proof: put it front and centre in the "Honest by design" section.
- Say plainly in the FAQ: it is **not allowed on the SAT, ACT or AP exams** (it has Wi-Fi and a camera); use exam mode or leave it at home. **Never** say or imply: exam-approved, "undetectable", stealth, a panic/hide button, "cheat".
- Audience: high school and college students (13+) and their parents. Don't market to kids 12 and under (no "for kids" copy, no cartoon kid imagery, no faces of minors).
- No Casio name, logo or trade dress claims in our branding. Footer: "Not affiliated with Casio Computer Co., Ltd."
- **No fake content:** no made-up reviews, testimonials, ratings, user counts, press logos or "as seen on". Leave those sections out.
- Don't name competitors. Read `Claude outputs/COMPETITOR_ANALYSIS.md` (sections 0 and 1.4) and quietly win on: honest positioning, real Casio-style key feel, a camera model that actually ships, published specs, and a crystal-clear subscription explanation.
- Legal pages (privacy, terms, refunds/warranty) are templates marked **"NEEDS LAWYER REVIEW"** at the top. Link them from the footer.
- **Never read, copy or ship `api_key.txt`** or any secret.

## Assets already made (use them, don't rebuild)
0. If this folder has no `website/` folder, copy it from the worktree where it was made: `.claude/worktrees/install-ui-ux-pro-max-skill-f85942/website/` → `website/` (skip `node_modules` and `video/out`; run `npm i` in `website/video` afterwards), and copy `.claude/worktrees/install-ui-ux-pro-max-skill-f85942/sim/web/make_embed.py` → `sim/web/`. Don't change anything else in that worktree. Then read `website/README.md`.
- **Try-it simulator:** `website/simulator/index.html` is the real calculator engine, calculator only, for an `<iframe>`. `MODE 4 =` cycles through 3 demo AI answers. No API key field, no network calls. It's ~500 KB, so lazy-load it (load on click or when scrolled into view). Rebuild with `python sim/web/make_page.py && python sim/web/make_embed.py`.
- **AI Solve video:** `website/assets/video/ai-solve.mp4` (12.8 s, 1080p, silent) + `ai-solve-poster.jpg`. Use `autoplay muted loop playsinline` with the poster, and a pause control. With `prefers-reduced-motion`, show the poster and a play button instead of autoplay. The video's end card says the placeholder brand "AI·CALC": once a name is chosen, change `brand=` in `website/video/src/AiSolveVideo.tsx` and re-render (see `website/README.md`). Remotion is free for teams of up to 3.
- **Product images:** use `website/renders/out/` (studio renders) if it has images. If it's empty, tell me to open Fusion so you can run `website/renders/studio_renders.py`, and meanwhile use `hardware/enclosure/final_assembly/renders/hero_front.png`, `hero_back.png` and `exploded_front.png`. Export web sizes (AVIF/WebP + PNG/JPEG fallback, with `width`/`height` set). Don't modify the originals.
- **Real photos** come later (`website/PHOTO_SHOT_LIST.md`). Leave a clear spot for them, but don't use stock photos or AI-generated images of the product.

## Skills to use, in this order
1. **`ui-ux-pro-max`**: run its design-system search for "consumer hardware edtech study tool, premium, trustworthy, students 13+" with `--design-system`. The video already uses a dark navy (#10141c), warm orange accent (#e07a4f, the calculator's DEL/AC keys), Barlow / Barlow Semi Condensed and IBM Plex Mono; prefer a system that fits those so the video matches, or tell me what to change. Show me the palette, fonts, style and spacing and **wait for my OK** before building.
   - Optional: if I name a site I like, run **`extract-design-system`** on it for inspiration only, never as a copy.
2. **`brandkit`** and **`image-to-code`**: we have no image generator, so use their rules as the art-direction checklist (a clean, spacious hero; no cards inside cards; one idea per section; strong type hierarchy; readable on a small laptop) and our real renders as the visuals.
3. **`emil-design-eng`** + **`apple-design`**: the polish bar (easing, shadows vs borders, depth, typography, restraint).
4. **`21st-ui`** (21st.dev), **only if its server is connected** (it needs my `API_KEY_21ST`): look up strong patterns for the hero, feature grid, FAQ accordion, pricing card and waitlist form, then **port them to plain HTML/CSS** (no React). Skip quietly if it isn't connected.
5. **`design:ux-copy`**: write all copy. Short, plain, confident and honest, no hype words. The video's lines are the voice: "Snap a problem." "Press = to shoot." "See every step." "Learn every step."
6. **`find-animation-opportunities`** → **`animate`**: motion only where it helps (hero product reveal, the 3 "How it works" steps, the specs scroll-in). Always respect `prefers-reduced-motion`.
7. **`mobile-native`**: most visitors will be on phones (100vh, tap highlight, input zoom, safe areas, the iframe on small screens).
8. Review pass, in this order, then fix everything they find:
   - **`break-ui`** (long text, missing images, slow network, 320 px wide, the iframe failing to load)
   - **`web-design-guidelines`**
   - **`review-animations`**
   - **`design:accessibility-review`** (WCAG 2.1 AA: contrast, keyboard, focus, alt text, form labels, video controls)

## Site structure
One landing page, `web/site/index.html`, plus the legal templates.
1. **Hero:** product render, the promise ("Snap a problem. Learn every step."), one sub-line, and the waitlist email field + button. Fits above the fold on a 13" laptop and on a phone.
2. **How it works:** 3 steps (Point → Press `=` → Learn the steps) with the AI Solve video.
3. **Try it:** the simulator iframe, with "Press MODE, 4, then =" next to it, and a note that it uses demo answers.
4. **Built like a real calculator:** real keymat feel, full offline calculator, magnetic charging, sets itself up on Wi-Fi, no LED. One short line each.
5. **Honest by design:** shows the steps, not just answers; exam mode (AI off, black status bar, teacher unlock); not allowed on SAT/ACT/AP; why that matters to parents.
6. **Specs:** a clean table (processor, camera, screen [generic], battery, charging, connectivity). Publishing specs builds trust.
7. **Pricing:** $225 + $15/month; what the subscription covers; first month free from first pairing, no card; 300 solves a month; cancel anytime; parent (18+) is the account holder.
8. **FAQ:** exams; ages; privacy (photos not stored, Anthropic processes them); Wi-Fi (2.4 GHz or phone hotspot, not school login pages); what happens after the free month; battery life ("being measured on prototypes"); when it ships ("preorders planned for 2027"); which subjects work (STEM and basic finance).
9. **Footer:** waitlist again, contact email placeholder, legal links, "Not affiliated with Casio Computer Co., Ltd."

## Build details
- The waitlist form: front end only. It should POST to `/api/waitlist` (a `TODO` comment says the Worker endpoint doesn't exist yet), with a friendly message if that fails. Don't sign me up for any service or add any keys.
- Performance: no layout shift, images sized and lazy below the fold, the video and iframe deferred. Aim for Lighthouse 90+ on mobile for performance, accessibility, best practices and SEO.
- SEO: title, meta description, Open Graph image (from a render or the video poster), a favicon from a simple unbranded mark.
- Serve it locally (for example `python -m http.server` in `web/site/`) and show me in the browser pane at phone, tablet and desktop widths, with a screenshot of each section.

## When you're done
- A short summary: what's built, what each skill found, what's still a placeholder, and the decisions you need from me.
- Run `graphify update .`.
- Append a dated note of the design decisions to my Obsidian project hub (`03 Business/AI-CALCULATOR/AI-CALCULATOR.md`).
- Don't commit, push or deploy.
