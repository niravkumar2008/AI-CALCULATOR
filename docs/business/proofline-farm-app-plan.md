# Proofline Farm: plan for the always-on AI ecosystem app

Status: design only (2026-10-18). Nothing here is built yet, and nothing has been committed.
Starting point: the Proofline Farm wallpaper (`tools/wallpaper-dashboard/index.html`) and the Agent Watch mod
(`~/.claude/mods/agent-watch`, source in `tools/agent-watch/mod`). Both are on branch `claude/sub-agent-visibility-047e40`, uncommitted.

## The idea in one paragraph

Today the farm only **shows** what your chats are doing. The app adds a **farm manager** that keeps working while you
are away. You drop tasks in a seed bag. The manager plants each one in its own field (its own copy of the code), sends a
robot farmhand (a headless Claude run) to work it, then sends an inspector (a second Claude run with a fresh mind) to
check the result against the task's "done when" list. Good work goes to the Harvest store for you to review and commit.
Anything that would leave your computer (a post, an email, spending money) goes to the **mailbox** and waits for your
yes. When your Claude plan's usage runs out, the farmhands go to sleep and wake up when it resets. The wallpaper keeps
working from the same data.

---

## 1. The agent loop

### Why self-prompting has been "good, not great"

When one chat keeps prompting itself, three things go wrong:
1. **No finish line.** "Improve the firmware" never ends, so the agent wanders.
2. **The agent grades its own homework.** It wrote the code, so it believes the code works.
3. **The chat gets long.** After hours, early details get squeezed out and quality drops.

The loop below fixes each one: every task has a written finish line, a different run does the checking, and every
run starts with a clean, short context.

### The parts

```
 seed bag (tasks/)            planner (sometimes)
       |                           |
       v                           v
  +----------------- farm manager (small Node script, no AI) -----------------+
  | picks the next ready task, checks the usage budget, makes a worktree,     |
  | starts a worker, watches it, starts the checker, writes the feed          |
  +---------------------------------------------------------------------------+
       |                 |                    |                    |
       v                 v                    v                    v
   worker            checker            mailbox (approvals)     feed (state.json)
 claude -p          claude -p           nothing outward          -> wallpaper
 own worktree       fresh context       happens without you      -> app / phone
```

**The farm manager is plain code, not an AI.** It never "thinks"; it only follows rules: which task is next, is there
budget, is a worker stuck, did the checker pass. That makes it predictable and free to run.

### The backlog: one file per task

Folder: `C:\Users\r_kas\ProoflineFarm\tasks\` (outside OneDrive and outside the repo, so syncing and worktrees don't fight
over it). One Markdown file per task, readable by you and by the script:

```markdown
---
id: T-0012
title: Add a unit test for the x^-1 key
kind: code              # code | marketing | research | hardware-check
repo: AI-CALCULATOR
priority: 2             # 1 = first
status: ready           # idea | ready | working | checking | harvest | failed | needs-you | done
tries: 0                # the manager stops after 2 failed tries
budget: small           # small ~ 20 min of work, medium ~ 1 h; bigger tasks must be split
needs_approval: false   # true for anything outward-facing
blocked_by: []          # e.g. [T-0010] waits until you commit T-0010
---
## What to do
Write a test in calculator-core that presses x^-1 on 4 and expects 0.25.

