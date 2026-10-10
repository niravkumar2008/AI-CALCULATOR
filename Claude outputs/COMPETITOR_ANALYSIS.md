# Competitor analysis: AI calculators (checked 2026-10-03)

For Nirav. Plain English. Every fact has a source link at the end of its row or line, as [Sx] (list at the bottom, all opened or searched on 2026-10-03). Anything marked **(estimate)** is my own guess, not a published number.

---

## 0. The short version

- **VoVo Corp = "VovoCorp"** (vovocorp.com), a small Spanish company. It sells a **scientific calculator with ChatGPT built in**, hand-assembled in Spain, in a normal-looking Casio-style case [S1][S2]. The domain was registered on **2026-05-19**, so it's about 5 months old [S5]. Its site says 63 reviews averaging 4.8★, but those are on its own site, not an independent one [S4].
- **Two models:** a base model with no camera at **$139.90** and a **Pro model with a 2K autofocus camera at $239.90**. The Pro model is **still a pre-order** [S2][S3].
- **Where they're ahead of us today:**
  1. A **free AI tier "forever"** (82 uses a day), with Premium at only $5.90–7/month [S2][S3].
  2. A **2.5" colour screen**.
  3. **They're already shipping** (base model) to 100+ countries, with reviews, Klarna and a reseller program.
  4. **A choice of AI model** (ChatGPT, Claude, Gemini, DeepSeek) plus **uploaded notes, memory and a companion web app**.
  5. A **cheaper no-camera model**.
- **Where we're ahead or can be:**
  - We designed our own PCB, with an e-paper screen (readable in sunlight, months of standby) and magnetic charging.
  - We use the US-standard fx-115ES key layout.
  - We're Claude-native.
  - We can be the **first camera model that actually ships** in the US.
  - We can be the **honest "study tutor"** while they market for exams (see section 1.4).
- **The biggest single gap is price, not hardware.** Our planned **$15/month** is 2–2.5× VovoCorp Premium. It's also above Photomath, Mathway and Gauth ($9.99–11.99/month) [S14][S15], and VovoCorp gives a free tier forever.

---

## 1. VovoCorp in detail

### 1.1 Product facts

