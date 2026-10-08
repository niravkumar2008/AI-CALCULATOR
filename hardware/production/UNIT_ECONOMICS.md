# Unit economics: what each calculator costs and earns

Written 2026-10-06 for Nirav. Every number here comes from `COST_MODEL.xlsx` in this folder. Change a blue cell there and the numbers update. Companion files: `KNOCKOFF_SHELL_PLAN.md` (the clone shell) and `QA_TEST_PLAN.md` (testing).

**The short answer:** at $225 the calculator makes money from 10 units on. The $15 subscription covers the Claude API for a typical student with about half left over. A heavy user at today's 500-solve cap costs more than $15 on Opus 5.5, though. The $2,000 covers the 2 prototypes, the samples and a 10-unit pilot, with about **$50 left**. It does not pay for the first 100 public units. Those need about $8,100, or about 37 pre-orders.

Most inputs are still estimates: the clone shell price, tariffs, how many buyers keep paying, and how much students actually use it. Treat the numbers as "about right", not exact.

---

## 1. What one calculator costs

| | 2 units | 10 units | 100 units | 1,000 units |
|---|---|---|---|---|
| Shell | Casio fx-115ES you own | clone, bought retail | clone from Alibaba | clone from a factory |
| Board | v14 | v15 | v15 | v15 |
| **Cost per unit** (parts, freight, scrap, samples, jig) | **$234** | **$92** | **$46** | **$34** |
| Same, plus selling-legally costs spread over the batch | $234 | $92 | **$81** | $37 |