## Done when
- cmd: npm test --prefix calculator-core        # the manager runs this itself; must exit 0
- cmd: git diff --stat                           # must show changes only under calculator-core/
- judge: the new test fails if the key is broken  # the checker AI decides this one
```

The **"Done when" list is the most important part.** `cmd:` lines are checked by the script with no AI (cheap and
honest). `judge:` lines are for the checker. A task with no "Done when" list is not allowed to start; it stays an `idea`.

### The worker (farmhand)

- Started by the manager as `claude -p "<task file + rules>" --output-format stream-json` inside a fresh git worktree
  made for that task (`git worktree add ...` or the CLI's own `-w` flag). Fresh context every time, so no long-chat drift.
- Uses your normal Claude login (your subscription), so there is no separate API bill (see section 2).
- Gets a short fixed rule sheet appended with `--append-system-prompt`: stay in this worktree, do only this task,
  don't commit, don't push, write `REPORT.md` with what you did and anything you are unsure about.
- Tool limits with `--allowedTools` / `--disallowedTools`: it can read, edit and run tests; it cannot `git push`,
  `git commit`, send email, post, or touch payment tools. A **PreToolUse hook** in the worker's `--settings` file blocks
  the same list a second time (belt and braces, in case a rule is missed).
- The manager reads the stream as it runs, so the farm can show the farmhand's current tool, just like Agent Watch does now.
- Watchdog: no new output for 10 minutes, or the same tool call repeated 5 times = stuck. The manager stops it and
  marks the task `failed` with the reason.

### The checker (inspector)

A separate `claude -p` run that has **not** seen the worker's chat. It gets: the task file, the diff, the `cmd:` results
and the worker's `REPORT.md`. It answers in a fixed JSON shape:

```json
{ "verdict": "pass | fail | needs-you", "why": "...", "fix_notes": "...", "next_tasks": [ {"title": "...", "done_when": ["..."]} ] }
```

- `pass` → status `harvest`: a mailbox card "ready to commit" with the diff summary. **You commit** (or press a button
  that runs the commit for you after you've looked).
- `fail` → the task goes back to `ready` with the checker's `fix_notes` added, `tries + 1`. After 2 fails: `needs-you`.
- `needs-you` → mailbox card with the question.
- `next_tasks` go straight to `ready` (your decision 4), but only if they have a "Done when" list. Without one they stay `idea`. Anything outward-facing still goes through the mailbox.

The checker can run on a cheaper/faster model than the worker to save usage.

### The planner (sometimes)

Once a day (or when the ready list is empty) a planner run reads `goals.md` (your short list of what matters this
month, e.g. "v15 firmware Apps menu", "waitlist page copy", "Kickstarter prep"), the latest harvests and the idea pile,
and proposes up to 5 tasks with "Done when" lists. Rules:
- Its tasks go straight to `ready` (your decision 4), and you see them in the morning summary.
- Tasks without a "Done when" list, or bigger than `medium`, stay as `idea` until they are split up.

### Budget and usage-limit awareness

Your Claude plan has a rolling usage window (roughly every 5 hours) plus a weekly cap. The farm must not eat the usage
you need for your own chats. Rules:

1. **Detect the wall.** When a run ends with the usage-limit message, the manager reads the reset time from it, marks
   every waiting farmhand `asleep`, and sets a wake-up for reset + 2 minutes. On the farm: night falls on those fields,
   farmhands sleep in the barn, a clock over the barn shows the reset time.
2. **Leave room for you.** Agent Watch already knows when you have a chat open and working. While you are actively
   chatting, the farm runs at most 1 farmhand. When you're not chatting, it may run more (up to 3 to start), within
   the budget rules (your decisions 1–2).
3. **Count what it spends.** Each run's JSON result includes token usage and a cost estimate. The manager adds them up
   per window and per day. The Claude desktop app shows the real "% used" numbers for the 5-hour and weekly limits
   (see "Weekly usage budget" below), and the manager uses its own count to fill the gaps between those readings.
4. **Size limits.** `budget: small/medium` maps to a time limit and a turn limit; a run that hits it is stopped and
   reported, not left running.
5. **Weekly budget.** A reserve of the weekly limit stays yours, and the farm spreads its share over the week. See "Weekly usage budget" below.

### Weekly usage budget: always leave enough for your own use

You use Claude for more than Proofline (school, other projects). The farm must never leave you with nothing for the
rest of the week. The **weekly** limit matters most here. The 5-hour limit refills every few hours, but once the weekly
limit is used up you wait days.

**Your numbers today (read from the Claude app, 2026-10-18 evening):** plan **Max** (you confirmed 2026-10-18; the app's usage card showed "Pro", but the `claude` command reports Max). Weekly limit (all models): **16%
used**, resets **Wednesday 21 Oct, about 6 pm** your time (22:00 UTC). It resets every Wednesday at that time (corrected 2026-10-18: Oct 21 is a Wednesday). 5-hour limit:
3% used. There is also a separate weekly limit for the Fable model (0% used). **Extra usage (pay-as-you-go) is off**:
when the limit is reached Claude just stops, it never charges you more. Keep it off; the farm must never be a reason to
pay extra.

#### The rules

1. **Your reserve comes first.** A part of the weekly limit belongs to you. The farm stops for the week when total
   weekly use reaches `100% − your reserve`. **Your reserve starts at 30%** (your choice, 2026-10-18): the farm can
   push the week to 70% at most, and the last 30% is yours. After that the reserve **learns** from how you really use
   Claude (rule 3). The farm never works past the line, not even "just to finish a task". A task that is cut off goes
   back to `ready` and continues next week.
2. **Spread it over the week (pacing).** Without pacing, the farm would use its whole share on Wednesday night and
   be idle until Wednesday. So it gets a daily allowance:
   `farm allowance today = (farm share left) ÷ (days until reset)`.
   Example: the farm share is 70% − 16% = 54% with 2.8 days left, so about **19% per day**. Leftover allowance from a
   quiet day carries over. A busy day doesn't steal from tomorrow.
3. **The reserve learns how much you really use (on, decided 2026-10-18).**
   - **What it records:** the manager knows the farm's own usage, so every hour it saves
     `your use = total weekly % − farm %` in `ProoflineFarm\usage-history.json`. It keeps a separate average for each
     weekday, because school days, weekends and project days differ.
   - **What it works out:** at any moment, your reserve is what you will probably still need before Wednesday's reset:
     `reserve = (your usual use for each day left this week, added up) × 1.5`. The 1.5 is a safety margin for a busier
     week than usual. Example: you usually use 4% on a school day and 8% on a Saturday. On Friday night with Sat, Sun,
     Mon, Tue left, the reserve is (8 + 8 + 4 + 4) × 1.5 = 36%. On Tuesday night, with only Wednesday left, the maths gives
     6%, but the 25% floor below applies, so the reserve is 25%.
   - **Limits, so learning can't hurt you:**
     - The first **2 weeks** use the fixed 30%. The learned number only takes over once there are 2 weeks of data.
     - The reserve **never goes below 25%** of the full week, whatever the maths says.
     - A raise (more for you) applies straight away. A cut (more for the farm) is limited to **5 points per week**,
       so one quiet week can't hand the farm your next busy one.
     - A week where you ran out of usage, or pressed "I need Claude", counts as a busy week and pushes the reserve up.
     - Weeks where the farm was off don't change anything, so the data isn't skewed.
   - **You can always see and undo it:** the budget barn shows "Reserve this week: 34% (learned from the last 3 weeks)"
     with a **Reset to 30%** button. Every Wednesday the farm writes a one-line note to the mailbox: "Last week you used
     X%, the farm used Y%, reserve is now Z%."
4. **The 5-hour limit is protected too.** During your awake hours the farm keeps the 5-hour limit below about 60%, so
   you can always start a chat and get real work done. At night (farm hours) it may use the whole 5-hour limit, but
   only inside its weekly allowance.
5. **"I need Claude" button.** In the budget barn, and as `/farm reserve` in the mod: "pause the farm until Monday" or
   "reserve 70% this week" (exam week, a big school project). It applies straight away and is undone automatically at
   the next weekly reset.
6. **Use the cheaper model where possible.** Even on Max, a smaller model gets more done from the same share. Workers use Sonnet. The checker and planner
   use Haiku when that's good enough. The farm never uses the most expensive model unless a task says so. A smaller
   model gets more tasks done from the same weekly share.

#### Where the "% used" numbers come from

- **Best source:** the Claude desktop app already shows these numbers (the usage card, and a tool inside Claude
  sessions that reads them). The plan is for the Agent Watch mod, which already runs inside every chat, to write them
  to `ProoflineFarm\usage.json` every few minutes while any chat is open. Whether a mod can read them directly still
  needs checking. If it can't, a tiny scheduled check (once an hour, very little usage) writes the numbers instead.
- **In between** (at night, when no chat is open): the manager adds the farm's own usage, from each run's result, to
  the last real reading. That stays close as long as you aren't using Claude somewhere else at the same time (phone,
  claude.ai). Phone or web use isn't counted until the next real reading, so the farm treats an old reading as a
  reason to be careful. If the newest reading is more than 6 hours old, the farm works at half its allowance.
- **Safety net:** the "usage limit reached" message from a run (rule 1 of the section above) still stops everything,
  even if the numbers were wrong.

#### What you see

The **budget barn** shows the week as one bar with three colours: *your use*, *farm use*, *your reserve* (fenced
off). A line marks where the farm should be today (pacing) and a clock counts down to Wednesday's reset. If the farm hits
its line for the day, its farmhands sit at the campfire until tomorrow's allowance. If the reserve line is reached, the
whole farm goes to bed until Wednesday and a sign on the barn says why.

**On Max:** your weekly limit is several times bigger than Pro's, so with a 30% reserve the farm can do a real night's
work. The first real task used well under 1% of the week. The rules work in percentages, so they hold on any plan. The
farm will never change your plan or turn on extra (paid) usage.

### The approval queue (mailbox)

Anything that leaves your computer or costs money is **never done by an agent**. Agents can only **draft**:
- marketing post → `drafts/post-<id>.md` (text + image path + where it would go)
- email → a Gmail **draft** at most (the Gmail tool's `create_draft`, never `send`), or just a text file
- spending (parts order, ads, domain) → a card with item, price, link; never a purchase

Each draft becomes a mailbox card: **Approve / Edit / Reject**. Approve on a post or email means *you* send it (or a
plain script sends exactly the text you approved, nothing re-written by AI). Spending is always done by you by hand.
Every card keeps a log line (who approved, when, what text), so nothing goes out you didn't see.

---

## 2. Where it runs without paying for a website

Two separate questions: **where the farmhands work** (needs your repo, Claude login, tests) and **where you look at it**
(wallpaper, app, phone).

### Where the work runs

| Option | What it costs | Good | Not good |
|---|---|---|---|
| **A. Your main PC, on 24/7** | $0 up front. Electricity for a desktop idling all night: about 60–150 W → roughly $7–17/month at $0.15/kWh | Already set up: repo, KiCad, Fusion, Gmail connector, wallpaper | PC has to stay on; farmhands running tests while you game or do CAD can slow it; sleep/updates/OneDrive can break runs |
| **B. Spare laptop, always on** | $0 if you have one. Power about 10–20 W → under $2/month | Your PC stays free; the manager itself is tiny | Needs a one-time setup (git, Node, Claude login, repo clone); no KiCad/Fusion work unless installed |
| **C. Raspberry Pi 5 (8 GB) + small SSD** | About $100–130 once. Power about 5 W → about $0.50/month | Silent, cheap, made to stay on; Claude Code runs on 64-bit Linux | Slow for heavy builds; no Windows tools (KiCad GUI, Fusion, Wallpaper Engine); Linux to learn |
| **D. Claude Code cloud routines (`/schedule`)** | Included in your plan (they use your plan's usage; plans have a daily cap on scheduled runs) | Runs even with every computer off; starts from your GitHub repo | Only sees what's in GitHub + connectors; no local files, no KiCad/Fusion, no wallpaper; can't run every few minutes |
| **E. GitHub Actions cron** | Free for public repos; private repos get 2,000 Linux minutes/month free, then pay. Claude via your plan's token (`claude setup-token`) or an API key | Runs in the cloud on a timetable; good for "nightly check" jobs | Each job starts from scratch (slow), max 6 h per job; secrets in GitHub; API-key billing if you go that way |

**Honest recommendation:**
- **Now (phase 1–2):** option A, your PC, with "only 1 farmhand while I'm chatting" on. The manager is a
  small Node script (about 30–50 MB of memory, close to 0% CPU while waiting). The heavy thinking happens on
  Anthropic's servers; what does load your PC is the tests and builds the farmhands run, and you control that with the
  number of farmhands (start with 1).
  You said you don't want to host a web server from your PC; this plan doesn't need one. The manager talks to the
  wallpaper through files, like Agent Watch does now.
- **Later (phase 4):** move the manager to a spare laptop (B) if you have one, otherwise a Pi (C). Your PC then only shows
  the wallpaper.
- **Backup heartbeat:** one cloud routine (D) a day for repo-only jobs (e.g. "run all tests on main, file a task if
  something broke") so something still happens when the house computer is off.
- Skip E unless you make the repo public or need a job that has to run in a clean machine.

### Subscription vs API tokens (rough numbers, check current prices)

- **Subscription (what you have):** a fixed monthly price; the farm uses the same usage windows as your chats. Running
  farmhands all night is "free" until you hit the window, then they wait. This is by far the cheapest way to keep agents
  busy. Bigger plans (Max) mainly buy bigger windows. Check Anthropic's current terms for running Claude Code headless and
  on a schedule with a personal plan before going heavy; personal automation of your own work is the normal use of
  `claude -p`, but the terms are theirs to set.
- **API key (pay per token):** no windows, but every token costs money. At Sonnet-class prices (around $3 per million
  input tokens and $15 per million output, less for cached input), a small coding task often costs somewhere around
  $0.30–$3, more if it reads a lot of files. 20 tasks a day ≈ $6–60/day, i.e. easily $200+/month, which is 10% of the whole
  $2,000 budget. Use the API only with a hard cap (`--max-budget-usd` per run, plus a monthly limit set in the Anthropic
  console), and only where the subscription can't go (e.g. GitHub Actions).
- The **Claude Agent SDK** (the library version of Claude Code) is the right tool when the manager is built as a real app
  and once Proofline offers anything to other people; it is normally billed with an API key. For your own farm,
  calling the `claude` command is simpler and uses your plan.

### Where you look at it (no paid hosting)

| Viewer | Cost | Notes |
|---|---|---|
| **Wallpaper** (already exists) | $0 | Reads a local file. Keeps working exactly as now; the farm just adds data. |
| **Local app window** | $0 | The same `index.html` opened as a file. Read-only from a file; to press buttons it writes "command files" through the mod (see 3). No server. |
| **Claude Code mod** (`/farm` commands, already have `/agents`) | $0 | Add tasks, stop a farmhand, approve a card from any Claude chat. |
| **Phone/away view: Cloudflare Pages + Worker + D1** | $0 on the free plan | Pages hosts the farm page for free. A small Worker takes status from the manager and stores it. D1 (the database the subscribe layer already uses) allows 100,000 writes/day free. KV allows only 1,000 writes/day on the free plan, so pushing every 3 s (28,800/day) would not fit; push on change, at most every 30–60 s, or use D1. Protect it with Cloudflare Access (free for up to 50 users) so only your Google login can open it. This matches the "spend $0 until essential" decision for the Cloudflare plan. |
| GitHub Pages | $0 | Fine for a read-only page, but it can't take button presses or hold private status; Cloudflare is the better fit since you're already going there. |

The phone view does **not** need your PC to accept connections: the manager pushes status **out** to the Worker and
pulls new tasks/approvals **back**, every minute or so. Nothing on your PC is opened to the internet.

---

## 3. The app UI

One world, three ways to see it: wallpaper (watch only), app window (watch + act), phone page (same as the app window).

- **The farm canvas is the main view.** Same pixel world as now. Each task being worked is a field with its title on
  the fence; the farmhand acts out its current tool (already built). New: the **inspector** (checker) walks the field with
  a clipboard when a task is being checked; a green flag = pass, a red flag = fail.
- **Click a farmhand** → side panel: task title, "Done when" list with ticks, live log (last 50 lines of tool calls,
  from the stream), time and usage spent, and buttons:
  - **Stop** — kills the run, task back to `ready`.
  - **Redirect** — type a sentence ("only change the test, not the key code"); the manager stops the run and restarts it
    on the same task with your note added (or resumes that session with `--resume` so it keeps what it learned).
  - **Open folder** — the task's worktree.
- **Seed-bag task board** (the shed by the gate): columns *Ideas · Ready · Working · Checking · Harvest · Needs you*.
  "Add seed" form: title, what to do, "Done when" lines (with examples you can tap), kind, priority. Dragging an idea to
  Ready is how you approve planner ideas.
- **Mailbox** (by the farmhouse, the flag goes up when something waits): approval cards (posts, emails, spending),
  "ready to commit" harvests, and "needs you" questions. Approve / Edit / Reject. The amber lantern from today's
  wallpaper now means "the mailbox has something".
- **Budget barn:** a meter for this usage window and today, the reset clock, the farm-hours switch, and a big
  **Pause farm** lever.
- **The wallpaper keeps working from the same feed.** The manager writes `state.json` (plus an `agents.js`-style
  `farm.js`) in the same way Agent Watch writes `agents.js` now, and copies it into the Wallpaper Engine project. Old
  wallpaper versions that don't know about the farm simply ignore the new section.

### How buttons work without a server

The app window and phone page never run commands themselves. A button only writes a small **command** (`stop T-0012`,
`approve M-0031`, `add task {...}`) to `ProoflineFarm\inbox\` (local, through the mod) or to the Worker's database (phone).
The manager picks commands up every few seconds, checks them, and acts. One place does things, so it's easy to log and
hard to get wrong.

---

## 3b. Keeping the Wi-Fi alive overnight

**The problem:** at night the Wi-Fi stops working while Claude is running, and turning the Wi-Fi off and on fixes it.
When the internet drops, every farmhand's run fails, and without a fix the farm would mark good tasks as "failed" and
lose the whole night.

**What this PC's logs show (checked 2026-10-18):** the adapter is an **Intel Wi-Fi 6E AX210**. Windows logged 46
Wi-Fi disconnects in the last 10 days. Almost all of them say *"The network is disconnected by the driver"*, and they
happen during the day too (several within minutes on 10-15, 10-16 and 10-18). So the Wi-Fi driver is dropping
the connection on its own. At night nobody is there to notice, so it stays down. Wireless power saving on mains power is
already set to "Maximum Performance", so the usual power-plan setting is not the cause. The PC also has a wired
**Realtek Ethernet port** that isn't being used.

### Fix 1: the watchdog (the automatic off-and-on)

A small PowerShell script does what you do by hand:

1. Every 2 minutes it checks for **real internet**, not just "Wi-Fi connected". (At night the PC can show "connected"
   while nothing gets through.) It tries `https://api.anthropic.com` and falls back to `1.1.1.1`.
