# Knockoff shell plan: measuring a 991ES-style clone and refitting the board (v15)

Written 2026-10-06. Companion to `QA_TEST_PLAN.md`, `COST_MODEL.xlsx` and `UNIT_ECONOMICS.md` in this folder.

**Where this sits in the plan.** The first 2 prototypes go in ground Casio fx-115ES shells (board v14, `../FINAL_STATUS.md`). The next 10–100 units go in cheap Casio-style clone shells from Alibaba (the "991ES PLUS" family: Runzon RZ-991ES PLUS, "OS-991ES Plus", generic "991ES PLUS 417 functions"), which needs a **v15 board** laid out for the clone's posts, ribs and key mat. The long-term answer is our own 3D-printed / moulded drop-in shell, which removes the trade-dress problem entirely (`../enclosure/replica/` is the unbranded CAD starting point).

**Why a clone and not more Casios:** a donor fx-115ES costs about $21 with tax and arrives with a Casio logo we must not sell; a clone is $3.25–3.99 at MOQ 20 on Alibaba (Accio market summary, 2026-10-05; `Claude outputs/Alibaba_Bulk_Sourcing.xlsx` S1) and can be ordered without a logo. The clone is **not** the same mould as the Casio, so nothing from the v14 hole map can be assumed. Everything below exists to find out what changed.

---

## 1. Ordering samples

| What | How many | From whom | Why |
|---|---|---|---|
| Clone calculators, complete (shell, key mat, window, screws, slide cover, their PCB and LCD inside) | **4 total: 2 each from 2 different sellers** | Seller A: the Alibaba "991ES PLUS" listing at $3.25–3.99 MOQ 20 (ask for a 2-piece sample order; most sellers do it for ~$5–8 each plus DHL). Seller B: a Runzon RZ-991ES PLUS or generic clone from AliExpress / Amazon (MOQ 1, $5–8 incl. shipping) | Two sellers tell you whether "991ES PLUS" means one mould or several. Two pieces per seller tell you the batch-to-batch spread. |
| Budget | about **$40–60** incl. DHL for the Alibaba pair | | In `COST_MODEL.xlsx`, 'Cash budget' sheet, row 2 ("Knockoff shell samples", $60). |

**What to ask the seller before paying (paste into Alibaba chat; the full template is on the "Supplier message templates" sheet of `Alibaba_Bulk_Sourcing.xlsx`):**
1. Photos of the **inside with the circuit board removed**, and of the **rubber key mat, contact side up**, with a ruler in the shot.
2. The **model number and mould number**, and written confirmation that future orders use the **same mould**.
3. Outer size and the screw positions (a photo of the back with a ruler is enough).
4. Price for 20 / 100 / 1,000 pcs, **plain version with no brand name or logo printed** (front plate, back, slide cover and box).
5. Whether they can ship **shell + key mat + window + screws + slide cover without the PCB, LCD and battery** (cheaper, lighter, no battery paperwork), and the price for that.
6. Which **key legends** are printed (we need the fx-115ES/991ES layout: SHIFT, ALPHA, MODE, ON; CALC and ∫dx on row 2; the 5 × 4 number block). The legends are moulded or printed on the keys and we cannot change them cheaply.
7. Whether a plain-window version (no "NATURAL-V.P.A.M." or "991ES PLUS" text on the faceplate) is available.

Keep all chat on-platform, pay by Trade Assurance, and never more than a 30 % deposit on a bulk order (`Alibaba_Bulk_Sourcing.xlsx`, "Checks before bulk", row 9).

---

## 2. Caliper list for the clone sample

Same style and IDs as `../measurements_2026-10-03.md` so the two shells can be compared line by line. Use the prefix **K** (knockoff). Measure **both samples from seller A** and **both from seller B**; write all four readings. Tools: 0.01 mm calipers, a 0.5 mm pencil, the phone camera, a ruler for the photos, and a printed `../fitcheck/grind_map_front_shell.svg` at 100 % for the dry fit (K-F1).

