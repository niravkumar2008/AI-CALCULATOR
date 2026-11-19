# Proofline Farm tools

Plan: [docs/business/proofline-farm-app-plan.md](../../docs/business/proofline-farm-app-plan.md).

## The farm game (open "Proofline Farm" on the desktop)

A full-screen pixel farm where every AI agent is a character you can click and talk to. It's inspired by the
"AI agent ecosystem" Short: a Mission Control roster with each agent's next self-prompting cycle, a Command Center,
an Attention box and a live event ticker.

**The crew** (`ProoflineFarm\agents.json`; rename or recolour them there):
| Agent | Job | Lives in |
|---|---|---|
| **Fern** | Foreman: plans the farm's work from your goals | Farmhouse |
| **Bolt** | Coder | Workshop |
| **Rivet** | Coder (firmware & hardware) | Workshop |
| **Poppy** | Marketer (drafts only, never posts) | Market |
| **Sage** | Researcher | Library |
| **Hawk** | Inspector: checks every finished job | Watchtower |

**Talking to an agent:** click the character (or their row in Mission Control) and type in the chat box.
- If they're **working**, they read your message at their next step and adjust (for example "use the same style as
  the row 1 tests").
- If they're **idle**, they answer, can read the code to answer questions, and can **plant tasks** from what you ask
  ("Poppy, draft 3 posts for the waitlist"). They also remember things you tell them.
- Fern can also add to the goals. Her drawer has the **goals editor** and **Plan now**.

**Self-prompting** (the farm keeps finding its own work):
- **Fern's cycle** (about every 90 min, or sooner when the seed bag runs low): she reads `goals.md`, looks at the real
  code, the open and finished tasks, past failures (`lessons.md`) and recent events. Then she plants up to 5
  small, checkable tasks for the right teammates.
- **Each worker's own cycle** (about every 2 h when idle with nothing ready in their line): they propose 1–2 tasks
  in their specialty, using their own memory (`agents\<name>\memory.md`).
- **Quality filters:** every planted task needs "Done when" checks, repeats of existing tasks are skipped, low-value
  ideas are skipped, and there are at most 20 new tasks a day. Failures become lessons, so the same impossible job
  isn't tried again. After each job the farmhand writes "Lessons" into its memory, so agents get better over time.
