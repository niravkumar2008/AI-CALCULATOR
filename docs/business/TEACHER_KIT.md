# Proofline teacher kit

For Nirav. Written 2026-10-11. **Draft: nothing has been sent or published.** Facts only from
`web/site-src/PAGE_BRIEF.md` and the real pages `/teachers/` and `/features/exam-mode/`.

Part A is the one-pager to print or turn into a PDF. Part B is notes for you. Part C is the
outreach message.

**When to use it:** only once a sample does exam mode on camera (`MARKETING_GUIDE.md` 2.7: no cold
outreach before then). Never sell to schools before the product ships, never say schools
"approve" it, never contact students through teachers.

---

## Part A. The one-pager (paste-ready)

> # Proofline exam mode: a guide for teachers
>
> **Proofline** is a scientific calculator with an AI tutor and a camera. This sheet shows how to
> tell when the AI is off, and gives a classroom policy you can adapt. It's your classroom: use it
> as is, change it, or set your own rule.
>
> ## Look for the black bar
>
> In exam mode, the **status bar across the top of the screen turns black** and shows a
> **padlock**, the word **EXAM** and a **timer** that counts up from when exam mode started.
>
> **No black bar = not in exam mode.** The calculator has no LED, so nothing glows or blinks either
> way. The screen is the only signal.
>
> [Illustration of the black status bar: padlock · EXAM · 0:42. Label it "Illustration. The final
> screen may look a little different." until a real photo replaces it.]
>
> ## Checking it from across the room (30 seconds)
>
> 1. Before the test, ask students to turn it on: **SHIFT MODE 8 =**.
> 2. Ask them to hold the calculators up, screen facing you.
> 3. Look for the black bar on every screen.
> 4. During or after the test, glance at the **timer**. Students can't turn exam mode off from the
>    keys, so a timer that reads at least as long as the test has been running means exam mode has
>    been on the whole time.
>
> ## What exam mode turns off
>
> | Off | |
> |---|---|
> | **The AI** | AI Solve and Type it can't be used |
> | **The camera** | Off |
> | **Wi-Fi** | Off |
> | **Every app** | Including Notes and History (saved notes and past answers can't be opened). Turning exam mode on closes any open app. MODE 4 just shows "Apps are off". |
>
> ## What stays on (normal calculator features)
>
> The scientific calculator: fractions, trig, logs, memory and replay, plus **statistics and
> regression (STAT, MODE 2)**, **function tables (TABLE, MODE 3)**, **CALC**, **SOLVE**,
> **definite integrals (∫dx)** and **derivatives (d/dx)**. Exam mode doesn't switch these off. If a
> test shouldn't allow them, say so in your rules.
>
> ## How it ends
>
> - **By itself after 12 hours**, or
> - **a teacher unlocks it with a USB cable.**
>
> Students can't turn exam mode off from the keys.
>
> ## Big exams
>
> **Not allowed on the SAT, ACT or AP exams, even in exam mode**, because it has Wi-Fi and a
> camera. Students should bring a standard calculator to those. It isn't approved for any
> standardized test.
>
> ## Classroom policy you can adapt
>
> > AI calculators are allowed for class work and homework.
> > For tests, exam mode must be on and visible on the screen before the test starts, or use a
> > school calculator.
> > They aren't allowed on the SAT, ACT or AP exams.
>
> Stricter option:
>
> > For tests, only calculators without Wi-Fi or a camera are allowed. Leave AI calculators in your
> > bag.
>
> Option that also limits calculator features:
>
> > For this test, exam mode must be on, and STAT, TABLE, SOLVE and the calculus keys may not be
> > used.
>
> ## Questions teachers ask
>
> **Can a student turn exam mode off during a test?**
> Not from the keys. It ends after 12 hours or when a teacher unlocks it with a USB cable.
>
> **Can they open notes or saved answers in exam mode?**
> No. Every app is off, including Notes and History.
>
> **Can it take photos in exam mode?**
> No. The camera is off.
>
> **Does it connect to anything in exam mode?**
> No. Wi-Fi is off.
>
> **Does the AI do the work for students?**
> Outside exam mode, AI Solve shows the answer and every step, so students can follow and check
> the working. It's meant for homework and study. If the handwriting is unclear, it says so
> instead of guessing. AI can make mistakes; that's why it shows every step.
>
> **Is it allowed on the SAT, ACT or AP?**
> No, not even in exam mode.
>
> **What happens to students' photos?**
> Photos are used only to solve that problem and are not stored. Only usage counts are kept.
>
> **Is it a graphing calculator?**
> No. It's a scientific calculator. No matrices, vectors, complex numbers, base-n or equation mode.
>
> **Does it work on school Wi-Fi?**
> School Wi-Fi with a login page isn't supported. It works with 2.4 GHz home Wi-Fi or a phone
> hotspot. The calculator itself works fully offline.
>
> **Is it available now?**
> Not yet. Preorders are planned for 2027.
>
> **Can I see a demo?**
> Yes. Email **hello@proofline.example** with "Teacher demo" in the subject. [Placeholder; replace
> with the real address.]
>
> More at **proofline.example/teachers**.
>
> *Proofline is not affiliated with Casio Computer Co., Ltd.*

---

## Part B. Notes for you

### B1. Printable layout

- **One US Letter sheet, single-sided**, 0.6 in margins. Must print well in black and white on a
  school copier (no meaning carried by color; the "black bar" picture is literally black).
- **Order:** title and one-line intro → "Look for the black bar" with the illustration (largest
  element, top third) → "Checking it from across the room" → two side-by-side boxes "Off" / "Stays
  on" → "How it ends" + "Big exams" in a bordered box → policy wording (in a box marked "Copy
  this") → footer: demo email, URL, not-affiliated line.
- The FAQ goes on the **back** or on the web page only; the front must fit on one side.
- Fonts: 11 to 12 pt body, 20 pt headings, the key sequence **SHIFT MODE 8 =** in keycap boxes
  (match `.keycap` on the site).
- Accessibility: real text (not an image) in the PDF, tagged headings, alt text on the
  illustration, contrast 4.5:1 or better.
- `/teachers/` already prints "just this summary on one sheet": export from there for a matching
  version.
- Replace the illustration with a real photo of a sample in exam mode as soon as one exists
  (logo covered). Until then keep the "Illustration" label.
- File name: `Proofline-exam-mode-for-teachers.pdf`. Date it in the footer ("Updated [month
  year]") so outdated copies are obvious.

### B2. Open questions before this goes out

- **USB unlock, step by step:** what does a teacher need (just a cable? a computer? a tool or a
  page?). PAGE_BRIEF only says "a teacher unlocks it with a USB cable". Don't print more than that
  until the firmware decides.
- **Can anyone with a USB cable unlock it?** Teachers will ask. Decide and document the answer.
- **A real photo** of the black bar from 5 to 8 meters away.
- **Demo method:** a video call with a sample, the `/try-it/` simulator, or (from spring 2027) a
  free sample loan to a few adult teachers for honest feedback, disclosed as a free loan, quoted
  only with written permission (`MARKETING_GUIDE.md` 2.7).

### B3. Rules for teacher work

- No claims that schools or districts approve it. No "allowed in class".
- Don't ask teachers to recommend it to students, collect student data, or pass anything to
  students.
- A teacher who posts about it for money is a creator: 18+ (of course), paid, disclosed (#ad),
  per `CREATOR_KIT.md`.
- Never quote a teacher without written permission.

---

## Part C. Outreach message for teacher communities

Use only where the community's rules allow posts from makers (many forums and groups ban
promotion; read the rules, ask a moderator if unsure). Always say you're the maker. One post, then
answer questions. No DMs to people who didn't ask. No links in places that ban them.

### C1. Forum or group post

> **Title:** I make an AI calculator. Here's how its exam mode works; I'd value teachers' honest
> feedback
>
> Hi all. Disclosure first: I'm the founder of Proofline, a scientific calculator with an AI tutor
> and a camera. It isn't for sale yet (preorders are planned for 2027), and I'm not asking anyone
> to buy anything.
>
> I know a calculator with AI and a camera worries teachers, so I designed the exam mode for you:
>
> - Students press SHIFT MODE 8 =. The AI, camera, Wi-Fi and every app turn off (notes and saved
>   answers too).
> - The status bar turns black with a padlock, "EXAM" and a timer counting up, so you can check it
>   from the front of the room.
> - Students can't turn it off from the keys. It ends after 12 hours, or a teacher unlocks it with a
>   USB cable.
> - The normal calculator features stay on: STAT, TABLE, SOLVE, integrals and derivatives.
> - It is not allowed on the SAT, ACT or AP, even in exam mode, and we say so everywhere.
>
> Outside exam mode, the AI shows every step rather than just an answer, and says so when it can't
> read the handwriting.
>
> My questions for you:
> 1. Would a visible exam mode like this be enough for you to allow it in class tests? What would
>    you need to see?
> 2. Is the timer useful, or would you want something else on screen?
> 3. What should the unlock process look like for a teacher?
>
> There's a one-page summary at proofline.example/teachers [add "and a demo video" only once it
> exists] if it helps. Happy to
> answer anything, including the hard questions.

### C2. Email to a teacher who asked for information

> **Subject:** Proofline exam mode: the one-page summary you asked for
>
> Hi [name],
>
> Thanks for asking. The one-page summary is attached (and at proofline.example/teachers).
>
> The short version: SHIFT MODE 8 = turns the AI, camera, Wi-Fi and every app off; the status bar
> turns black with a padlock, "EXAM" and a timer; students can't turn it off from the keys (12
> hours, or a teacher unlocks it with a USB cable). It isn't allowed on the SAT, ACT or AP.
>
> If a short video call with a working sample would help, reply with a couple of times that suit
> you.
>
> [Name], Proofline
> [postal address] · Reply "stop" and I won't email again.
