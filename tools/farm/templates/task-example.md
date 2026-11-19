---
id: T-0000
title: Example - copy this file to make a task (files starting with _ are ignored)
kind: code              # code | marketing | research | hardware-check
repo: AI-CALCULATOR     # must be listed in farm.config.json
priority: 2             # 1 = first
status: idea            # idea | ready | working | checking | harvest | failed | needs-you | done
tries: 0                # the farm stops after 2 failed tries and asks you
budget: small           # small = up to 25 min, medium = up to 70 min; bigger tasks must be split
needs_approval: false
blocked_by: []          # e.g. [T-0003]: waits until T-0003 is done (committed by you)
---
## What to do
Say exactly what you want, in plain words. Name files or folders if you know them.
Example: In calculator-core, add a unit test that presses x^-1 on 4 and expects 0.25.

## Done when
- cmd: npm test --prefix calculator-core
- judge: the new test would fail if the x^-1 key were broken

Rules for "Done when":
- "cmd:" lines are commands the farm runs itself in the task's folder. They must finish with exit code 0.
- "judge:" lines are checked by the inspector (a second AI that didn't do the work).
- A task with no "Done when" lines is never started; it stays an idea.
- Marketing tasks only make drafts (in .farm/drafts/). Nothing is posted or sent.