- Everything stays inside your usage rules (30% weekly reserve, daily pacing, 5-hour cap while you're awake).

**Buildings:** seed shed = task board, mailbox = what waits for you, silo = usage (its grain level is your weekly use),
farmhouse = Fern and the goals. Each building opens a cut-away room view. **Command Center** (top right) has the
Mission, Tasks, Timeline and Agents tabs.

When orders start coming in, a new agent (for example an orders clerk in a post office) can be added to
`agents.json` with its own kind of tasks.

## The farm manager (phase 1)

You leave tasks in a folder, and the farm works on them by itself, also overnight. For each task it:
1. makes a separate copy of the code (a git *worktree*, in `ProoflineFarm\fields\T-0001`), so nothing touches your
   main folder;
2. sends a **farmhand** (a headless Claude run, Sonnet) to do the task;
3. runs the task's `cmd:` checks itself (no AI involved);
4. sends an **inspector** (a second, fresh Claude run, Haiku) that didn't see the work session to judge it against
   the task's "Done when" list;
5. writes the verdict into the task file: **harvest** (passed, ready for you to review and commit), back to **ready**
   with fix notes (one more try), or **needs-you**.

It never commits, pushes, sends, posts or buys anything. A safety guard (`lib/guard.cjs`) blocks those actions
inside every farm run, and each one it blocks is written to `logs\guard.log`. Marketing tasks only write drafts (in the
task's `.farm\drafts\`).

### Rules it follows (your decisions, 2026-10-18)
- **Weekly reserve:** the farm stops for the week when your weekly limit reaches 70%; the last 30% is yours. It also
  spreads its share over the days left until Wednesday's reset, so it doesn't use it all on one night.
- **5-hour limit:** between 7:00 and 23:00 it keeps the 5-hour limit under 60%, so you can always start a chat.
- **Your chats:** while you're chatting (any Claude chat active in the last 10 min), 1 farmhand. Otherwise up to 3.
- **Usage limit hit:** the task goes back to `ready` without using a try, and the farm sleeps until the reset time.
- **Wi-Fi down** (from the watchdog's `wifi-state.json`): it waits. A cut-off run goes back to `ready` without using a try.
- **Stuck farmhand** (no output for 10 min, or the same action 5 times): it is stopped, and that counts as a failed try.
  After 2 failed tries a task waits for you.
- **New tasks:** the inspector may add up to 3 follow-ups per task, which start on their own (decision 4). They're
  limited to 10 a day and 2 levels deep. Follow-ups without "Done when" lines stay ideas.
- **When you commit** a harvest in its worktree, the farm notices and marks the task `done`.

The real "% used" numbers come from a tiny mod (`usage-mod/`) that every farm run loads. It writes
`ProoflineFarm\usage.json` whenever the numbers move. Before the first run there is no reading, so the farm starts with
1 farmhand to get one.

### First-time setup (about 5 minutes)
1. **Sign Claude in for the command line** (done 2026-10-18). The desktop app has its own login, and typing `/login` inside
   the app doesn't sign in the `claude` command. If farm runs ever say "not logged in", open the Claude app's terminal
   panel in `tools\farm` and run `powershell -ExecutionPolicy Bypass -File .\sign-in-claude.ps1`. In a normal Windows
   terminal, `claude auth login` does the same. Farm runs use this login, i.e. your normal plan usage, no extra bill.
2. **Open the Proofline Farm app:** double-click **Proofline Farm** on your desktop (or in the Start menu). The icons
   were made 2026-10-18; `app\make-shortcut.ps1` makes them again. The app:
   - runs the farm manager for you and restarts it if it stops by accident;
   - **keeps farming when you close the window**: it lives in the tray (the sprout icon by the clock). Right-click the
     icon to pause, open, or **Quit**. Quit stops the farm;
   - **Settings → Start with Windows** makes it open hidden at every login, so the farm is always on. Tick it once;
   - has tabs: **Farm** (farmhands at work, usage, what's waiting for you), **Tasks** (the board), **Plant a task**
     (add one without editing files), **Summary**, **Settings** (reserve, number of farmhands, folders).
   Other ways to run it, if you ever need them:
   - `install-farm-autostart.ps1`: a Windows Scheduled Task that starts the manager hidden at login, without the app.
   - `start-farm.cmd`: runs the manager in a window, only while the window is open.
   - Only one farm manager runs at a time. If one is already running, a second one says so and exits.
   - The farm works while Windows is logged in and awake. It stops when the PC sleeps, shuts down, or you log out.
     For overnight work, set sleep to "Never" when plugged in.
   - It runs from this worktree's folder. If this worktree is ever deleted, run the installer again from wherever
     `tools\farm` lives then (for example after you commit and merge it).
3. **Add a task:** in the app, **Plant a task**. Or copy `ProoflineFarm\tasks\_EXAMPLE.md` to e.g. `T-0003.md`,
   change the `id`, write "What to do" and "Done when", and set `status: ready`. The farm picks it up within 15 seconds.

**Important:** each task's worktree starts from the `main` branch, so the farm only sees work that is committed to
`main`. Work that's still uncommitted in other worktrees (the website, the farm wallpaper, these farm tools) is
invisible to it until you commit and merge it.

A good first task to try it (small, safe, easy to check):

```markdown
---
id: T-0001
title: Add a "Running the tests" section to README.md
kind: code
repo: AI-CALCULATOR
priority: 1
status: ready
tries: 0
budget: small
needs_approval: false
blocked_by: []
---
## What to do
In the main README.md, add a short section "Running the tests" that explains, in plain words for a beginner,
how to build and run the tests in tests/ (look at CMakeLists.txt to see how). Don't change anything else.

## Done when
- cmd: node -e "process.exit(require('fs').readFileSync('README.md','utf8').includes('Running the tests')?0:1)"
- judge: the steps match CMakeLists.txt, are easy for a beginner, and nothing else in README.md changed
```

### Everyday use
| You want to… | Do this |
|---|---|
| See what it's doing | `node manager.cjs status` (in `tools\farm`), or look at the window |
| Read the summary | `ProoflineFarm\summary.md` (written every morning at 7:00), or `node manager.cjs summary` for right now |
| Review a harvest | Open `ProoflineFarm\fields\T-0001` (VS Code / GitHub Desktop). Look at the changes, and commit them if you like them. The farm then marks the task done. If you don't like them, set the task back to `ready` and write what to change under "What to do". |
| Is it running? | `node manager.cjs status`: the first line says "Manager: running" or "not running" |
| Pause the farm | Create an empty file named `PAUSE` in `ProoflineFarm` (delete it to continue). It finishes the current step, then waits. |
| Turn always-on off | `powershell -ExecutionPolicy Bypass -File install-farm-autostart.ps1 -Uninstall`, or disable "Proofline Farm" in Task Scheduler |
| Stop one task | Set its `status:` to `idea` in the task file |
| Change the rules | Edit `ProoflineFarm\farm.config.json` (e.g. `"weeklyReserve": 40`, `"maxFarmhands": 2`) and restart the farm |

Each run's full record (what the farmhand did, check output, the inspector's verdict, the farmhand's report) is in
`ProoflineFarm\runs\T-0001\`.

### Tests
`node test/farm.test.cjs` runs the whole loop against a throwaway repo with a fake Claude (no usage). It covers
passing, failing checks, inspector fail, usage limit, internet drop, no login, the weekly reserve, daily pacing,
a stuck farmhand, tasks without "Done when", and your commit marking a task done.
`node test/guard.test.cjs` checks the safety guard.

### Not in phase 1 yet
- Showing farm tasks on the wallpaper. Farm runs are normal Claude sessions, so Agent Watch should show them as fields
  automatically, but this hasn't been checked with a real run yet. The farm's own feed (`ProoflineFarm\farm.js`) is
  ready for the wallpaper to use later.
- `/farm` commands, the mailbox, the phone view (phase 2), and the learning reserve (it already records
  `usage-history.jsonl` every hour, so the learning has data when it's built).

## Wi-Fi watchdog

Fixes the "Wi-Fi dies at night, turning it off and on fixes it" problem without you being awake.
Every 2 minutes it checks for real internet. After 3 failed checks (about 6 min) it restarts the Wi-Fi adapter, the
same as switching it off and on. If 3 restarts don't help, it waits 15 min and tries again. Everything goes into a log.

- `wifi-watchdog.ps1` – the check (Task Scheduler runs it every 2 minutes).
- `install-wifi-watchdog.ps1` – sets up the Scheduled Task for you (run once, as administrator).

### Install (easiest way, about 1 minute)

1. Press **Start**, type `PowerShell`, right-click **Windows PowerShell** → **Run as administrator** → Yes.
2. Paste this and press Enter:

   ```powershell
   powershell -ExecutionPolicy Bypass -File "C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\.claude\worktrees\musing-mahavira-a1d3c4\tools\farm\install-wifi-watchdog.ps1"
   ```

   It should print `Installed 'Proofline Wi-Fi Watchdog' (adapter 'Wi-Fi').` It copies the script to
   `C:\Users\r_kas\ProoflineFarm\bin`, so it keeps working even after this worktree is gone.
3. Check it: open **Task Scheduler** (Start → type "Task Scheduler"), click **Task Scheduler Library**, find
   **Proofline Wi-Fi Watchdog**. "Next Run Time" should be within 2 minutes, and after it runs, "Last Run Result"
   should say `The operation completed successfully. (0x0)`.

To remove it: same admin PowerShell, the same command with ` -Uninstall` on the end.

### Or set it up by hand in Task Scheduler

1. Copy `wifi-watchdog.ps1` to `C:\Users\r_kas\ProoflineFarm\bin\` (make the folders).
2. Task Scheduler → **Create Task…** (not "Basic Task").
3. **General** tab: Name `Proofline Wi-Fi Watchdog`. Click **Change User or Group…**, type `SYSTEM`, OK.
   Tick **Run with highest privileges**.
4. **Triggers** tab → New: **On a schedule**, **One time**, start now. Tick **Repeat task every: 5 minutes** and then
   type `2 minutes` in the box, **for a duration of: Indefinitely**. OK. Add a second trigger: **At startup**.
5. **Actions** tab → New: Program `powershell.exe`, arguments:
   `-NoProfile -NonInteractive -ExecutionPolicy Bypass -WindowStyle Hidden -File "C:\Users\r_kas\ProoflineFarm\bin\wifi-watchdog.ps1"`
6. **Conditions** tab: untick **Start the task only if the computer is on AC power**.
7. **Settings** tab: tick **Run task as soon as possible after a scheduled start is missed**. Set "Stop the task if it
   runs longer than" to **1 hour** (the shortest option). For "If the task is already running", pick **Do not start a new instance**. OK.

### Reading what happened

- Log: `C:\Users\r_kas\ProoflineFarm\logs\wifi.log`, e.g.
  ```
  2026-10-19 02:14:03  no internet (check 1 of 3)
  2026-10-19 02:18:05  restarting 'Wi-Fi' (restart 1 of 3, down 4 min)
  2026-10-19 02:18:41  restart fixed it, was down 5 min
  ```
  Nothing in the log means the internet never dropped.
- State for the farm manager: `C:\Users\r_kas\ProoflineFarm\wifi-state.json` (`"online": true/false`).

### Test it safely (doesn't touch the adapter)

```powershell
powershell -ExecutionPolicy Bypass -File wifi-watchdog.ps1 -DryRun -SimulateOffline -FarmDir "$env:TEMP\farmtest"
```

Run it 3+ times, then look at `$env:TEMP\farmtest\logs\wifi.log`. Lines marked `[dry run]` show what it would have done.

### Things to know

- The PC must stay **awake** overnight. Settings → System → Power → Screen and sleep → "When plugged in, put my device
  to sleep after" = **Never**. A sleeping PC runs no tasks and no Claude.
- If you switch the Wi-Fi off yourself, the watchdog leaves it off.
- If the adapter has a different name (e.g. after a driver update), the installer lists the adapters. Run it again
  with `-Adapter "<name>"`.
- The watchdog only fixes the symptom. To get fewer drops, see section 3b, "Fix 3", in the plan: driver update,
  adapter power settings, or a cable.