Front view = looking at the keys, top (screen) end up. "Inside" readings are taken with the back cover off and the clone's PCB lifted out (keep its screws).

### Case, with calipers
| ID | Item | Reading (A1 / A2 / B1 / B2) | fx-115ES reference | Notes |
|---|---|---|---|---|
| K-C1 | Front shell outer length | | 159.9 | |
| K-C2 | Front shell outer width, screen section | | 79.3 | |
| K-C2b | Outer width, keypad section | | ≈ 66.5 (G4) | Clones often skip the waist. If the keypad section is as wide as the top, the v15 board can be wider there. |
| K-C3 | Back cover outer length × width | | 161 × 79.3 | |
| K-C4 | Wall thickness, screen section (L / R) | | 5.5 incl. wire-holder stubs; 4.7 without | Measure with and without any stubs. Decides the board width in the screen section (v14: 64.75 there). |
| K-C5 | Wall thickness, keypad section | | 0.8 | |
| K-C6 | Clone's own PCB width × length | | 65.05 × 97.9 (Casio) | Their board proves what fits. Photograph both sides of it with a ruler. |
| K-C6b | Clone PCB thickness | | 0.8 (Casio) | If the clone uses 1.0 or 1.2 mm, the key stack (D9/K-D9) changes. |
| K-C7 | Post diameters: screw posts / locating pegs | | 3.85 / 2.9 | Count them, too. Casio: 6 screw posts + locating pegs in pairs. |
| K-C8 | Post-pair spacing, top to bottom (left-right distance between the two posts of each row) | | 46.5 (function section) / 42.3 (number section) | Write "one post only, left/right" where a row has one. |
| K-C9 | Top-corner screw posts, centre to centre | | 68.1 | |
| K-C10 | Bottom screw posts, centre to centre | | 39–40 (board notches 38.94) | |
| K-C11 | Top-left corner post: centre to inside of left wall / top wall | | 4.2 / 6.1 | |
| K-C12 | Window opening (LCD lens) | | 60.65 × 24.3 | Must be ≥ the e-paper active area 48.55 × 23.7 with the panel centred; the panel outline is 59.2 × 29.2. |
| K-C13 | Window top edge to case top edge | | 24.0 | Sets where J2 and the e-paper slot go. |
| K-C14 | Side-wall stubs beside the screen: how far in / how high | | 5.45 / 5 | |
| K-C15 | Each post row, from the case top edge to the post centre (say if you read to the hole's far edge) | | 8.9 / 69.95 / 89.6 / 120.45 / 130.2 / 142.4 / 154.6 | Photograph the open front shell with the ruler along one edge so the rows can be cross-checked. |
| K-C16 | Screw size and thread (take one out) | | Casio: small self-tappers, reused | If different, buy 10 % spare screws with the shells. |

### Key mat and contacts
| ID | Item | Reading | fx-115ES reference | Notes |
|---|---|---|---|---|
| K-M1 | Number block: column pitch × row pitch (centre of the 7 key to the centre of the 8 key, etc.) | | 12.85 × 11.13 | Measure across 4 columns and divide by 4 for accuracy. |
| K-M2 | Function block: column pitch × row pitch | | 10.66 × 9.5 | Across 5 columns / 2 rows and divide. |
| K-M3 | Row spacing R1→R2 and R2→R3 (SHIFT row to the next rows) | | 11.7 / 8.9 | |
| K-M4 | Contact pill size: number keys / function keys / SHIFT row / arrow pad | | pads 9.0 × 7.0 / 7.0 × 5.5 / 6.0 × 4.5 / 5.0 × 4.0 | Pill = the black carbon dot on the mat's underside. v15 pads must be bigger than the pill by ≥ 0.5 mm all round. |
| K-M5 | Count of contacts | | 50 (46 keys + 4 arrow directions) | If the clone has a different key count (e.g. no CALC / ∫dx), the key map in firmware changes. |
| K-M6 | Arrow pad: UP/DOWN and LEFT/RIGHT contact spacing | | ±5.3 / ±6.6 from centre | Casio's UP/DOWN were 1.2 / 1.0 mm further out than the fx-300ES; expect the same kind of surprise. |
| K-M7 | Key mat overall outline and its locating holes (which posts pass through it) | | | Photograph contact side up with a ruler: this photo plus `tools/blobs.py` gives every pad position for v15 (same method as stage 10). |
| K-M8 | Mat material feel: silicone thickness at the skirt, carbon pill hardness | | | Thin clone mats bottom out and double-press. Note it; it affects the pad-to-mat distance. |

### Depths and heights
| ID | Item | Reading | fx-115ES reference | Notes |
|---|---|---|---|---|
| K-D1 | Board height: from the front shell's rim (parting line) down to the top of the clone PCB (component side) | | 5.5 | Our board sits where theirs did. |
| K-D3 | Space above the board at the top edge (for the magnet connector) | | ≈ 5 | Needs 3.5 with the notch, 6 without. |
| K-D4 | Space behind the board at the camera spot (board to the back cover's inside face) | | ≈ 6.0 | Design limit = reading − 0.3. The camera is 5.4 and J4 5.5 tall, so **≥ 5.7 needed under them** after any grind. |
| K-D5 | Back cover thickness at the camera spot | | ≈ 1.0 | |
| K-D6 | Solar box height on the back cover (and whether there is one: some clones have a fake solar cell with no box) | | frame 5–6 | A clone with no solar box saves the longest grind (3–4 min with jig A). Ask the seller for the no-solar version if it exists. |
| K-D7 | Coin-cell holder rib height (and the cell type: LR44 vs AAA) | | 5–6 | An AAA-style clone has a long battery tube: a big obstacle **or** a ready-made LiPo pocket. Measure the tube's inside: 1200 mAh #258 is 62 × 34 × 5.0. |
| K-D8 | Clone LCD module thickness | | 3.75 | |
| K-D9 | Key mat: top of the button rubber to the black contact | | 1.5 | |
| K-D10 | Closed calculator thickness: keypad section / screen section | | 11.3 / 11.7–11.8 | |
| K-D13 | Back-cover rib and ring heights (rib grid / small ring / big ring / lower ribs) | | 1 / 4 / 4 / 1 | Sketch the rib map on a photo of the back cover's inside. |

### Camera, magnet and battery spots
| ID | Item | Reading | fx-115ES reference | Notes |
|---|---|---|---|---|
| K-S1 | Camera hole spot: from the case top edge and from the left edge to the point where a Ø7 hole can go through a **flat, rib-free** patch of the back cover (≥ 14 mm clear along the rib direction) | | front X 0, Y 43.84 (KiCad 150, 95.1); rib B ground 14 mm | The camera must look out the back through 1.0 mm of plastic. Mark the spot in pencil on the sample. |
| K-S2 | What is on the outside of the back cover at K-S1 (label recess, texture, moulded text) | | smooth | A moulded "991ES" or logo there is a trade-dress problem **and** a drilling problem. |
| K-S3 | Magnet notch spot: top wall, from the right outer edge to the centre of a 21.5-wide, 7.95-deep U-notch that clears the corner screw post and any snap teeth | | centre 22.5 from the right outer edge (front X 22.5) | The magnet face is 21 × 7 oval, 5.5 deep with the legs. Check nothing inside the top wall (LR44 holder, solar frame) is within 1 mm of the notch. |
| K-S4 | Back-cover lip behind K-S3: height and whether it exists | | lip exists on some fx-115ES | Jig B's "5b" snip. |
| K-S5 | Battery area: the largest flat rectangle on the back-cover floor in the top-left corner (under where the LR44/solar used to be), L × W, and the free height to the faceplate features above it | | 33.5 × 20.7 × 3.8 (holder kept) | The #1317 is 26.0 × 19.75 × 3.8. If the clone has ≥ 5.7 free, a 502030 (250 mAh) fits with no trim. |
| K-S6 | Solar area: the solar window in the faceplate (size, and whether it is a real cell or a printed fake) | | real cell, 34.6 × 13.6 box | A printed fake means no cutout: the magnet connector can go anywhere along the top wall. |
| K-S7 | Lead channel: a ≥ 1.5 mm gap from K-S5 to where J4 will be | | reserved | |

### Dry fit
| ID | Item | Result | Notes |
|---|---|---|---|
| K-F1 | Paper dry fit of the **v14** outline (`../fitcheck/grind_map_front_shell.svg`, 100 %, 50 mm line checked) in the clone's front shell | post-by-post: inside hole / pushes / misses by x mm | This is the go/no-go input (section 4). Expect misses; write down every post's offset in mm and direction. |
| K-F2 | Lay the v14 key-pad overlay (same print) on the clone mat, contact side up | each pill on its pad / off by x mm | A pill more than 1 mm off its pad means that pad moves in v15. |
| K-F3 | Flatbed scan or ruler-photo of the open front shell, the mat (contact side) and the back cover's inside | files in `C:\Users\r_kas\.claude\uploads\` | Feed to the KiCad session for v15 (`tools/blobs.py` + homography, as in stage 10). |

---

## 3. What changes on the board for v15

Treat v15 as "v14 with the mechanical layer re-derived". **Do not touch the schematic**, the camera GPIO map, J1's reversed footprint, the J3 pin order or the CPL rotation tool (`../HANDOFF.md`, must-not-undo list).

| Area | Likely change | Driven by | Risk |
|---|---|---|---|
| Board outline | Width in the screen section (K-C4), keypad section (K-C2b, K-C5), length (K-C1, K-C13), waist shape | K-C1–C5, K-C6 | Low. The clone's own PCB outline is the safe envelope; copy it minus 0.25 mm like G2 did for Casio. |
| Post holes | All of H1–H14 move or vanish; slotted holes again where the two samples disagree by > 0.3 mm | K-C7, C8, C15, K-F1 | Medium. Holes sit next to key pads and the I2C lines (stage 10 lesson: moving H1 cuts I2C_SCL). Budget one full re-route pass. |
| Screw notches (bottom) and top corner cut-outs | New centre distance (K-C10, K-C9) | | Low |
| Key pads (50) | Re-positioned from the K-M7 photo; sizes from K-M4; arrow pad from K-M6 | K-M1–M7 | Medium. Pads are the B-side copper the key matrix rows/cols feed; the TCA8418 matrix wiring stays, only the pad positions move. |
| E-paper window / slot | J2 and the 1.0 × 14 mm ribbon slot follow the window (K-C12, K-C13) **only if** the window moves by more than the 14.3 mm ribbon budget allows | K-C12, C13 | Medium. Rule from stage 14: don't move J2 unless the ribbon budget breaks. |
| Camera spot | J1 stays; the camera module is taped where K-S1 says, within the 70 mm ribbon | K-S1 | Low |
| Magnet connector J3 | Stays at the top edge; its x position may shift with K-S3 | K-S3, K-S6 | Low (right-angle SMD header, no routing underneath) |
| Battery J4 and R2 | J4 stays near the battery area; **R2 → 10 kΩ (100 mA)** if the clone takes a 250–300 mAh cell, **4.7 kΩ (213 mA)** only for the 1200–1500 mAh custom-shell cell (`../enclosure/final_assembly/battery_upgrade.md` §5) | K-S5 | Low. One BOM line. |
| Test pads | Add the pogo-jig pads planned for v2 (3.3 V, GND, EN, IO0, USB D+/D−, VBAT) while the layout is open | roadmap §5 | Low, but only if there is B-side room away from the key pads |
| Silk | "AI CALC v15 <date>" plus **our own logo**, no "Casio", no "fx", no "ES" | section 5 | Nil |
| Height stack | Re-check `fitcheck/clearance_table.md` against K-D4 and the clone rib map (K-D13); regenerate the grind map | K-D1, D4, D13 | Medium. A thinner clone (K-D10 < 11.3) can make the camera (5.4) not fit at all: then the camera goes to a thinner module or the shell is the custom one. |

Rough effort: one KiCad session for the outline, holes and pads from the K-F3 photos, one for re-routing and DRC to 0/0/0, one for an independent re-check like `verification/05_stage14_recheck.md`. JLCPCB cost for the 10-board v15 pilot is in `COST_MODEL.xlsx` ('Unit cost', 10-unit column): about $18 a board assembled plus about $8 a unit freight and duty, so about **$260 for 10 assembled boards**; the whole 10-unit pilot is about $770 of parts ('Cash budget' row 4).

---

## 4. Go / no-go rule for the clone

Run after the four samples are measured. **All of these must be true** for the clone to be the 10–100-unit shell:

1. **Same mould, twice.** Both pieces from the same seller agree on every K-C and K-M reading within **±0.2 mm**, and the two sellers either agree within ±0.3 mm (same mould) or you pick one seller and get the mould number in writing (K-section 1, question 2).
2. **Our parts fit in height.** K-D4 − 0.3 ≥ **5.7 mm** at the camera spot and at J4's spot after at most the same two grinds we do today (solar box, one rib). If the clone is thinner so that this needs a third grind zone or thinning the floor below 0.8 mm: **no-go**.
3. **The key mat is usable.** 50 contacts (K-M5), pills ≥ 2 mm from every post (so the v15 pads can keep ≥ 0.25 mm copper-to-hole), and no pill more than 1.5 mm from where a pad can physically go (K-F2). The SHIFT / ALPHA / MODE / ON row and the arrow pad must exist in the same order as the fx-115ES (the firmware key map assumes it).
4. **Window ≥ 50 × 24 mm** (K-C12) so the e-paper's 48.55 × 23.7 active area shows with margin, and the window-to-top distance (K-C13) keeps J2 inside the 14.3 mm ribbon budget, or the window is in the same place as the Casio's within 2 mm.
5. **A flat, rib-free Ø7 patch exists on the back for the camera** (K-S1, ≥ 14 mm along the rib) that is **not** under moulded text or a logo recess (K-S2).
6. **The magnet notch fits in the top wall** without hitting a post, the LR44/solar structure or a snap tooth (K-S3), and the back-cover lip can be snipped (K-S4).
7. **Battery floor ≥ 27 × 21 × 3.9 mm** free (K-S5) for the #1317, or any larger box.
8. **No Casio marks** anywhere on the plain version: no "CASIO", no "fx-", no "ES PLUS", no "NATURAL-V.P.A.M.", no Casio-style wordmark on the faceplate, back, slide cover or box. Printed key legends are fine (they are functional). If the seller cannot supply a plain version: **no-go for sales**, prototypes only.
9. **Price and MOQ** at 100 ≤ **$4.50** delivered per shell-with-mat (COST_MODEL assumption), MOQ ≤ 100, lead time ≤ 30 days.

Any single "no" on 2, 3, 5 or 8 is a hard no-go: skip the clone and go straight to the custom shell (the `../enclosure/replica/` CAD plus a silicone key mat; roadmap §3.2). A "no" on 1 means buy from a different seller and measure again. A "no" on 4, 6, 7 or 9 is a design or negotiation problem, not a stop.

**Decision record:** write the result in `hardware/production/knockoff_decision.md` (date, seller, mould number, the four K-F1 offset lists, GO/NO-GO and which rule failed).

---

## 5. Trade-dress caution

A clone shell copies the look of a Casio fx-991ES. Casio's wordmark is a registered trademark, and the overall look of its calculators (shape, two-tone silver/navy faceplate, key colours and layout, "NATURAL-V.P.A.M." badge) can be protected as **trade dress** (unregistered product design is protectable in the US if it has acquired distinctiveness; Casio has sold these for 15+ years). We are not lawyers; the point of this section is to keep us on the obviously safe side until a lawyer has looked.

Three tiers, in order of risk:

| Use | Risk | Rule |
|---|---|---|
| Our own 2 prototypes in ground Casio fx-115ES shells | Low (private use, not sold) | Fine. Never sell or give them away with the Casio logo visible. |
| Beta / pilot units in **plain** clone shells, given free or sold at cost to testers | Medium | Only with the do's below, every unit labelled "Not affiliated with Casio", and a lawyer or law-school clinic consult **before** the first unit leaves the house. |
| Public sales (Kickstarter, Shopify, Amazon) in clone shells | **High** | **Don't**, unless a lawyer says the specific shell is clear. Public sales wait for the custom shell or a supplier shell with its own design. |

**Do:**
- Order the **plain version**: no brand name, no model name, no slogan on any surface, including the slide cover and the box. Put it in the PI ("no printed brand or model text").
- Put **our own logo and product name** on the faceplate (a printed label under the window lens, or a pad-printed mark if the seller offers it) and on the back label, with **"Contains FCC ID: …"** and **"Not affiliated with Casio Computer Co., Ltd."** on the back label and in the quick-start card.
- Use our own **box, manual, product photos and listing text**. Never write "Casio", "fx-991", "fx-115", "ES PLUS" or "ClassWiz" anywhere in marketing, SEO tags or the Amazon title. Comparisons ("works like a normal scientific calculator") are fine; naming Casio as a lookalike is not.
- Change what you can cheaply change: a different faceplate colour from the seller's catalogue (many clones come in black/grey/blue), our own window lens printing, our own slide-cover colour. Each visible difference reduces the "confusingly similar" argument.
- Before scale (≥ 100 public units): a **trademark / trade-dress check by a lawyer** (SCORE or a university IP clinic is often free; a private consult $200–500), plus a USPTO search of our own name (roadmap §4: $350 per class, classes 9 and 42). Record the advice in `knockoff_decision.md`.
- Keep the custom shell on the roadmap as the real fix (`../enclosure/replica/` is already unbranded and matches the board).

**Don't:**
- Don't keep or copy any Casio wordmark, logo, model number, "NATURAL-V.P.A.M." / "ClassWiz" badges or the Casio-style font on the shell, the key caps or the box.
- Don't photograph a Casio or a Casio-branded unit for the listing, and don't use "Casio-compatible" or "Casio-style" as marketing words.
- Don't resell the clone's own branding either (a Runzon or OS logo is someone else's mark too).
- Don't sell modified **Casio** units at any volume: a materially altered branded product resold under the original brand is the classic trademark case (roadmap §4).
- Don't rely on "it's a clone, so Casio's problem is with the factory": the seller in the US (us) is who gets the letter.
- Don't let the key legends be the only thing that makes the unit a calculator: our own firmware and e-paper UI already do, but keep the "normal calculator mode" working without a subscription so the product is a calculator first (roadmap §0).

If a lawyer says the plain clone is still too close, the fallback is the **custom shell** path with the clone's **key mat only** (a rubber mat bought as a spare part is a functional component, not trade dress) sitting in our own moulded body. That keeps the $3 part that is hardest to make ourselves.

---

## 6. Order of work

1. Order the 4 samples (section 1) this week; they take 7–15 days.
2. Measure (section 2) the day they arrive; upload the K-F3 photos.
3. Apply the go/no-go (section 4); write `knockoff_decision.md`.
4. If GO: KiCad session for v15 (section 3), 10-board JLCPCB order (5 assembled), then the 10-unit pilot batch using `QA_TEST_PLAN.md`.
5. In parallel: lawyer / clinic consult (section 5), own-name trademark search.