How to read it:
- **2 units cost $234 each** because one-time costs fall on just 2 calculators: $112 of shipping and tariff, $69 of bench tools, and spare parts. The roadmap's "$138" left those out. The cash you actually spend for the 2 is **$426** (the shopping list). The Casios are already yours.
- **From 10 to 100 the cost halves.** The board drops from $18 to $9 (JLCPCB's fixed $67 per order gets shared), the camera from $12 to $6, and the battery, magnet connector and cable from Adafruit retail (~$17) to Alibaba (~$5).
- **At 100 the selling-legally costs add $35 a unit.** That is the ~$3,550 one-time bill for the FCC test, a lawyer, the LLC, a trademark and insurance (roadmap §4). At 1,000 units it shrinks to $3.50 a unit.
- **The biggest unknown is import duty.** The model assumes about 35 %, the low end of what JLCPCB quotes (35–92.5 %). At the high end, freight and duty roughly double, adding about $8 a unit at 100.
- **Your own time isn't counted** at 2–100 units: about 37 minutes per calculator (grinding with the jigs, assembly, QA). At $15 an hour that would be about $9 a unit.

## 2. What you keep on a $225 sale

| Per calculator sold | 10 units | 100 units | 1,000 units |
|---|---|---|---|
| Price | $225 | $225 | $225 |
| Card fee (2.9 % + 30¢) | −$6.83 | −$6.83 | −$6.83 |
| Shipping it to the customer | −$6.00 | −$6.00 | −$6.00 |
| Returns / warranty allowance (3 %) | −$6.75 | −$6.75 | −$6.75 |
| Claude API for the free first month | −$7.52 | −$7.52 | −$7.52 |
| Unit cost (incl. selling-legally at 100+) | −$91.56 | −$81.13 | −$37.22 |
| **Hardware profit** | **$106** (47 %) | **$117** (52 %) | **$161** (71 %) |

The 2 prototypes are not for sale, so they have no margin. If you did sell them, you'd lose about $36 each.

## 3. Does $15 a month cover the Claude API?

**For a typical user, yes.** A "typical" user is about 5 photo solves a day (150 a month, from the roadmap).

| Claude API cost per user per month | 2 a day (light) | 5 a day (typical) | 10 a day (heavy) | at the 500/month cap |
|---|---|---|---|---|
| **Opus 5.5** ($4 / $20 per M tokens) | $3.01 | **$7.52** | $15.03 | **$24.72** |
| Sonnet 5.5 ($2 / $10; the proxy's default today) | $1.51 | $3.77 | $7.54 | $12.40 |
| Haiku 4.5 ($1 / $5) | $0.36 | $0.90 | $1.79 | $2.95 |

One solve on Opus 5.5 costs about **4.9¢**. That's about 2,500 tokens in (the photo plus the instructions) and about 2,100 out (counting the thinking), with the fixed instructions cached.

What's left from the $15, per paying user per month, on Opus 5.5:

| | 10 units | 100 units | 1,000 units |
|---|---|---|---|
| $15 − card fee 74¢ − API $7.52 − server share | **$3.18** (21 %) | **$6.39** (43 %) | **$6.71** (45 %) |
| Same user at the 500-solve cap | −$14.03 | −$10.81 | −$10.49 |

The server costs about $25 a month however many people use it, so with only 7 paying users it takes $3.57 from each.

**What this means:**
1. **$15 pays for up to about 9.5 solves a day on Opus 5.5.** Above that, that user costs you money.
2. **The 500-solve cap is too high for Opus 5.5.** You can lower `MONTHLY_CAP` to about **280** (the break-even point), or keep the cap and run **Sonnet 5.5**. Sonnet is already the proxy's default model. It roughly halves the API cost ($3.77 typical), and even a user at the 500 cap ($12.40) stays below $15.
3. **The Batch API's 50 % discount doesn't help here.** Batch answers can take minutes to hours, and a student is waiting at the calculator.
4. **Over a year, one buyer's subscription is worth about $34 of profit at 100 units.** This assumes 70 % of buyers start paying after the free month and 8 % cancel each month. That's about 5.3 paid months out of 12. Both percentages are guesses until the beta gives real numbers. **Hardware + subscription ≈ $150 profit per calculator at 100 units, ≈ $196 at 1,000.**

## 4. Break-even and payback

- **The 10-unit pilot** carries $150 of one-time costs (samples). It covers them after **2 sales**.
- **The 100-unit batch** carries about $3,800 of one-time costs: the selling-legally bill plus samples and the test jig. It covers them after **25 of the 100** are sold. The other 75 are profit.
- **Getting the $2,000 back:** about **18–19 calculators** sold at $225 (13 at 1,000-unit costs).
- **The free first month** costs about $7.52 in API per buyer. A paying user earns that back in about **1.2 months** at 100 units (2.4 months at 10 units, because the server is shared by so few).
- **If you ever sold the calculator at cost,** the subscription alone would take about **13 months** to pay back the unit cost at 100 units, or 5.5 months at 1,000. Selling at $225 means you don't have to wait for that.

## 5. Where the $2,000 goes

| # | What | When | $ |
|---|---|---|---|
| 1 | Prototype build: JLCPCB v14 + Adafruit + Waveshare + Seeed + bench tools | after the paper dry fit | 426 |
| 2 | 4 knockoff shell samples from 2 sellers | this week | 60 |
| 3 | Samples of the other 100-batch parts (camera, e-paper, battery, magnet pair) | after the shells pass | 90 |
| 4 | 10-unit pilot on the v15 board (all parts, freight, scrap) | after GO and v15 | 766 |
| 5 | Proxy server, 3 months | beta | 75 |
| 6 | Claude API for 12 users' free month (on Opus 5.5) | beta | 90 |
| 7 | Law clinic / lawyer check on the clone shell (trade dress) | before any unit leaves | 150 |
| 8 | Indiana LLC | before taking money | 97 |
| 9 | Domain and email | beta | 20 |
| | 10 % contingency | | 177 |
| | **Total** | | **1,951** |
| | **Left from $2,000** | | **≈ 49** |

**Not covered by the $2,000:** the first 100 public units (≈ $4,570 of parts) plus the selling-legally bill (≈ $3,550): **≈ $8,100**. At $225 that's **about 37 pre-orders** (Crowd Supply or Kickstarter, roadmap §4). Take the pre-orders **before** you pay for the 100, so the buyers' money funds the batch rather than yours. If the lawyer says pilot units may be sold (`KNOCKOFF_SHELL_PLAN.md` §5), each one sold at $225 instead of given away adds about $106 to the cushion.

The budget is tight. If the clone fails the go/no-go (`KNOCKOFF_SHELL_PLAN.md` §4), the $766 pilot line moves to the custom-shell path, and that needs a new quote.

## 6. First three sourcing actions

1. **Order the v14 prototype parts as soon as the paper dry fit passes** (`../FINAL_STATUS.md` §1–2; Shopping List, $426). Before paying JLCPCB, check that **ESP32-S3-MINI-1-N4R2 (C3013941)** is in stock, and accept no substitute. Order the Adafruit #5412 cable first, because stock is low.
2. **Order the 4 knockoff samples this week:** 2 from an Alibaba "991ES PLUS" seller and 2 from AliExpress/Amazon (Runzon RZ-991ES PLUS or similar), about $60 in total. Paste the 7 questions from `KNOCKOFF_SHELL_PLAN.md` §1 into the chat first: inside photos, mould number, a plain no-logo version, and shell-only pricing. Pay through Trade Assurance.
3. **Send the RFQs for the 100-batch parts with the longest lead times** (message templates are in `Alibaba_Bulk_Sourcing.xlsx`):
   - an OV5640 AF camera from V-Vision with the Seeed XIAO-Sense pinout,
   - the GDEY0213B74 e-paper from Good Display,
   - a 150 mAh LiPo in 26 × 20 × 3.8 mm with the **UN38.3 report in the seller's name**.

   Buy samples only after the knockoff shell passes (budget line 3, $90).

## 7. Numbers to replace with real data first

| Guess today | Where it lives | Replace it with |
|---|---|---|
| 5 solves a day, 2,100 output tokens a solve | Inputs rows "Solves per active user per day" and the model table | The proxy's own usage log from the beta (the `usage` field of each answer) |
| 70 % start paying, 8 % cancel a month | Inputs "Subscription behaviour" | Beta conversions after the free month |
| Clone shell $3.99 at 100 | Unit cost row 2 | The seller's written quote for the plain version |
| ~35 % import duty | Unit cost row 13 | The tariff line JLCPCB shows at checkout on the v14 order |
| $25/month server | Inputs | The real Render/Fly bill |
