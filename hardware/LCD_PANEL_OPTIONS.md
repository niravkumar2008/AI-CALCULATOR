# LCD panel options for the v15-LCD board

Research only, 2026-10-08. Nothing was ordered and nothing was committed. Written for Nirav.

## Verdict (read this first)

> **Updated 2026-10-08 evening (final pre-order review, `verification/14_v15_final_preorder.md`): the panel to buy is the BuyDisplay ER-TFT019-1.** The original research verdict (Adafruit #5394 for prototypes) is kept below for the record, but it is superseded.

**Buy: BuyDisplay / East Rising ER-TFT019-1, the version without touch.** About $6–7 each; **$6.22 each at 10, $5.71 at 100**. Buy 3 for the prototype builds.
- Its own datasheet (rev 2.0, `ER-TFT019-1_Datasheet.pdf`) was read page by page and matches the v15 board **pin for pin** (table at the end of this file): ST7789P3, 4-wire SPI (IM1 = IM2 = 1), 25.80 × 49.72 × 1.43 mm, tail 30 pins × 0.5 mm, **36.6 ± 0.3 mm**, 4.5 mm stiffener.
- Its outline drawing confirms, independently of the Unvision drawing used in review 13, that finger 1 arrives at the **bottom** of J5 (pad 1, silk tick) with the gold fingers facing up. J5 (HDGC C2919501) is dual contact, so the face does not matter.
- It is the same glass as the TFT1901 / N190-1732TBWPG01 family the board was designed to, so no board change was needed for the panel itself. Review 14 did change **R23 to 15 Ω (C22810)** so its backlight (VLED 2.8–3.2 V at 60 mA) runs at ≈ 26 mA typical (16–37 mA range).
- **ER-CON30HT-1** (BuyDisplay's own top-contact ZIF socket, offered with the panel): **not needed**, J5 is already on the board.

**Fallback: Adafruit #5394** (1.9" 320×170 breakout, $17.50). The panel inside is the same 30-pin glass and can be taken off the breakout. Only buy it if the ER-TFT019-1 is out of stock or a ready-made test display is wanted.

**Bulk (100+): Alibaba Goldenmorning T190X7-C30-01 or ZJY, about $2.30–2.50**, after a sample check (diode test, tail length, colours). Ask for the **plastic-frame** (~1.4 mm) build; Goldenmorning's published drawing is the 2.11 mm metal frame. Details in the Alibaba section below. At 100+ a panel maker can also quote an 18–20 mm custom tail.

### Original research verdict (2026-10-08 morning, superseded)

**Keep the panel we already designed for:** the 1.9" 170×320 IPS, ST7789, 30-pin 0.5 mm FPC (the TFT1901 / N190-1732TBWPG01 / NFP190B family). Nothing else on the market fits the 60.65 × 24.3 mm window as well. Every bare 1.9" 170×320 panel I could find with a real datasheet has the **same 30-pin tail, 36.6 mm long**. A short-tail version of this panel is not sold off the shelf. You only get one by asking a panel maker for a custom FPC, which makes sense later, for bulk.

- *(Superseded by the ER-TFT019-1 above.)* Prototypes were going to be Adafruit #5394, "1.9" 320x170 Color IPS TFT Display – ST7789", $17.50, in stock, ships from the US.
  - Adafruit does **not** sell this panel bare, only on its breakout board. The breakout (57.2 × 36.6 × 5.2 mm) cannot go in the calculator.
  - Adafruit's own open-source PCB files show the panel inside is **our exact 30-pin panel**, with the **same pin numbering as J5**: pin 7 = SPI clock, pin 8 = D/C, 20 = LEDA, 21–24 = LEDK.
- Bulk was going to be the same bare 30-pin panel from AliExpress (Jessinie Store, about $2.82 + shipping, "30Pin" variant, ST7789 not the DJ9853 clone). The Alibaba section below replaced this.

## Board changes needed

> **Update 2026-10-08 (review 13):** the socket part is unchanged, but J5's footprint was mirrored (`LcdReversed`, like J1) because the panel's finger 1 arrives at the other end after the tail goes through the slot, and four of the five LCD SPI roles were swapped (now MOSI IO5, DC IO6, SCK IO8, CS IO41, RST IO42; see `hardware/verification/13_v15_lcd_review.md`). The "no pin-order change" below is superseded. Option 1 below (slot to x 180.5–181.5, J5 to (158.6, 93.1)) was **applied** in review 13; the board was verified ORDER in review 14.

**For the recommended panel: no socket change, no pin-order change.** J5 (HDGC 0.5K-HX-30PWB, LCSC C2919501, 2,240 in stock today, $0.36 at 5+) and the J5 pad N = panel pin N mapping stay exactly as drawn. Adafruit's schematic confirms the mapping independently: the Adafruit and QDtech pin tables match `stage15_lcd.md` §3 pin for pin.

**The tail length is still a problem, and it is a board-layout choice, not a part choice.** The 36.6 mm tail is about 19 mm longer than the route to J5 (`enclosure/final_assembly_v15_lcd/REPORT.md` §4). There are two ways to handle it:

1. **Recommended: lengthen the route on the PCB** (REPORT.md §5, about 30.6 mm of route, then one gentle loop):
   - Move the FPC slot outwards from x 177.4–178.4 to **x 180.5–181.5**, still y 83.1–103.1 and 1.0 × 20 mm. That leaves 2.15 mm to the board edge and 3.45 mm to the rib.
   - Move **J5 about 7 mm west**: mouth near x 160, body x 157–160. **Keep the body east of x 156.5**, because of the camera keep-out. "10 mm west" does not fit.
   - Re-place the J5 cluster (Q4, Q5, R21–R23, C19, C36) and re-route the five SPI lanes, LCD_PWR_N, LCD_BL_EN and +3V3 to the new J5 position. Pin order is unchanged.
   - Keep the strip between the slot and J5 free of parts taller than 1 mm.
2. **No board change:** fold the spare tail as the flat 3-layer Z-fold that was modelled. This works, but it uses two hard creases (r ≈ 0.18 mm), which is the riskiest point in the assembly. If you choose it, fold once over a 0.3 mm shim and never re-fold.

Only if you later switch to the Adafruit 1.3" bare panel (candidate F, not recommended) would J5 have to change. It would become a 24-pin socket, **HDGC 0.5K-HX-24PWB, LCSC C2919499**: 0.5 mm pitch, double-sided contacts, 3,690 in stock, $0.30 at 5+, the same family as C2919501, so JLCPCB places it the same way. You would also need a new pin map, because that panel's pinout is completely different (1 LEDA, 2 LEDK, 3 GND, 4 VDD, 5 VDDIO, 6 IM1/2, 7 RESET, 8 CS, 9 SCL, 10 D/C, 11 RD, 12 SDA, 13–20 DB0–7, 21 TE, 22 NC, 23–24 GND), plus a new slot position.

## Fit limits used (from REPORT.md and stage15_lcd.md)

Window 60.65 × 24.3 mm. Active height ≤ ~23.5 mm, outline height ≤ ~26 mm, outline length ≤ ~58 mm, thickness ≤ ~2.0 mm. Bare panel with an FPC tail, SPI, 3.3 V. Route from glass edge to J5 is 17.5 mm today and about 30.6 mm with change 1 above.

## Comparison

Prices and stock were seen on 2026-10-08. "—" means not published or not visible.

| # | Seller | Part / SKU | Price 1 / 10 / 100 | Stock | Size, active area, resolution | Controller, interface | FPC pins / pitch / tail | Backlight, brightness | Datasheet | Fit verdict |
|---|---|---|---|---|---|---|---|---|---|---|
| **A** | **Adafruit** (US) | **#5394** 1.9" 320×170 IPS breakout | **$17.50 / $15.75 / $14.00** | In stock | Breakout 57.2 × 36.6 × 5.2 mm. Panel inside: 25.8 × 49.72 × 1.43 mm, active 22.70 × 42.72 mm, 170×320 IPS | ST7789, 4-wire SPI (8080 pins grounded) | 30 / 0.5 mm, in a 30-pin connector on the breakout; tail ≈ 36–38 mm (same panel as C) | 4 LEDs, one anode and 4 cathodes | Adafruit Eagle files (pin map) + TFT1901 spec (C) | **Panel fits** (same as the current design). The breakout itself does not fit; take the panel off it. Best for prototypes. |
| **B** | AliExpress, Jessinie Store | "1.9 inch … Bare Screen 30Pin Plug-in", variants "Metal frame 30Pin" / "Plastic frame 30Pin" (FPC printed NFP190B-21AF / -21A) | $2.82 (sale; list $3.76), shipping $5.27; no qty breaks shown | "99999 available"; delivery Oct 17–24 | 1.9" 170×320 IPS, same outline as C (seller gives no drawing) | ST7789 (also sold as a DJ9853 clone; avoid) | 30 / 0.5 mm, tail ≈ 35–37 mm from the photos | — | None from seller (the "manual" is a CE leaflet); use the TFT1901 spec | **Fits**, same panel. Best for bulk / spares. Check the first batch with the meter test (stage15 §6). |
| **C** | QDtech / LCDwiki (spec source); Heltec, LilyGO spares | **TFT1901** (= N190-1732TBWPG01-C30 family) | Not sold by LCDwiki as a bare part; ~$3–6 on AliExpress / Taobao | — | Outline 25.8 × 49.72 × 1.43 mm (BL), glass 24.8 × 48.52, active 22.695 × 42.72, viewing area 23.695 × 43.72, 170×320 IPS, 0.1335 mm pixels | ST7789, 4-SPI or 8080-8 bit (IM1/IM2) | **30 / 0.5 mm, 15.5 mm wide, 36.6 ± 0.3 mm long, 0.3 mm thick, 4.5 mm stiffener** | 4 white LEDs; drawing says 80 mA at 3.0–3.4 V, 350 cd/m² (Heltec spec says 60 mA) | [TFT1901 spec](https://www.lcdwiki.com/res/MSP1901/TFT1901-SPEC_V1.0.pdf) | **Fits.** This is the reference drawing the v15 board was designed to. |
| **D** | NoseDisplay (manufacturer) | **ND019TSN170320-A01** | Quote only | Samples on request | Outline 25.10 × 49.30 × **2.10** mm, active 22.7 × 42.72, 170×320 IPS | ST7789P3, SPI | 30-pin; pins 1–24 same as C, 25–30 unlisted; tail not published. **Custom FPC length offered** | 350 cd/m² | [Product page](https://www.nosedisplay.com/products/1-9-inch-tft-display-170x320-resolution-spi-interface-st7789p3-controller-ips-screen/) (spec on request) | **Fits in height.** 2.10 mm is 0.1 mm over the 2.0 mm limit; REPORT.md says even a 2.0 mm module leaves 0.7 mm. Bulk option if a short tail is wanted. |
| **E** | **BuyDisplay / East Rising** | **ER-TFT019-1** (no touch) — **CHOSEN (review 14)** | ~$6–7 / **$6.22** / **$5.71** (Chinese site: ¥43.45 / ¥42.30 / ¥38.83) | In stock | 25.80 × 49.72 × 1.43 mm, active 22.69 × 42.72, 170×320 IPS | ST7789P3, 8080 or 4-SPI (IM1 = IM2 = 1); touch version optional, not wanted | 30 / 0.5 mm, 15.5 wide, **36.6 ± 0.3 mm**, 4.5 mm stiffener | 4 LEDs (cathode on pin 22), VLED 2.8–3.2 V at 60 mA | Datasheet rev 2.0 (read, see the end of this file); [Product page](https://www.buydisplay.com/full-viewing-angle-spi-1-9-inch-ips-tft-lcd-display-panel-170x320-st7789) | **Fits, verified pin for pin** against its datasheet. Prototype panel. Custom-FPC service also offered. |
| **F** | **Adafruit** (US) | **#4520** 1.3" 240×240 IPS, **bare** (ZJY133T-IF05) | $12.50 / $11.25 / $10.00 | In stock (11 shown) | Outline 26.16 × 29.22 × 1.5 mm, active 23.40 × 23.40 | ST7789VW, 4-SPI / 8080 | **24 / 0.5 mm, tail ≈ 14.9 mm** past the backlight edge, 12.5 mm wide, 0.3 mm stiffener | 2 LEDs, 30–40 mA at 2.8–3.0 V, 250 cd/m² | [Adafruit PDF](https://cdn-shop.adafruit.com/product-files/4520/4520_C13462__________.pdf) | **Fits, but poor.** Outline height 26.16 is 0.16 mm over the limit. The picture is a 23.4 mm square, only 39 % of the window width. Needs a 24-pin socket (C2919499), new pin map and new slot. Not recommended. |
| G | LCSC | Wisevision **N114-2413THBIG01-H13** (C2890618) 1.14" 240×135 IPS | $2.56 / $2.16 / $1.66 (96+) | 170 | 1.14" (typically ~25 × 15 mm active) | ST7789V, SPI | Not checked | — | [LCSC](https://www.lcsc.com/product-detail/C2890618.html) | **Too small** (about 25 mm wide image). Not recommended. |
| H | LCSC / JLCPCB | HS20HS072RX (C5329582) 2.0" 240×320 | $3.40 / $2.90 / $2.15 (500+) | 1,174 | Active 30.6 × 40.8 | ST7789, SPI | — | — | [LCSC](https://www.lcsc.com/product-detail/C5329582.html) | **Does not fit**: active height 30.6 mm is taller than the 24.3 mm window (already rejected in stage15 §2). |

**LCSC note:** LCSC and JLCPCB do not stock any 1.9" 170×320 panel today. I searched "170x320", "ST7789V3", "N190-1732", the Wisevision range and the 1.9" size filter. So no version of this panel can be JLCPCB-placed; it is plugged in by hand, as planned. The only "ZJY190S0800TG01" at JLCPCB is a customer-consigned breakout module with 0 stock.

**Adafruit's other options:**
- The only bare colour TFTs Adafruit sells that are this small are #4520 (1.3", F) and #4421 (1.54" 240×240). The 1.54" active area is about 27.7 mm square, too tall for the window.
- Everything else small (1.14" #4383/#6113, 1.47" #5393, 1.69" #5206, 1.9" #5394, 2.0" #4311) is a breakout with headers and will not fit in the calculator.
- #5394 is still the right Adafruit buy, because its panel can be taken off the breakout.

## Practical notes

- **Pin 1 check:** do the diode test from `stage15_lcd.md` §6 on the first panel from each source. The backlight LEDs are between finger 20 and fingers 21–24.
- **Brightness:** the TFT1901 drawing rates the backlight at 80 mA (3.0–3.4 V) and 350 cd/m²; the Heltec spec and the ER-TFT019-1 say 60 mA (ER: VLED 2.8 / 3.0 / 3.2 V). With **R23 = 15 Ω** (review 14) the board drives ≈ 26 mA typical, 16–37 mA across the Vf range, never above 51 mA, so the panel is inside its rating either way.
- **Viewing angle:** all candidates A–E are IPS, rated about 80° in every direction.
- **On the Adafruit breakout the panel is held by tape.** Adafruit warns that the backlight can separate from the glass if nothing holds it down. On the v15 board the 0.1 mm tape and the front plate do that job.

## Sources (all accessed 2026-10-08)

- Adafruit #5394 product page (price tiers, stock, breakout size, "bare panel not sold separately"): https://www.adafruit.com/product/5394
- Adafruit #5394 open-source PCB (Eagle schematic and board: part `DISP_LCD_ST7789_1.9IN`, package `TFT_1.9IN_170X320_30P`, pin map): https://github.com/adafruit/Adafruit-1.9in-320x170-Color-IPS-TFT-PCB
- Adafruit 1.9" learn guide downloads: https://learn.adafruit.com/adafruit-1-9-color-ips-tft-display/downloads
- Adafruit #4520 bare 1.3" (price tiers, stock): https://www.adafruit.com/product/4520 ; datasheet ZJY133T-IF05: https://cdn-shop.adafruit.com/product-files/4520/4520_C13462__________.pdf
- Adafruit TFT/LCD category listing: https://www.adafruit.com/category/97
- LCDwiki 1.9inch IPS Module (MSP1901) and the TFT1901 spec (drawing: tail 36.6 ± 0.3 mm, pin table): https://www.lcdwiki.com/1.9inch_IPS_Module , https://www.lcdwiki.com/res/MSP1901/TFT1901-SPEC_V1.0.pdf
- AliExpress Jessinie Store, 1.9" bare 30-pin: https://www.aliexpress.us/item/3256812660787172.html
- NoseDisplay ND019TSN170320-A01: https://www.nosedisplay.com/products/1-9-inch-tft-display-170x320-resolution-spi-interface-st7789p3-controller-ips-screen/
- BuyDisplay ER-TFT019-1 (page behind a bot check; price from the Chinese mirror via search): https://www.buydisplay.com/full-viewing-angle-spi-1-9-inch-ips-tft-lcd-display-panel-170x320-st7789
- LCSC: C2919501 (30-pin socket) https://www.lcsc.com/product-detail/C2919501.html ; C2919499 (24-pin socket) via https://www.lcsc.com/search?q=0.5K-HX-24PWB ; C2890618 https://www.lcsc.com/product-detail/C2890618.html ; C5329582 https://www.lcsc.com/product-detail/C5329582.html ; LCD category https://www.lcsc.com/category/405.html
- Waveshare 1.9inch LCD Module (a breakout using a different 24+2-pin panel): https://www.waveshare.com/wiki/1.9inch_LCD_Module
- In this repo: `hardware/stage15_lcd.md`, `hardware/enclosure/final_assembly_v15_lcd/REPORT.md`

## Alibaba.com suppliers (2026-10-08)

Research only. No supplier was contacted, no account was created, nothing was ordered or committed.

**How this was gathered, and its limits.** One Alibaba.com search results page loaded normally and gave the data in the table: title, price range, MOQ, years on Alibaba, rating, "sold" count, delivery estimate and purchase-protection badges. After that, every Alibaba product page, supplier page and new search sent back a slider CAPTCHA. Solving CAPTCHAs is off limits, so I did not open any listing's detail page. That means three things:
- The quantity price tiers come from search-engine snippets of the listing, and only where a snippet showed them.
- "Verified" and Trade Assurance status could not be read. The cards only show "Easy Return" or "Money-back guarantee", which are Alibaba's order-protection badges.
- No listing's own pin drawing could be checked. The only pin table I confirmed is in a datasheet the maker publishes on its own website (Goldenmorning).

Re-check the price tiers in a normal browser before ordering.

**What has to match** (from `stage15_lcd.md` and `verification/13_v15_lcd_review.md`):
- 30-pin, 0.5 mm FPC: 1/25/30 GND, 2 VDD, 3–4 IM2/IM1, 5 RESET, 6 CS, 7 SCL, 8 D/C (WR/RS), 9 RD, 10 SDA, 11–18 DB0–7, 19 SDO/NC, 20 LEDA, 21–24 LEDK, 26–29 NC/TP.
- Outline about 25.8 × 49.7 mm, thickness ≤ 1.6 mm preferred (2.0 mm is the hard limit), active area 22.7 × 42.72 mm.

| # | Supplier (yrs on Alibaba, rating) | Listing | Price seen (1 / 100 / 1,000) | MOQ | Controller (as listed) | Pinout match | Tail | Custom FPC | Delivery estimate / notes |
|---|---|---|---|---|---|---|---|---|---|
| 1 | **Shenzhen Goldenmorning Electronic Co., Ltd.** (8 yrs, CN, 4.7/5, 16 reviews; maker with its own site; badge "Easy Return"; verified/TA not visible) | [T190X7-C30-01, 1,405 sold](https://www.alibaba.com/product-detail/1-9-Inch-IPS-TFT-LCD_1601731552729.html); also [11000024648172](https://www.alibaba.com/product-detail/1-9-Inch-TFT-LCD-Display_11000024648172.html) (270 sold) | $1.85–2.50 range, tier breaks not visible (a snippet of the store page shows $1.50–2) | 1 pc | ST7789**P3** | **YES**: the [datasheet](https://goldenmorninglcd.com/wp-content/uploads/2025/06/T190X7-C30-01-V1.pdf), p.5/p.7, has every pin 1–30 identical to the TFT1901 / N190 table above, 4-line SPI with IM1 = IM2 = 1 | **36.6 ± 0.5 mm**, 15.5 mm wide, 0.3 mm stiffener | Website offers FPC-length and frame options, "contact" (no MOQ stated) | No date shown. **Caution:** this drawing is the **metal-frame (SUS304) version, 25.0 × 49.33 × 2.11 mm**, which is over both 1.6 and 2.0 mm. Ask for the plastic-frame version (~1.4 mm). Backlight 4 LEDs, Vf 3.2 V typ at 80 mA. |
| 2 | **Zhengzhou Zhongjingyuan Electronic Technology Co., Ltd. (ZJY)** (5 yrs, CN, 5.0/5, 1 review; panel brand) | [1600647344150](https://www.alibaba.com/product-detail/1-9-Inch-170x320-30-PIN_1600647344150.html), 52 sold | **$2.50 (2–999) / $2.50 / $2.30 (1,000–4,999)**; $2.10 at 5k, $2.00 at 10k+ | 2 pcs | ST7789**V3** (listing keywords); active 22.7 × 42.72 mm, 350 cd/m², 4-SPI / 8080 | Unclear. The specs match the N190/TFT1901 glass, but the drawing could not be opened | Not visible (probably 36.6 mm) | Not stated | Badge "Easy Return". No date shown |
| 3 | **Shenzhen Toppop Electronic Co., Ltd.** (8 yrs, CN, 4.8/5, 25 reviews; 19 % reorder rate) | [1600477280385](https://www.alibaba.com/product-detail/1-9-Inch-170-320-ST7789_1600477280385.html) $2.30, 61 sold; ["IN STOCK" 1601646904134](https://www.alibaba.com/product-detail/-IN-STOCK-1-9-Inch_1601646904134.html) $2.20 | $2.20–2.30 single price (a snippet shows $2.00); no tiers visible | 2 pcs | ST7789 (their touch version is listed as ST7789V2) | Unclear | Not visible | "Factory custom touch panel"; FPC not stated | Delivery by Nov 05 (~4 weeks). Has "Add to cart" |
| 4 | **Shenzhen Boyida Electronics Co., Ltd.** (4 yrs, CN, 4.8/5, 31 reviews) | [1601787007362](https://www.alibaba.com/product-detail/1-9-Inch-TFT-LCD-Display_1601787007362.html) "30-Pin … with Plastic Frame" | **$1.42–1.98** (cheapest seen; tiers not visible) | 5 pcs | ST7789 | Unclear ("plastic frame" suggests the thin ~1.4 mm version) | Not visible | Not stated | Badge "Easy Return". No date shown |
| 5 | **Shenzhen Sungood Electronics Technology Co., Ltd.** (4 yrs, CN, 4.7/5, 57 reviews) | [1600915530174](https://www.alibaba.com/product-detail/1-9-inch-TFT-LCD-display_1600915530174.html) $2.30–3.20; [1600822949258](https://www.alibaba.com/product-detail/1-9-inch-TFT-LCD-display_1600822949258.html) $2.37; [1601161206062](https://www.alibaba.com/product-detail/1-9-Inch-TFT-IPS-LCD_1601161206062.html) $2.52–2.57 | $2.30–3.20; tiers not visible | 5 pcs | ST7789 | Unclear | Not visible | Not stated | Distributor ("New and Original"). 1601161206062 shows "Money-back guarantee", delivery by Oct 31 (~3 weeks) |
| 6 | **Shenzhen Chance Technology Co., Ltd.** (9 yrs, CN, 4.8/5, 26 reviews) | [1601471613948](https://www.alibaba.com/product-detail/1-9-Inch-170x320-LCD-Display_1601471613948.html), "10 Pin / 30 Pin … 350 Nits" | $2.65 | 2 pcs | ST7789 | Unclear. Pick the 30-pin variant; 350 nits matches N190 | Not visible | Not stated | Delivery by Nov 05 |
| 7 | **Shenzhen Huake Baiyu Technology Co., Ltd.** (5 yrs, CN, 4.8/5; 30 % reorder rate) | [1601058915194](https://www.alibaba.com/product-detail/Bar-Type-1-9-Inch-170_1601058915194.html), "30pins … Optional Touch" | $4 | **500 pcs** | ST7789 | Unclear | Not visible | Optional touch; ask about FPC | Delivery by Jan 15. Bulk/OEM only |
| 8 | HELLOCHIP LIMITED (1 yr, HK trader, 4.7/5, 554 reviews) | [1601883159751](https://www.alibaba.com/product-detail/1-9-Inch-IPS-TFT-LCD_1601883159751.html) | $1.80–2.30 | 1 pc | ST7789 | Unclear | Not visible | No (trader) | Young trader account. Fine for samples, not for bulk |
| 9 | Shenzhen Xinliwei Electronics Technology Co., Ltd (6 yrs, CN, 4.9/5, 41 reviews) | [1601848392382](https://www.alibaba.com/product-detail/1-9-Inch-IPS-TFT-LCD_1601848392382.html), resells Goldenmorning **T190X7-C30-01** | $2.26 | 1 pc | ST7789P3 | Yes, if it really is T190X7-C30-01 (same datasheet as #1) | 36.6 mm | No | Delivery by Nov 05. Has "Add to cart" |

Other 1.9" 30-pin listings I saw were customizable CTP/touch versions or higher priced. Examples: Zhuohong $4.20–5.50, Kaisheng Century $1.76–1.78, Qintang Shengshi $1.67–3.50, Dinsen $2.48–6.50, Duoweisi $2.25–2.28 at MOQ 10, Shenzhen Smart Electronics $7.80–13.50. All are in the same search; none had a pin table I could check.

**Thin-glass factories worth a quote request for the custom short tail (not confirmed as Alibaba stores):**
- **Team Source Display (TSD)**: [TST019QVBS-02B](https://www.tslcd.com/tsd-1-9-inch-170x320-350-nits-mcu-interface-tft-lcd-display-module_p752.html). Outline **25.8 × 49.72 × 1.48 mm** (the exact N190 outline), ST7789V2, 30-pin, 4 LEDs at 60 mA / 3.0 V. They advertise FPC customization.
- **iFan Display**: [IF019GU17-32](https://ifan-display.com/product/1-9-inch-mcu-tft-lcd-screen-170x320/). 30-pin, ST7789V, 1.36 mm thick, active area 22.7 × 42.72 mm.
- Neither publishes an MOQ for a custom FPC. None of the Alibaba listings states a custom-tail MOQ or price either. Expect a one-off FPC tooling charge plus about 1k pcs; this is not confirmed.

**ST7789V2 / V3 / P3, does the init change?** No. All three use the same ST7789 command set and the same 240 × 320 RAM. A 170-wide glass sits at **column offset 35** on all of them. The `firmware-v15-lcd/src/lcd.cpp` sequence (LovyanGFX `Panel_ST7789`, offset_x 35, INVON) applies unchanged. Vendors' example code only differs in optional gamma and power trims (PORCTRL / VCOMS / PVGAMCTRL). Those change the colour tint slightly, not whether the panel works. Test colours and inversion on the first sample.

**Recommendation**
- **Samples: Goldenmorning T190X7-C30-01** (#1, $1.85–2.50, MOQ 1).
  - Why: it is the only Alibaba listing whose pin table I could actually check, and it matches J5 pin for pin. It comes from a manufacturer that publishes a full datasheet.
  - Order 2–3 pcs and **ask for the plastic-frame (~1.4 mm) build**, because the published drawing is the 2.11 mm metal-frame build.
  - If they only have metal-frame stock, the sample is still good for the pin-1 diode test, firmware bring-up and the Vf measurement (§S1/§M1). It is not good for the final fit.
  - A cheap second source to sample at the same time: Boyida #4 (plastic frame, $1.42–1.98, MOQ 5).
- **100–1,000 units: Zhongjingyuan (ZJY)** (#2).
  - Why: the only listing with published quantity tiers ($2.50 at 100, $2.30 at 1,000, MOQ 2). It is a panel brand, and the listing's ST7789V3 / 350 cd/m² / 22.7 × 42.72 mm specs match the N190/TFT1901 glass.
  - **Conditional:** get their drawing and pin table first, and buy 2 pcs to check before the bulk PO.
  - Fallback: Goldenmorning at volume, if it can supply the thin frame.
  - If a short tail is wanted at 1k, get a custom-FPC quote from Goldenmorning or TSD.

**Ask the supplier before ordering (3 questions)**
1. "Please send the mechanical drawing and the 30-pin table for this exact part number. Is pin 7 = SCL, 8 = D/C (WR/RS), 10 = SDA, 20 = LED+, 21–24 = LED−, 1/25/30 = GND? What is the outline thickness: plastic frame about 1.4 mm, or metal frame 2.1 mm?"
2. "Which controller is on the glass (ST7789V3, V2 or P3, not a clone such as DJ9853)? Please send your init code. What is the backlight forward voltage and the rated current per LED and total (Vf at 20 mA)?"
3. "Can you make the same panel with the FPC tail shortened to 18–20 mm (same 0.5 mm × 30 pin, same pin order, 0.3 mm stiffener)? What are the MOQ, the tooling/NRE cost, the unit price at 100 / 500 / 1,000, the sample lead time and the mass-production lead time?"

**Shipping notes:**
- Small sample orders on Alibaba usually ship by express (DHL/FedEx/UPS) at $15–30 for a few panels. "Delivery by" dates on the cards were Oct 31 – Nov 5 for in-stock items, and Jan 15 for Huake's 500-pc MOQ.
- Panels are glass. Ask for tray + ESD bag packing.
- US import duty on Chinese LCD modules applies at the bulk stage. Budget for it in `production/UNIT_ECONOMICS.md`; it was not researched here.

**Sources (accessed 2026-10-08):**
- Alibaba.com search "1.9 inch 170x320 ST7789 30pin tft": https://www.alibaba.com/trade/search?SearchText=1.9+inch+170x320+ST7789+30pin+tft
- The listing URLs in the table.
- Goldenmorning T190X7-C30-01 page and datasheet: https://goldenmorninglcd.com/tft-display/1.9-inch-170x320-st7789p3-t190x7-c30-01/ , https://goldenmorninglcd.com/wp-content/uploads/2025/06/T190X7-C30-01-V1.pdf
- Goldenmorning Alibaba store (blocked by CAPTCHA): https://goldenmorning.en.alibaba.com/
- TSD: https://www.tslcd.com/tsd-1-9-inch-170x320-350-nits-mcu-interface-tft-lcd-display-module_p752.html
- iFan: https://ifan-display.com/product/1-9-inch-mcu-tft-lcd-screen-170x320/
- Tier prices for ZJY/Toppop: web-search snippets of the listing pages.

Machine-readable summary: `hardware/production/lcd_suppliers.json`.

## BuyDisplay ER-TFT019-1: checked against its datasheet (2026-10-08)

Source: `ER-TFT019-1_Datasheet.pdf` (EastRising/BuyDisplay, rev 2.0), pages 5, 6, 8, 9, downloaded by Nirav.

**Verdict: compatible with the v15-LCD board as drawn. It is the same panel (identical outline and tail), no board change for the panel itself; R23 was changed to 15 Ω in review 14 to suit its backlight. This is the panel to buy.**

| Check | ER-TFT019-1 | v15 board expects | OK |
|---|---|---|---|
| Size | 25.80 × 49.72 × 1.43 mm, active 22.69 × 42.72 | same panel family, ≤ 2.0 mm | yes |
| Controller / interface | ST7789P3, 4-wire SPI (IM1 = IM2 = 1) | ST7789, SPI, IM1/IM2 to VDD | yes |
| Ribbon | 30 pin, 0.5 mm, 15.5 wide, 0.3 thick, **36.6 ± 0.3 mm**, 4.5 mm stiffener | 36.6 mm route (one bow), 30 pin 0.5 mm | yes |
| Finger order | pin 1 at the top, 30 at the bottom (display-face view, tail to the right) | same as the Unvision drawing used in review 13 | yes |
| Pins 1–10 | GND, VDD, IM2, IM1, RESET, CS, SCL, RS (D/C), RD, SDA | same | yes |
| 11–18 | D0–D7 | tied to GND | yes |
| 19 | NC | SDO, left open | yes |
| 20 | LEDA | LEDA | yes |
| 21–24 | NC, **LEDK (22 only)**, NC, NC | 21–24 tied together as LEDK | yes: the NC pins are harmless; the cathode is on 22, which is in the tied group |
| 25–30 | GND, CTP_RST/SCL/SDA/INT (touch version only), GND | GND, 26–29 open | yes |
| VDD | 2.4–3.3 V (typ 2.8) | 3.3 V via Q4 | yes, at the top of the range (as noted in review 13) |
| Backlight | Vf 2.8–3.2 V (typ 3.0) at 60 mA | R23 **15 Ω** (C22810) from 3.3 V (was 22 Ω) | yes: ≈ 26 mA typical, 16–37 mA range, ≤ 51 mA worst case (review 14 M1; 22 Ω would have given only 12–27 mA). Measure V(R23)/15 on the first board |

The ER-CON30HT-1 connector in the same download is BuyDisplay's own top-contact ZIF socket. It is **not needed**: the board uses the dual-contact HDGC C2919501 (J5), which takes the ribbon either face up.