2. After **3 failed checks in a row** (about 6 minutes, so a short blip doesn't trigger it), it runs
   `Restart-NetAdapter -Name "Wi-Fi"`. That is exactly "turn it off and on". It waits 30 s and checks again.
3. If 3 restarts in a row don't help, it waits 15 minutes before trying again, so it doesn't flip the Wi-Fi all night.
4. It writes every drop and restart to `ProoflineFarm\logs\wifi.log` (time, how long it was down, whether the restart
   worked). That gives you a record of when it happens.

Restarting the adapter needs administrator rights. So the script runs as a **Windows Scheduled Task** set to "Run
with highest privileges" and "Run whether user is logged on or not". You create it once by hand (one admin click).
After that it needs no password and no Claude usage, and it runs even when the farm is off.

**Built (2026-10-18):** `tools/farm/wifi-watchdog.ps1` and the one-time installer `tools/farm/install-wifi-watchdog.ps1`.
Install steps are in `tools/farm/README.md`. It passed a dry-run test of the offline/restart/wait/recover sequence and
a real online check. It has not yet been tested on a real night-time drop. The real check is an HTTPS request to the
Claude API (any reply, even an error, proves the internet works), with Microsoft's connection-test page as a backup.
It uses no ping, because ping can work while DNS is broken.

### Fix 2: the farm handles drops calmly

- The manager reads the watchdog's state file. While the internet is down, it **doesn't start new runs**, and a run
  that died from a connection error goes back to `ready` **without** counting as a failed try.
- On the farm: a "NO SIGNAL" sign goes up on the farmhouse radio mast, the farmhands wait under the barn roof, and the
  morning log shows "Wi-Fi down 02:14–02:21, restarted once".

### Fix 3: stop the drops at the source (do these once, cheapest first)

1. **Use a cable if you can.** The PC has an Ethernet port. A long flat cable or a powerline adapter
   (about $30–40) avoids the problem completely. This is the most reliable fix for an always-on farm.
2. **Update the Intel Wi-Fi driver** from Intel's site (Intel Driver & Support Assistant). Windows Update often ships
   an older version, and AX210 drops are commonly fixed by a newer driver.
3. **Device Manager → Network adapters → Intel Wi-Fi 6E AX210 → Properties:**
   - *Power Management* tab: untick "Allow the computer to turn off this device to save power".
   - *Advanced* tab: set "MIMO Power Save Mode" to "No SMPS", and "Roaming Aggressiveness" to "Lowest". The PC doesn't
     move, so it shouldn't hunt for other access points at night.
4. **VPN:** ExpressVPN is installed. If it is connected overnight, its own reconnects can look like a dead internet.
   Turn it off before leaving the farm running, or check the watchdog log to see whether drops line up with it.
5. **Router:** if drops keep happening at the same time each night, check whether the router has a nightly reboot,
   a Wi-Fi schedule or "band steering" that moves the PC between 2.4/5/6 GHz. Pinning the PC to the 5 GHz network
   often helps.

**Done looks like:** after a night of farm work, `wifi.log` shows every drop was fixed by the watchdog within about
6–8 minutes, no task was marked failed because of the internet, and over a week (after fixes 3.2–3.3, or a cable) the
number of drops goes down.

---


## 4. Your decisions (2026-10-18)

| # | Question | Your answer | What the farm does |
|---|---|---|---|
| 1 | When may it use more than one farmhand? | Any time you're not chatting, as much as it wants, within the rules | When Agent Watch shows no chat of yours working, it runs as many farmhands as the budget rules allow (weekly reserve, daily pacing, 5-hour cap). It starts at a maximum of **3 at once**, so your PC stays usable and the farmhands don't trip over each other's git work. Raise it later in settings. |
| 2 | While you chat? | Keep 1 farmhand running | The moment you start working in a chat, extra farmhands finish their current step and park. 1 keeps going. |
| 3 | Where does it run? | Your PC first, another laptop later | Phase 1–3 on this PC. Moving to the laptop is phase 4. The manager is just files and a script, so moving is a copy plus a Claude login. |
| 4 | Do new tasks wait for you? | No, start everything on its own; give me a review/summary every so often | Planner and checker tasks go straight to `ready`. You get a summary (below). **Two things still always wait for you:** anything outward-facing (posts, emails, spending) goes to the mailbox, and commits stay yours, because harvests wait in their worktrees until you commit. That's your earlier rule, and "start on its own" doesn't change it. |
| 5 | Phone view soon? | Yes, you're on your phone a lot | The phone view moves up to **phase 2** (see the build order). |
| 6 | Ethernet cable? | Possible, but you'd rather not buy one and the router is far | No cable. The plan is the Wi-Fi watchdog (**installed 2026-10-18**, first check passed) plus the Intel driver update and adapter settings from section 3b, Fix 3. |
| 7 | Weekly reserve | 30% to start, then learn | See "Weekly usage budget". |

### The summary you asked for (decision 4)

Because tasks now start without you, the summary is how you stay in charge:
- **Morning summary** (each day, around when you wake up): what got done overnight, what's waiting for you to
  commit, what failed and why, what the planner added, and usage spent versus your reserve. It shows up in three places:
  1. the app's mailbox and the phone view (top card),
  2. a short line on the wallpaper ticker,
  3. in a Claude chat: type `/farm summary` in any chat to get it. If you like, a scheduled Claude task can also open a
     short "Farm morning summary" chat each morning (it costs a little usage, so it's off unless you turn it on).
- **Weekly review** (Wednesday, at the usage reset): the week's harvest, what the planner is aiming at next week, and
  the reserve note. If something looks off, you change `goals.md`, or delete tasks, and the farm follows.
- **Brakes you always have:** the Pause lever (app, phone, `/farm pause`, or a `Pause` file), "I need Claude",
  and deleting any task file.

## 5. Build order (updated for your decisions)

Each phase ends with something you can use. Phase 1 is sized for one weekend.

### Phase 0 — done
- Wi-Fi watchdog installed as a Scheduled Task (2026-10-18). Still to do by hand: Intel driver update and adapter
  settings (section 3b, Fix 3).

### Phase 1 — farmhands work overnight on their own (weekend)
- `tools/farm/manager.cjs`: read `tasks/`, pick the top `ready` tasks, make a worktree per task, run workers with
  `claude -p`, run the `cmd:` checks, run the checker, update the task files, write `state.json`.
- Parallel farmhands: up to 3 when you're not chatting, 1 while you are (decisions 1–2).
- Safety from day one, because tasks now start on their own: allowed/blocked tool lists + the PreToolUse hook that blocks
  send/post/push/pay/commit tools. Marketing tasks write drafts only.
- Usage: limit-message detection (sleep until reset), weekly reserve 30% (read `usage.json`), daily pacing, 5-hour cap
  while you're awake.
- "Internet down = wait, not fail" (read `wifi-state.json`).
- Stuck-run watchdog (10 min no output, or 5 repeats = stop).
- Morning summary as a file + `/farm summary` in the Agent Watch mod.
- Wallpaper: farm tasks as fields (reuse the existing farmhand drawing).

**Done looks like:** you leave 3 small real tasks before bed. In the morning: each has a verdict, passing ones wait in
their worktrees for you to commit, the summary tells you all of it in one screen, the farm respected the reserve and
paused when you opened a chat, and nothing was committed, pushed or sent.

**First real run (2026-10-18, 22:15):** T-0001 (add a "Running the tests" section to README.md) **passed** in about
40 seconds and waits in `ProoflineFarm\fields\T-0001` for you to review and commit. The inspector added a follow-up,
T-0002, which started on its own and correctly ended as **needs-you** after 2 tries: CMake isn't installed on this PC,
and the farm may not install programs. The usage mod reported the real numbers (5-hour 8%, weekly 17%). Fixed after
the run: the guard misread Git-Bash paths (`/c/...`) as outside the worktree; the farmhand and inspector are now told
the farm is Windows-only; farm log times are local. Still open: Agent Watch wasn't running (its status file was
7 h old), so neither the wallpaper nor "are you chatting" detection could see anything.

**Farm game + crew (2026-10-18, after Nirav shared an "AI agent ecosystem" Short):** the app is now a full-screen
pixel farm tycoon (`app/game.js`). Six named agents (Fern foreman, Bolt and Rivet coders, Poppy marketer, Sage
researcher, Hawk inspector) are clickable characters. You can chat with each one: a working agent gets the message
live through stream-json input (tested); an idle one answers in a read-only chat run and can plant tasks or save
memories. Self-prompting: Fern's planning cycle and each worker's own cycle, with goals.md, lessons.md, per-agent
memory, duplicate and low-value filters and daily caps (`lib/planner.cjs`, `lib/agents.cjs`). Also a Mission Control
roster with cycle timers, a Command Center (Mission / Tasks / Timeline / Agents), an Attention box and an event
ticker. Tests: `test/crew.test.cjs` (9), `test/farm.test.cjs` (11), `test/guard.test.cjs`.

**Desktop app (pulled forward from phase 3, 2026-10-18):** `tools/farm/app/` is an Electron app with a desktop and
Start-menu icon. It runs and keeps restarting the manager, lives in the tray when the window is closed, and has a
"Start with Windows" switch. Tabs: Farm (farmhands, usage bars with the reserve and pacing lines, waiting-for-you
cards), Tasks board, Plant a task, Summary, Settings. Buttons: stop one farmhand, try again, make ready / hold,
mark done, open the task's folder. The farm canvas from the wallpaper can replace the simple field view once that
branch is merged.

**Status before the real run: built and tested with a fake Claude.**
- `tools/farm/manager.cjs` plus `lib/` (task files, farm runs, guard, prompts), `usage-mod/` (reports the real 5-hour
  and weekly "% used" from inside every farm run), `start-farm.cmd`, and `templates/task-example.md`. How to use it:
  `tools/farm/README.md`.
- Tests: `test/farm.test.cjs` (11 scenarios: pass, failed checks, inspector fail, usage limit, Wi-Fi drop, no login,
  weekly reserve, daily pacing, stuck farmhand, no Done-when, your commit marking a task done) and
  `test/guard.test.cjs` (31 allow/block cases). All pass.
- The real `claude` command accepts all the farm's flags and loads the usage mod. **Blocker:** the `claude` command
  isn't logged in on this PC (the desktop app has its own login). Run `claude`, then `/login`, once.
- Not checked yet: farm runs showing up on the wallpaper through Agent Watch, and the first real reading in `usage.json`.
- Note: worktrees start from `main`, so the farm only sees work that is committed to `main`.

### Phase 2 — phone view + mailbox (moved up, decision 5)
- Cloudflare Worker + D1 + Pages on the free plan, behind Cloudflare Access (your Google login only). The manager
  pushes status out and pulls commands back. Nothing on your PC is opened to the internet.
- Phone page made for a small screen first: the farm canvas (tap a farmhand for its log + Stop / Redirect), the
  mailbox (approve / edit / reject drafts), the seed bag (add a task), the summary card, the budget bar and the Pause lever.
- Mailbox cards for drafts, "ready to commit" harvests and "needs you" questions.

**Done looks like:** from your phone, away from home, you read the morning summary, add a task, watch it get worked,
stop one farmhand, and approve a draft. The Cloudflare dashboard shows $0.

### Phase 3 — the full app on the PC + planner
- The same page in "app mode" on the PC (big farm canvas, click a farmhand, seed-bag board, budget barn).
- `/farm` commands in the mod: add, list, stop, redirect, approve, pause, reserve.
- Planner with `goals.md`; checker `next_tasks` go straight to `ready` (decision 4), always with done-criteria.
- The learning reserve (`usage-history.json`, takes over after 2 weeks), the "I need Claude" button, and the mod
  writing the real % numbers to `usage.json`.

**Done looks like:** for a week you only read summaries and approve drafts, the planner keeps the seed bag full with
tasks that match `goals.md`, and your own chats never run out because of the farm.

### Phase 4 — the second laptop
- Move the manager to the other laptop when you bring it. Your PC then only shows the wallpaper.
- Install the Wi-Fi watchdog there too (same installer, check the adapter name).
- One daily cloud routine as a backup heartbeat for repo-only jobs.

**Done looks like:** with your PC switched off for a day, the farm keeps going and the phone view still updates.

### Phase 5 — polish
- Per-kind trust settings, stats over time, nicer art (inspector, mailbox flag, night barn with the reset clock).

## Things to check before building
- Anthropic's current terms and plan limits for headless (`claude -p`) and scheduled use on your plan.
- Whether the Agent Watch mod can read the plan's "% used" numbers directly (otherwise use the hourly check).
- Current Cloudflare free-plan limits for Workers, D1 and KV (numbers above are from the free plan as I know it).
- Current model prices if any part uses an API key.