| Item | VovoCorp (what they publish) | Our calculator (v13 board) | Source |
|---|---|---|---|
| Models / price | Base (no camera) **$139.90** (list $179.90); **Pro with camera $239.90, pre-order** | One model, **$225** | [S2][S3] |
| Subscription | **Free: "82 uses a day", forever** (a photo counts as 2–3 uses). Premium is shown as **$5.90/mo or $59.90/yr** on the product page, **$7.00/mo** on the home page and **$69–70/yr** as an add-on. First month of Premium free. | **$15/month**, first month free | [S2][S3][S6] |
| What Premium adds | Pick ChatGPT / Claude / Gemini / DeepSeek, "10x more" use, more reasoning, memory of past questions, PDF/TXT notes it answers from | Claude only (planned) | [S2][S6] |
| Screen | **2.5" colour** ("much more clarity and contrast") | 2.13" black/white e-paper, 250×122. Full refresh 2 s, partial 0.3 s | [S2], [S18] |
| Camera | **2K autofocus** (Pro model only) | OV5640 5 MP autofocus | [S2] |
| Connectivity | **Wi-Fi + Bluetooth** (phone hotspot, a phone link), airplane mode | Wi-Fi (ESP32-S3; Bluetooth LE possible in the chip, not used yet) | [S2][S6] |
| Battery | "15–20 h continuous use", charges in 30–60 min. A customer reports "6–7 days" per charge and another a 20–30 min charge. **Spare battery sold for $19.90**, so it's probably removable. | 150 mAh LiPo (Adafruit #1317; was 100 mAh until 10/6). AI-solve battery life not measured yet (24 h soak planned for 10/18) | [S2][S4] |
| Offline calculator | Acts as a "classic 82ms calculator" (looks like a Casio **fx-82MS**-class shell) "using a hardware hacking system" | Our own calculator engine in an fx-115ES shell (more functions than the fx-82MS class) | [S2] |
| Typing questions | Yes, by typing on the keys (purple A–F alpha letters, "Ans" = space, "EXP" = "=") or by photo | Photo only today **(check firmware plan)** | [S6][S7] |
| Notes / files | Upload text (any length), images and videos; notes readable offline | None yet | [S2] |
| Companion app | **PWA at app.vovocorp.com**: syncs history, chat and stats; upload your syllabus; battery notifications; free web simulator to try it | None yet (the proxy server is planned 10/4) | [S8] |
| "Privacy" features | Secret PIN, an **"emergency button" (ON) that snaps back to the normal calculator**, and a $29.90 privacy screen | No LED (exam mode) | [S2] |
| Languages | Spanish, English, Italian (German mentioned in reviews) | English | [S2][S4] |
| Distribution | Free shipping, 100+ countries, Klarna / Apple Pay, reseller program ("start reselling and earn money"), VovoCare 1-year cover $11.90 | Not selling yet | [S2][S3][S7] |
| Marketing | TikTok @vovocorp (hashtags #examenes #estudiar), Instagram, YouTube (about 150 subscribers) | Not started | [S5][S7][S9] |
| Teardown / processor | **Not published.** I found no teardown, chip, battery mAh or certification (CE/FCC) info. | ESP32-S3-MINI-1-N4R2 | — |

### 1.2 What customers say (their own review page, 63 reviews, 4.8★) [S4]
- **Liked:** looks exactly like a normal calculator; a colour screen; memory for long problems; fast charging; quick WhatsApp/chat support; free updates.
- **Complaints:**
  - **Keys don't register if pressed lightly** (two reviewers).
  - Shipping delays.
  - A battery arrived unplugged.
  - It came in a plain calculator box.
- These are self-hosted reviews (Shopify), so treat them as marketing, not independent proof.

### 1.3 Areas where VovoCorp beats us (ranked by how much it matters)

| # | Area | Why they win | How big a problem it is for us |
|---|---|---|---|
| 1 | **Price / subscription** | A free tier forever plus $5.90–7/mo Premium vs our $15/mo. The base model is $139.90. [S2][S3] | **High.** Buyers will compare monthly fees first. |
| 2 | **Shipping today + social proof** | They take orders in 100+ countries, show reviews and run a reseller program [S2][S4] | High. But their camera model is a pre-order too, so the camera race is still open. |
| 3 | **Software around the device** | Multiple AI models, memory, notes upload, a PWA with history sync, a free web simulator [S2][S8] | High. It's all software, so we can copy it fast. |
| 4 | **Display** | A 2.5" colour screen shows more text and photo previews, and redraws instantly [S2] | Medium. Our e-paper is clearer in sunlight and lasts much longer in standby, but it's smaller and redraws slowly (0.3–2 s) [S18]. |
| 5 | **Two SKUs, accessories, languages** | A no-camera model, a spare battery, a care plan, 3+ languages [S2] | Medium. Easy to copy: our camera is a plug-in module, so a "Lite" model just leaves it out. |
| 6 | **Typing questions + Bluetooth phone link** | They ask by keys or photo, and connect through the phone's hotspot or Bluetooth [S2][S6] | Medium. This is a firmware feature for us (the ESP32-S3 already has BLE). |

### 1.4 Where VovoCorp is weak (our openings)
- **Exam marketing.** VovoCorp markets for exams: #examenes, an "emergency button", a "privacy screen", notes "during exams" in reviews, and "100% accuracy" claims [S2][S4][S7].
  - The SAT bans any calculator with Wi-Fi, Bluetooth or a built-in camera [S16]. So their positioning invites school bans and bad press.
  - **Our opening:** be the honest **"AI study tutor that looks like a calculator"**, as the roadmap already says. Don't copy the stealth features.
- **A new, unknown company:** 5-month-old domain, email-only contact, no press I could find [S5][S10].
- **Keypad feel complaints** [S4]. We use Casio's own rubber keymat on our PCB pads, so ours should feel like a real Casio.
- **No published certifications or specs** (battery size, FCC/CE, chip). We can publish ours.

---

## 2. Other competitors

| Competitor | What it is | Price | Where it beats us | Where we beat it | Source |
|---|---|---|---|---|---|
| **7-CAL** (Hong Kong, Kickstarter) | AI calculator with a 2K AF camera, "240+ functions", **Wi-Fi or Wi-Fi + 4G**, LaTeX display, chat room, file upload, custom API key | **$229.99**, out of stock until Oct 5. Credits: $20 per 30 days (500 credits) or $5 per 100. 30 days free. | **4G option** (no Wi-Fi needed), LaTeX display, multi-model, bring-your-own API key, has already shipped. Kickstarter: 28 backers, HK$35,791 (Sept 2025). | Battery: needs AAA rechargeables or a built-in option. Probably a 2-line screen (see the reseller row). Stealth/"privacy mode" marketing. | [S11][S12][S19] |
| **"SUPER AI Calculator + Camera"** (Invento Electrónico, Seville, Spain) | Looks like a resold 7-CAL (same "240 functions", 350 KB files, credit prices) **(estimate)** | **€329** | Wi-Fi, 2K AF camera. Battery 16.5 h normal, **5 h on Wi-Fi**. | **A 2-line display**, so our e-paper shows far more | [S13] |
| **ChromaLock TI-84 mod / "secret.solver"** | DIY or one-off TI-84 mods with ChatGPT | n/a | Viral YouTube/TikTok attention | Not a real product, cheating-oriented | [S20] |
| **Photomath** (Google) | Phone app, photo → steps | Free; Plus about **$9.99/mo or $69.99/yr** | Free basic steps, best phone cameras, huge user base | Needs a phone. **Phones are now restricted in schools in 35 states + DC** [S21] | [S14] |
| **Gauth** (ByteDance) | Phone app, photo → AI answer, live tutors | Plus **$11.99/mo or $99.99/yr** | Live human tutors, fast, free tier | Same phone problem | [S14] |
| **Mathway** (Chegg) | Phone/web solver | **$9.99/mo or $39.99/yr** | Cheap yearly plan, offline solving | Same phone problem | [S15] |
| **Socratic / Google Lens** | Socratic merged into Google Lens (2025) | Free | Free, everywhere | Phone needed, no calculator | [S17] |
| **Casio fx-991CW ClassWiz** | Plain scientific calculator, QR code → graph on ClassPad.net | **about $23** | Price, exam-legal, solar | No AI, no explanations | [S22] |
| **NumWorks** | Graphing calculator, Python, official exam mode (France) | **about $125–150** | Exam-legal, open firmware, colour screen | No AI | [S23] |
| **TI-84 Plus CE Python** | US classroom standard | **about $110–160** | Teacher adoption, SAT/ACT-legal | No AI | [S24] |

**Takeaway:**
- **Phone apps** win on price and camera quality; we win where phones aren't allowed or wanted.
- **Normal calculators** win on exams and price; we win on explanations.
- **VovoCorp and 7-CAL** are the real head-to-head rivals, and both are about $230–240 with cheap or free AI.

---

## 3. How we become better

Ratings: **Impact** (H/M/L) = how much it changes a buyer's choice. **Effort/Risk** (L/M/H) = work before the order plus the chance of breaking the board.

### 3(a) PCB / hardware: what's possible before Monday's order

**Limits I checked in the repo:**
- **Height:** 5.7 mm above the board (stage13_heights.md).
- **Board:** 2 layers, 0.8 mm.
- **No free GPIOs.**
  - pins_final.h uses every usable pin. Only the strap pins IO3/45/46 and the UART test pads IO43/44 are left.
  - The TCA8418's 18 lines are all used by the 8×10 key matrix.
  - So **anything that needs a new GPIO (mic, buzzer, flash LED) is effectively v2.**
  - Anything that sits on the **existing I2C bus** (fuel gauge, secure element) needs no new pins.
- **No 32 kHz RTC crystal is possible:** its pins are GPIO15/16, already used by camera PCLK and D5 [S25].
- **The module is already the best fit:** the ESP32-S3-MINI-1 comes only as N8 (no PSRAM) or N4R2. The camera needs PSRAM, so keep N4R2 [S26].

| # | Change | Impact | Effort / risk | Parts (LCSC) | Recommendation |
|---|---|---|---|---|---|
| H1 | **Add bulk capacitance on +3V3/SYS** (2 × 22 µF 0805 next to U1, plus one on SYS). Fixes review item S4: Wi-Fi TX peaks of 350–500 mA are 3.5–5 C on the 100 mAh cell assumed at the time (2.3–3.3 C on the 150 mAh #1317 chosen 10/6) and can brown out mid-solve. | **H** (reliability: a solve that reboots looks broken) | **L** effort / **L** risk, only if space next to U1 exists. Re-run DRC. | Samsung 22 µF 0805, **C45783 (Basic part, no extra fee)** | **THIS BOARD** |
| H2 | **Bigger battery with no board change.** The bay is about 31 × 11.5 mm with about 9.4 mm depth (stage 13), and J4 is a standard JST-PH. Candidates: LP501230, **140 mAh**, 12.5 × 31 × 5.3 mm; 601230 class, **about 180 mAh** [S27]. That's +40–80 % runtime. Keep the 50 mA charge current (R2) at first; it's safe for any of them. | **H** (battery is VovoCorp's headline claim: 15–20 h) | **L**. Measure the bay after grinding the solar box; check polarity against J4 (review S5). Must be under the ground solar box: +2.6 mm top for a 6 mm cell **(estimate)**. | Off-board (Adafruit/EEMB/Amazon) | **THIS BOARD** (no PCB change; buy 1–2 to test) |
| H3 | **Add test pads**: USB D+/D−, GND, VBUS (TP1/5/6/7 + UART already exist) so a pogo-pin jig can flash and test units. | M (manufacturing speed, the roadmap's "makes itself" plan) | **L**. Pads on the bottom layer if there's room near the key pads; otherwise v2. | none | **THIS BOARD if space; else v2** |
| H4 | **Unpopulated (DNP) footprint for a fuel gauge on I2C** (MAX17048, battery % like a phone). Placing an empty footprint costs nothing at assembly. | M | M (new footprint, routing near I2C, DRC). The LCSC number must be checked before adding. | MAX17048 (verify LCSC #) | **v2** (the firmware ADC curve gives ±10 % **(estimate)**; not worth the risk 2 days before the order) |
| H5 | Secure element (ATECC608) for subscription anti-cloning | L–M | M | ATECC608B | **v2 / probably never.** ESP32-S3 already has secure boot, flash encryption and the HMAC / digital-signature peripheral. Do it in firmware. |
| H6 | Microphone for voice questions | M | **H** (no free GPIOs, I2S needs 2–3) | e.g. MSM261 PDM mic | **v2** (needs a GPIO plan, e.g. free IO43/44 from UART) |
| H7 | Buzzer / speaker | L | M (no GPIO) | — | **v2**, low value (a calculator should be quiet in class) |
| H8 | Camera flash / white LED | M (photos in dim classrooms) | M (no GPIO; conflicts with the "no LED" rule) | — | **v2**. Software fix first: auto exposure + higher-gain capture. |
| H9 | Colour LCD (2.4–2.8" SPI IPS) instead of e-paper | M–H (matches VovoCorp) | **H** (new FPC, backlight power, window size, battery drain) | — | **v2** (or the custom-shell generation). Keep e-paper as a selling point: sunlight, standby. |
| H10 | 4G/LTE modem (like 7-CAL) | M | **H** (size, power, SIM, certification) | — | **v3 / never for this shell** |
| H11 | IMU | L | M | — | Skip |
| H12 | RTC crystal | L | Not possible (pins taken) | — | Skip. Sync time over Wi-Fi (NTP). |
| — | **Magnet connector option A/B (stage 13)** | Blocking | — | — | **Decide first.** Nothing above matters if the order slips. |

**My advice for Monday:** do **H1** (bulk caps), buy **H2** cells to test, and do **H3** only if it's a 10-minute job. Leave everything else for v2. The board is DRC-clean, and the magnet decision is the real deadline.

### 3(b) Firmware, companion app and proxy (no board change, highest return)

| # | Feature | Beats | Impact | Effort | When |
|---|---|---|---|---|---|
| F1 | **Free daily tier on the proxy** (e.g. 15–25 solves/day on Haiku-first routing; Haiku ≈ $0.0065/solve per the roadmap §3.3 **(estimate)**) | VovoCorp's free 82/day | **H** | L (proxy rate limit) | Launch |
| F2 | **"Verified answer" check**: our on-device calculator engine re-computes the final numbers Claude gives and shows ✓ or "check this step". No competitor claims this. | Everyone on accuracy | **H** | M | Launch |
| F3 | **Tutor mode**: hint → next step → full answer, with a "check my work" photo mode | Positions us as learning, not cheating | **H** | M (prompting + UI) | Launch |
| F4 | **Companion web app (PWA)** on the proxy: Wi-Fi setup, solve history, the full worked solution with LaTeX on the phone/laptop, a parent view | VovoCorp app | **H** | M | Launch → month 1 |
| F5 | **Type questions with the keys** (alpha letters on the fx-115ES keys) | VovoCorp, 7-CAL | M | M | Month 1 |
| F6 | **Latency**: crop + JPEG quality ~60, stream the answer, show the first step with a 0.3 s partial refresh while the rest arrives, limit TX power (review S4) | Feel of speed | M–H | M | Launch |
| F7 | **Notes / follow-up memory** (upload a syllabus on the PWA; the proxy adds it to the prompt; prompt caching keeps cost low) | VovoCorp Premium | M | M | Premium tier |
| F8 | **Wi-Fi via phone hotspot + BLE provisioning** | VovoCorp | M | M | Month 1 |
| F9 | **OTA updates** | Everyone | H (fixes after shipping) | M | Launch |
| F10 | **Classroom lock**: teacher QR / PIN turns AI and radios off until unlocked, with a visible "AI OFF" icon. It's still not SAT-legal (a camera is present), but helps with teachers. | VovoCorp's bad reputation risk | M | M | Month 2 |
| F11 | **Security**: per-device token, secure boot + flash encryption, device certificate via the S3 digital-signature peripheral, kill switch | Clones / abuse | H (needed before selling) | M | Launch |
| F12 | Languages (Spanish first) | VovoCorp | L–M | L | Later |

### 3(c) Business and marketing

| # | Move | Why | Impact | Effort |
|---|---|---|---|---|
| B1 | **Re-price the subscription**: $225 with a **free daily tier forever** + **Plus at $4.99/mo or $39/yr**. This matches the roadmap's own $3–5 analysis (§3.3). $15/mo is above every competitor [S2][S14][S15]. | Price is our biggest gap | **H** | L |
| B2 | **"Lite" model without the camera** (about $149 **(estimate)**): the same board with no camera module plugged in, so typed AI questions only | Matches VovoCorp's $139.90 base model | H | L |
| B3 | **Honest positioning**: "AI study tutor, not for exams". Target parents and **phone-free schools** (35 states + DC restrict phones [S21]) | Turns VovoCorp's exam-cheat image into our advantage | **H** | L |
| B4 | **Publish specs + certifications** (FCC SDoC, UN38.3, battery mAh) | VovoCorp publishes none | M | M (already in the roadmap) |
| B5 | **Teacher / tutor pilots** (Purdue, local high schools, tutoring centres) with a teacher dashboard | Education partners | M–H | M |
| B6 | **Short videos** showing photo → steps → ✓ verified (TikTok/YouTube/Instagram), plus a free **web simulator** (we already have simulator.html) | VovoCorp and 7-CAL live on TikTok | H | L–M |
| B7 | **Custom shell before scale** (no Casio trade dress, per the roadmap) | Both rivals look like modified Casios too, which is a legal risk | H (legal) | H |
| B8 | Accessories: a spare battery, a 1-year care plan, US-only fast shipping, Affirm/Klarna | VovoCorp extras | L–M | L |

---

## 4. Recommended top 5 (impact ÷ effort)

1. **Re-price** to a free daily tier + $4.99/mo Plus (B1 + F1): **business, launch**.
2. **Bulk caps on +3V3/SYS** (H1): **this board**.
3. **Bigger LiPo in the same bay** (H2, 140–180 mAh, no PCB change): **this board** (buy and test cells).
4. **"Verified answer" + tutor mode** (F2 + F3): **firmware, launch**. No rival has this.
5. **Companion PWA + typed questions + OTA** (F4, F5, F9): **software, launch → month 1**.
- **v2 hardware:** colour LCD, mic, flash LED, fuel gauge, a GPIO plan (e.g. free the UART pins), pogo test pads, a custom shell.

---

## 5. Questions for Nirav

1. Where did you see VovoCorp being "ahead"? A video, a friend's unit? If you have a link, send it so I can check their exact claims (the TikTok and YouTube pages wouldn't load text for me).
2. Is **$15/month** fixed? Every rival is at $0–7 (VovoCorp) or $9.99–11.99 (apps).
3. Is there room next to U1 for 2 × 0805 caps? (I can check in KiCad if you want.)
4. Do you want a "Lite" no-camera model on the price list?

---

## Sources (all accessed 2026-10-03)

- [S1] VovoCorp home page: https://vovocorp.com/en-us
- [S2] VovoCorp product page (specs, prices, Premium, accessories, shipping): https://vovocorp.com/en-us/products/calculadora-con-ia
- [S3] VovoCorp product list (prices, $7/mo Premium): https://vovocorp.com/en-us/collections/all
- [S4] VovoCorp reviews page (63 reviews, 4.8★, quotes): https://app.vovocorp.com/opiniones/ (also https://vovocorp.com/en-us/pages/opiniones)
- [S5] Gridinsoft domain check (registered 2026-05-19, Shopify, YouTube about 150 subscribers): https://gridinsoft.com/online-virus-scanner/url/vovocorp-com
- [S6] Search-result summary of VovoCorp FAQ (82 uses/day, photo = 2–3 uses, hotspot/Bluetooth): via https://vovocorp.com/en-us/products/calculadora-con-ia and search
- [S7] VovoCorp TikTok videos (captions only): https://www.tiktok.com/@vovocorp/video/7643195765598293270 , https://www.tiktok.com/@vovocorp/video/7644965408449105174 , https://www.tiktok.com/@vovocorp/video/7644842803360451862
- [S8] VovoCorp companion app (PWA): https://app.vovocorp.com/
- [S9] VovoCorp YouTube "How to use VovoCorp Calculator" (page text not readable): https://www.youtube.com/watch?v=SdSwyZnkidI
- [S10] VovoCorp contact page (email only): https://vovocorp.com/en-us/policies/contact-information
- [S11] 7-CAL product page ($229.99, credits, specs): https://www.7-cal.com/product-page/ai-calculator-with-camera
- [S12] 7-CAL on Gadgetify (2025-09-02): https://www.gadgetify.com/7-cal-ai-calculator/
- [S13] Invento Electrónico "SUPER AI Calculator + Camera" (€329): https://inventoelectronico.com/en/calculators/187-AI-calculator.html
- [S14] Photomath Plus and Gauth Plus prices (aggregator search results): https://subger.com/en/service/mathway , https://opentherank.com/education-pricing/gauth/
- [S15] Mathway price ($9.99/mo, $39.99/yr): https://subger.com/en/service/mathway
- [S16] College Board SAT calculator policy: https://satsuite.collegeboard.org/sat/what-to-bring-do/calculator-policy
- [S17] Socratic (Google), Wikipedia: https://en.wikipedia.org/wiki/Socratic_(Google)
- [S18] Waveshare 2.13" e-paper refresh times (2 s full / 0.3 s partial): https://www.waveshare.com/wiki/Pico-ePaper-2.13
- [S19] 7-CAL Kickstarter (28 backers, HK$35,791; page blocked, search summary): https://www.kickstarter.com/projects/7-cal/7-cal-the-worlds-first-ai-calculator-with-built-in-camera
- [S20] ChromaLock TI-84 ChatGPT mod (search summary); secret.solver TikTok: https://www.tiktok.com/@secret.solver/video/7560007796389891358
- [S21] Ballotpedia, state school phone restrictions (35 states + DC as of 2026-03-24): https://news.ballotpedia.org/2026/03/26/kansas-becomes-thirty-third-state-to-enact-a-k-12-cellphone-ban/
- [S22] Casio fx-991CW: https://www.casio.com/us/scientific-calculators/product.FX-991CW/ ; price https://underwooddistributing.com/products/casio-classwiz-cw-fx-991cw-scientific-calculator
- [S23] NumWorks price: https://underwooddistributing.com/products/numworks-graphing-calculator
- [S24] TI-84 Plus CE Python price: https://jamestownbookstore.gtcc.edu/product/ti-84-plus-ce-python-graphing-calculator-texas-instruments
- [S25] ESP32-S3 32 kHz crystal on GPIO15 (XTAL_32K_P) / GPIO16: https://esp32.com/viewtopic.php?p=155736 , https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32s3/api-reference/peripherals/clk_tree.html
- [S26] ESP32-S3-MINI-1 variants (N8, N4R2 only): https://lcsc.com/product-detail/image/ESP32-S3-MINI-1-N4R2_C3013941.html , https://www.espressif.com/sites/default/files/documentation/esp32-s3-mini-1_mini-1u_datasheet_cn.pdf
- [S27] EEMB LiPo cell sizes (LP501230 140 mAh 12.5 × 31 × 5.3; LP401230 100 mAh): https://www.eemb.com/product-122 , https://www.eemb.com/product-121
- Repo facts: `hardware/HANDOFF.md`, `hardware/stage13_heights.md`, `hardware/pins_final.h`, `hardware/REVIEW_independent_2026-10-03.md` (S4), `Claude outputs/LAUNCH_ROADMAP.md` §3.3 and §4.
