"""Builds hardware/production/COST_MODEL.xlsx (formulas, blue inputs, sources beside them)."""
import openpyxl
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter as L
from openpyxl.worksheet.datavalidation import DataValidation

OUT = r"C:\Users\r_kas\OneDrive\Documents\GitHub\AI-CALCULATOR\hardware\production\COST_MODEL.xlsx"

F = "Arial"
BLUE = Font(name=F, size=10, color="0000FF")
BLACK = Font(name=F, size=10, color="000000")
BOLD = Font(name=F, size=10, bold=True)
TITLE = Font(name=F, size=13, bold=True)
NOTE = Font(name=F, size=9, italic=True, color="555555")
HDR_FILL = PatternFill("solid", fgColor="D9E1F2")
TOT_FILL = PatternFill("solid", fgColor="F2F2F2")
INPUT_FILL = PatternFill("solid", fgColor="FFF9E5")
thin = Side(style="thin", color="BBBBBB")
TOP = Border(top=Side(style="thin", color="000000"))
USD = '"$"#,##0.00'
USD0 = '"$"#,##0'
PCT = '0.0%'
NUM = '#,##0'
NUM2 = '#,##0.00'
WRAP = Alignment(wrap_text=True, vertical="top")

wb = openpyxl.Workbook()
names = {}  # name -> "'Sheet'!$X$n"


def ref(sheet, col, row, absolute=True):
    c = f"${col}${row}" if absolute else f"{col}{row}"
    return f"'{sheet}'!{c}"


def put(ws, cell, value, font=BLACK, fmt=None, fill=None, bold=False):
    c = ws[cell]
    c.value = value
    c.font = Font(name=F, size=10, bold=bold, color=font.color) if bold else font
    if fmt:
        c.number_format = fmt
    if fill:
        c.fill = fill
    return c


def header(ws, row, labels, widths=None):
    for i, lab in enumerate(labels, start=1):
        c = ws.cell(row=row, column=i, value=lab)
        c.font = BOLD
        c.fill = HDR_FILL
        c.alignment = Alignment(wrap_text=True, vertical="center")
    if widths:
        for i, w in enumerate(widths, start=1):
            ws.column_dimensions[L(i)].width = w


# ---------------------------------------------------------------- Inputs
wsI = wb.active
wsI.title = "Inputs"
S = "Inputs"
put(wsI, "A1", "Inputs: every blue number is an assumption you can edit. Black = formula.", TITLE)
put(wsI, "A2", "Written 2026-10-06. Sources beside each input. (estimate) = no hard source; confirm by quote.", NOTE)
header(wsI, 4, ["Input", "Value", "Unit", "Source / why"], [46, 12, 14, 110])

inputs = [
    ("sec", "Pricing and fees"),
    ("hw_price", "Calculator price", 225, "$", USD, "Decided 2026-10-04 (LAUNCH_ROADMAP.md §0)."),
    ("sub_price", "Subscription price per month", 15, "$/month", USD, "Decided 2026-10-04 (LAUNCH_ROADMAP.md §0)."),
    ("free_months", "Free months at the start", 1, "months", NUM, "First month free = proxy TRIAL_DAYS=30 (server/proxy/config.py)."),
    ("monthly_cap", "Fair-use cap, solves per month", 500, "solves", NUM, "Proxy MONTHLY_CAP=500 (server/proxy/config.py)."),
    ("pay_pct", "Card fee, percent", 0.029, "%", PCT, "Stripe US standard online card rate 2.9 % + 30¢ (estimate; check your Stripe dashboard)."),
    ("pay_fixed", "Card fee, fixed per charge", 0.30, "$", USD, "Stripe US standard 30¢ per charge (estimate)."),
    ("ship_out", "Shipping one calculator to a customer", 6.00, "$", USD, "USPS Ground Advantage, ~8 oz padded box, zone 4-5 (estimate)."),
    ("warranty_pct", "Returns / warranty allowance (% of price)", 0.03, "%", PCT, "Roadmap §5: budget 2-5 % of units needing help or replacement (estimate)."),
    ("sec", "Subscription behaviour"),
    ("attach", "Buyers who start paying after the free month", 0.70, "%", PCT, "Assumption (estimate). Nothing measured yet: replace with beta data."),
    ("churn", "Monthly cancellation rate of paying users", 0.08, "%/month", PCT, "Assumption (estimate). Student products churn in summer; 8 %/month ≈ half gone after 8 months."),
    ("horizon", "Months counted for subscription value (LTV)", 12, "months", NUM, "One school year plus summer. Conservative; anything after month 12 is upside."),
    ("server_month", "Proxy hosting per month (server + database)", 25, "$/month", USD, "Small Render/Fly instance + disk (estimate). server/proxy/README.md: not deployed yet."),
    ("sec", "Claude API usage (per solve)"),
    ("model", "Model used for solves", "Opus 5.5", "", None, "Pick from the table below (Opus 5.5 / Sonnet 5.5 / Haiku 4.5). NOTE: the proxy's default MAIN_MODEL today is claude-sonnet-5-5 (config.py)."),
    ("solves_day", "Solves per active user per day (typical)", 5, "solves/day", NUM2, "Roadmap §3.3 'typical' = 150 solves/month ≈ 5/day (estimate)."),
    ("days_month", "Days per month", 30.4, "days", NUM2, "365 / 12."),
    ("in_tokens", "Input tokens per solve (photo + prompt)", 2500, "tokens", NUM, "Roadmap §3.3: photo ≈ (w×h)/750 ≈ 1,600 for 1280×960, plus system prompt ≈ 2,500 total (estimate)."),
    ("cached_tokens", "…of which a fixed system prompt that can be cached", 900, "tokens", NUM, "Estimate. If the fixed prompt is shorter than the model's minimum cacheable prefix (512-4,096 tokens) it will not cache: set 0."),
    ("cache_hit", "Cache hit rate on that prefix", 0.80, "%", PCT, "Estimate (cache lives ~5 min; busy hours hit, quiet hours miss)."),
    ("cache_write_mult", "Cache write price as a multiple of input price", 1.25, "×", NUM2, "Anthropic 5-minute cache write = 1.25× base input."),
    ("batch_disc", "Batch discount applied", 0.0, "%", PCT, "Batch API is 50 % off but answers take minutes to hours: unusable for live solves. Keep 0 %."),
    ("sec", "Selling legally (one-time, before public sales)"),
    ("fcc", "FCC Part 15B SDoC test (1 lab day)", 1750, "$", USD0, "Roadmap §4: $1,000-2,500 [S10]; midpoint."),
    ("lawyer", "Lawyer: trade dress + terms of service", 300, "$", USD0, "Roadmap §4: clinic $0-300; KNOCKOFF_SHELL_PLAN.md §5: private consult $200-500."),
    ("llc", "Indiana LLC", 97, "$", USD0, "Roadmap §4 [S25]."),
    ("trademark", "USPTO trademark, classes 9 + 42", 700, "$", USD0, "Roadmap §4: $350 per class [S24]."),
    ("insurance", "Product liability insurance, first year", 700, "$", USD0, "Roadmap §4: ~$400-1,000/yr (estimate)."),
    ("sec", "JLCPCB assembly fee check (already inside the PCBA price row)"),
    ("jlc_setup", "Standard PCBA setup fee per order", 25, "$", USD, "Given 2026-10-06 (JLCPCB Standard PCBA)."),
    ("ext_fee", "Extended-part loading fee per part type", 3, "$", USD, "Roadmap §3.1: $3 × 14 extended types per order (JLCPCB rule)."),
    ("ext_types", "Extended part types on the board", 14, "types", NUM, "Roadmap §3.1."),
    ("joints", "Solder joints per board", 310, "joints", NUM, "Estimate from the 77-designator BOM: ESP32 module 65, TCA8418 25, 2 FPCs 52, SOT-23s ~45, connectors ~12, ~55 passives × 2."),
    ("joint_rate", "Price per solder joint at 100 boards", 0.0013, "$", '"$"0.0000', "Given 2026-10-06 (JLCPCB)."),
    ("sec", "Your own time (shown, not added to cost at 2-100)"),
    ("own_min", "Your minutes per unit: grind + assemble + QA", 37, "min", NUM, "Grinds with jigs 9-11 min (jigs/README.md), assembly ~15 min (estimate), QA Part A 5 + Part B 6-8 min (QA_TEST_PLAN.md)."),
    ("own_rate", "Value of your hour", 15, "$/h", USD, "Assumption: a student job wage (estimate)."),
    ("sec", "Budget"),
    ("budget", "Cash available for supply", 2000, "$", USD0, "Business plan: $2,000 budget."),
]

r = 5
for item in inputs:
    if item[0] == "sec":
        put(wsI, f"A{r}", item[1], BOLD)
        r += 1
        continue
    key, label, val, unit, fmt, src = item
    put(wsI, f"A{r}", label)
    c = put(wsI, f"B{r}", val, BLUE, fmt, INPUT_FILL)
    put(wsI, f"C{r}", unit)
    d = put(wsI, f"D{r}", src)
    d.alignment = WRAP
    names[key] = ref(S, "B", r)
    r += 1

# model price table
r += 1
put(wsI, f"A{r}", "Claude model prices (per million tokens)", BOLD)
r += 1
for i, lab in enumerate(["Model", "Input $/M", "Output $/M", "Cache read $/M", "Output tokens per solve", "Source"], start=1):
    c = wsI.cell(row=r, column=i, value=lab)
    c.font = BOLD
    c.fill = HDR_FILL
# widen E/F for table
wsI.column_dimensions["E"].width = 14
wsI.column_dimensions["F"].width = 70
mt_first = r + 1
models = [
    ("Opus 5.5", 4, 20, 0.20, 2100, "Given 2026-10-06 and Anthropic price table (cached 2026-09-25). Output incl. thinking (roadmap §3.3)."),
    ("Sonnet 5.5", 2, 10, 0.20, 2100, "Anthropic price table (cached 2026-09-25). Proxy default MAIN_MODEL."),
    ("Haiku 4.5", 1, 5, 0.10, 800, "Anthropic price table; cache read = 0.1× input (estimate). Roadmap §3.3: ~800 output tokens."),
]
for m in models:
    r += 1
    put(wsI, f"A{r}", m[0], BLUE, None, INPUT_FILL)
    put(wsI, f"B{r}", m[1], BLUE, USD, INPUT_FILL)
    put(wsI, f"C{r}", m[2], BLUE, USD, INPUT_FILL)
    put(wsI, f"D{r}", m[3], BLUE, USD, INPUT_FILL)
    put(wsI, f"E{r}", m[4], BLUE, NUM, INPUT_FILL)
    put(wsI, f"F{r}", m[5]).alignment = WRAP
mt_last = r
MT = {k: f"'{S}'!${c}${mt_first}:${c}${mt_last}" for k, c in
      [("name", "A"), ("in", "B"), ("out", "C"), ("cache", "D"), ("otok", "E")]}
dv = DataValidation(type="list", formula1=f"=$A${mt_first}:$A${mt_last}", allow_blank=False)
wsI.add_data_validation(dv)
dv.add(names["model"].split("!")[1].replace("$", ""))

r += 2
put(wsI, f"A{r}", "Derived certification total (formula)", BOLD)
put(wsI, f"B{r}", f"={names['fcc']}+{names['lawyer']}+{names['llc']}+{names['trademark']}+{names['insurance']}", BLACK, USD0)
names["cert_total"] = ref(S, "B", r)
put(wsI, f"D{r}", "FCC + lawyer + LLC + trademark + insurance. Roadmap §3 calls this 'about $3.5k one-time'.")
wsI.freeze_panes = "A5"

# ---------------------------------------------------------------- Unit cost
wsU = wb.create_sheet("Unit cost")
U = "Unit cost"
put(wsU, "A1", "Cost per finished calculator at 2 / 10 / 100 / 1,000 units", TITLE)
put(wsU, "A2", "2 = Casio fx-115ES prototypes (v14, retail parts). 10 / 100 / 1,000 = knockoff 991ES-style shell (v15 board). "
               "Per-unit prices from Claude outputs/Alibaba_Bulk_Sourcing.xlsx 'Cost by batch' and LAUNCH_ROADMAP.md §3 unless noted.", NOTE)
header(wsU, 4, ["#", "Item", "2 units", "10 units", "100 units", "1,000 units", "Source / notes"],
       [5, 52, 13, 13, 13, 13, 95])
VOLS = ["C", "D", "E", "F"]
put(wsU, "B5", "Batch size (units)", BOLD)
for col, v in zip(VOLS, [2, 10, 100, 1000]):
    put(wsU, f"{col}5", v, BLUE, NUM, INPUT_FILL)
put(wsU, "G5", "Edit to test other batch sizes; the per-unit prices below don't re-scale by themselves.")
put(wsU, "B6", "Shell used")
for col, v in zip(VOLS, ["Casio donor", "Clone (retail)", "Clone (Alibaba)", "Clone (factory)"]):
    put(wsU, f"{col}6", v, BLUE, None, INPUT_FILL)

put(wsU, "A7", "Per-unit parts (price per unit at that volume)", BOLD)
rows_var = [
    ("PCB + PCBA (board, all SMD parts, JLCPCB assembly)", [72.50, 18, 9, 7.30],
     "2: JLCPCB $145 for 5 PCBs / 2 assembled (Shopping List row 1) ÷ 2. 10/100/1,000: roadmap §3 table [S8]."),
    ("Shell + key mat + window + screws", [21, 6, 3.99, 3.25],
     "2: donor fx-115ES ~$21 incl. tax (already owned; value shown). 10: retail clone incl. shipping. 100: Alibaba $3.25-3.99 MOQ 20 [S1]. 1,000: $3.25 (factories quote $1.50-3.50 at 3,000 [S2])."),
    ("Camera OV5640 AF", [12.99, 12, 6, 4.50], "2: Seeed 114993115 $12.99. Then roadmap / Alibaba OEM [S3]."),
    ("E-paper 2.13\" raw panel", [6.99, 6.60, 5.50, 4.00], "2: Waveshare $6.99. Then Good Display direct by RFQ (estimate)."),
    ("LiPo 150 mAh + JST-PH", [5.95, 5.95, 1.80, 1.30], "2/10: Adafruit #1317. 100/1,000: Alibaba cell with PH lead + PCM (estimate)."),
    ("Battery dangerous-goods freight surcharge", [0, 0, 0.80, 0.30], "UN3481 air surcharge (estimate)."),
    ("Magnetic connector pair", [6.50, 5.85, 1.80, 1.20], "Adafruit #5358 $6.50 / $5.85 (10+); Alibaba MG04-254-RA $1.80 [S7]."),
    ("Magnetic USB cable (in the box)", [4.95, 4.95, 1.80, 1.40], "Adafruit #5412 $4.95; OEM $1.40-1.80 (estimate)."),
    ("Retail box + insert", [0, 1.50, 1.00, 0.60], "Prototypes unboxed. Alibaba rigid box $0.20-1.02 @100 [S9]."),
    ("Quick-start card (with 'not affiliated with Casio')", [0, 0.10, 0.25, 0.05], "[S10]; small runs estimate."),
    ("Serial / QC label + polybag", [0, 0.15, 0.10, 0.05], "Estimate."),
    ("Tape, Kapton, glue (per-unit share)", [0, 0.50, 0.30, 0.20], "2: bought as bench tools (one-time row below). Estimate."),
    ("Inbound freight + US import duty (per unit)", [0, 8, 8, 5], "2: paid as a lump (one-time row below). Roadmap: ~35 % duty, low end of 35-92.5 % [S8]. Biggest swing factor."),
]
r = 8
var_first = r
for i, (lab, vals, src) in enumerate(rows_var, start=1):
    put(wsU, f"A{r}", i)
    put(wsU, f"B{r}", lab)
    for col, v in zip(VOLS, vals):
        put(wsU, f"{col}{r}", v, BLUE, USD, INPUT_FILL)
    put(wsU, f"G{r}", src).alignment = WRAP
    r += 1
var_last = r - 1
put(wsU, f"B{r}", "Parts subtotal", BOLD)
for col in VOLS:
    put(wsU, f"{col}{r}", f"=SUM({col}{var_first}:{col}{var_last})", BLACK, USD).fill = TOT_FILL
sub_row = r
r += 1
put(wsU, f"B{r}", "Scrap + spares allowance, %")
for col, v in zip(VOLS, [0, 0.10, 0.07, 0.05]):
    put(wsU, f"{col}{r}", v, BLUE, PCT, INPUT_FILL)
put(wsU, f"G{r}", "Alibaba sheet row 15 (estimate). At 2 the spares are a one-time row instead.")
scrap_pct = r
r += 1
put(wsU, f"B{r}", "Scrap + spares allowance, $")
for col in VOLS:
    put(wsU, f"{col}{r}", f"={col}{sub_row}*{col}{scrap_pct}", BLACK, USD)
scrap_row = r
r += 1
put(wsU, f"B{r}", "Paid assembly labour (box build)")
for col, v in zip(VOLS, [0, 0, 0, 2.50]):
    put(wsU, f"{col}{r}", v, BLUE, USD, INPUT_FILL)
put(wsU, f"G{r}", "You build 2-100 yourself (time not costed, see row below). 1,000: EMS box build (roadmap §3, estimate).")
lab_row = r
r += 1
put(wsU, f"B{r}", "Variable cost per unit", BOLD)
for col in VOLS:
    put(wsU, f"{col}{r}", f"={col}{sub_row}+{col}{scrap_row}+{col}{lab_row}", BLACK, USD, TOT_FILL, bold=True)
varc_row = r
r += 2

put(wsU, f"A{r}", "One-time costs for the batch (whole $ amount)", BOLD)
r += 1
rows_once = [
    ("Shipping + tariff lump (JLCPCB DHL $25 + tariff $40 + Adafruit $15 + Waveshare $20 + Seeed $12)", [112, 0, 0, 0],
     "Shopping List rows 2, 3, 7, 9, 11. At 10+ freight is per unit (row 13)."),
    ("Bench tools and consumables (tape, glue, drill bit, vinyl, IPA, glasses, charger)", [69, 0, 0, 0], "Shopping List rows 12-19."),
    ("Prototype spares (1 extra camera, battery, magnet pair)", [25.44, 0, 0, 0], "Shopping List: 3 cameras, 3 batteries, 3 connectors for 2 units."),
    ("Supplier samples (2 each from 2 suppliers, incl. 4 knockoff shells) + express", [0, 150, 150, 120], "Alibaba sheet row 16 (estimate); KNOCKOFF_SHELL_PLAN.md §1."),
    ("Pogo test jig + fixtures", [0, 0, 100, 150], "Roadmap §5: $50-150 (estimate)."),
    ("Third-party pre-shipment inspection", [0, 0, 0, 300], "~1 man-day (estimate). Only worth it at 1,000."),
]
once_first = r
for lab, vals, src in rows_once:
    put(wsU, f"B{r}", lab).alignment = WRAP
    for col, v in zip(VOLS, vals):
        put(wsU, f"{col}{r}", v, BLUE, USD, INPUT_FILL)
    put(wsU, f"G{r}", src).alignment = WRAP
    r += 1
once_last = r - 1
put(wsU, f"B{r}", "One-time total for the batch", BOLD)
for col in VOLS:
    put(wsU, f"{col}{r}", f"=SUM({col}{once_first}:{col}{once_last})", BLACK, USD, TOT_FILL)
once_tot = r
r += 1
put(wsU, f"B{r}", "One-time per unit")
for col in VOLS:
    put(wsU, f"{col}{r}", f"={col}{once_tot}/{col}$5", BLACK, USD)
once_pu = r
r += 2

put(wsU, f"B{r}", "COST PER UNIT (before selling-legally costs)", BOLD)
for col in VOLS:
    put(wsU, f"{col}{r}", f"={col}{varc_row}+{col}{once_pu}", BLACK, USD, TOT_FILL, bold=True)
put(wsU, f"G{r}", "Compare: Alibaba_Bulk_Sourcing.xlsx ≈ $92 @10, $46 @100, $34 @1,000 (see 'Checks').")
cpu_row = r
names["cpu"] = {col: ref(U, col, r) for col in VOLS}
r += 1
put(wsU, f"B{r}", "Selling legally? (1 = yes, spread the one-time certification over this batch)")
for col, v in zip(VOLS, [0, 0, 1, 1]):
    put(wsU, f"{col}{r}", v, BLUE, NUM, INPUT_FILL)
put(wsU, f"G{r}", "2 = your own prototypes; 10 = free / at-cost beta (KNOCKOFF_SHELL_PLAN.md §5). Public sales start at 100.")
sell_row = r
r += 1
put(wsU, f"B{r}", "Certification, lawyer, LLC, trademark, insurance per unit")
for col in VOLS:
    put(wsU, f"{col}{r}", f"={col}{sell_row}*{names['cert_total']}/{col}$5", BLACK, USD)
cert_pu = r
r += 1
put(wsU, f"B{r}", "COST PER UNIT incl. selling-legally costs", BOLD)
for col in VOLS:
    put(wsU, f"{col}{r}", f"={col}{cpu_row}+{col}{cert_pu}", BLACK, USD, TOT_FILL, bold=True)
cpu_all = r
names["cpu_all"] = {col: ref(U, col, r) for col in VOLS}
r += 1
put(wsU, f"B{r}", "Cash for the whole batch (incl. one-time)", BOLD)
for col in VOLS:
    put(wsU, f"{col}{r}", f"={col}{cpu_all}*{col}$5", BLACK, USD0)
batch_cash = r
names["batch_cash"] = {col: ref(U, col, r) for col in VOLS}
r += 2

put(wsU, f"A{r}", "Information only (not added above)", BOLD)
r += 1
put(wsU, f"B{r}", "JLCPCB assembly fee inside the PCBA price: (setup + extended fees) ÷ batch + joints × rate")
for col in VOLS:
    put(wsU, f"{col}{r}",
        f"=({names['jlc_setup']}+{names['ext_fee']}*{names['ext_types']})/{col}$5+{names['joints']}*{names['joint_rate']}",
        BLACK, USD)
put(wsU, f"G{r}", "Why the board price drops from 10 to 100: the fixed $67 per order is shared. The joint fee itself is only ~$0.40 a board; the rest of row 1 is parts.")
r += 1
put(wsU, f"B{r}", "Your time per unit, valued (not cash)")
for col in VOLS:
    put(wsU, f"{col}{r}", f"=IF({col}{lab_row}>0,0,{names['own_min']}/60*{names['own_rate']})", BLACK, USD)
put(wsU, f"G{r}", "37 min × $15/h. Zero where paid labour takes over.")
wsU.freeze_panes = "C5"

# ---------------------------------------------------------------- API cost
wsA = wb.create_sheet("API cost")
A = "API cost"
put(wsA, "A1", "Claude API cost per solve and per subscriber per month", TITLE)
put(wsA, "A2", "Chosen model and usage come from 'Inputs'. Batch pricing does not apply to live solves.", NOTE)
header(wsA, 4, ["Item", "Value", "How it's worked out"], [52, 14, 90])
a_rows = [
    ("Model", f"={names['model']}", None, "Inputs"),
    ("Input price $/M tokens", f"=INDEX({MT['in']},MATCH(B5,{MT['name']},0))", USD, "Model table on Inputs"),
    ("Output price $/M tokens", f"=INDEX({MT['out']},MATCH(B5,{MT['name']},0))", USD, "Model table"),
    ("Cache read price $/M tokens", f"=INDEX({MT['cache']},MATCH(B5,{MT['name']},0))", USD, "Model table"),
    ("Output tokens per solve (incl. thinking)", f"=INDEX({MT['otok']},MATCH(B5,{MT['name']},0))", NUM, "Model table"),
    ("Input tokens read from cache", f"={names['cached_tokens']}*{names['cache_hit']}", NUM, "cached prefix × hit rate"),
    ("Input tokens written to cache (misses)", f"={names['cached_tokens']}*(1-{names['cache_hit']})", NUM, "cached prefix × miss rate"),
    ("Input tokens at full price", f"={names['in_tokens']}-B10", NUM, "all input − cache reads (writes are billed at full price plus the write premium)"),
    ("Input cost per solve", f"=(B12*B6+B10*B8+B11*B6*({names['cache_write_mult']}-1))/1000000", '"$"0.0000', "full-price input + cache reads + write premium"),
    ("Output cost per solve", f"=B9*B7/1000000", '"$"0.0000', "output tokens × output price"),
    ("COST PER SOLVE", f"=(B13+B14)*(1-{names['batch_disc']})", '"$"0.0000', "after any batch discount"),
    ("Solves per user per month (typical)", f"={names['solves_day']}*{names['days_month']}", NUM, "solves/day × days"),
    ("API COST PER SUBSCRIBER PER MONTH (typical)", f"=B15*B16", USD, ""),
    ("API cost per month for a user at the fair-use cap", f"=B15*{names['monthly_cap']}", USD, "cap = 500 solves (proxy)"),
    ("Solves per month that $15 can pay for", f"=({names['sub_price']}-({names['sub_price']}*{names['pay_pct']}+{names['pay_fixed']}))/B15", NUM, "($15 − card fee) ÷ cost per solve. Hosting excluded."),
    ("…as solves per day", f"=B19/{names['days_month']}", NUM2, ""),
    ("Cap that keeps a heavy user at break-even (suggested MONTHLY_CAP)", f"=ROUNDDOWN(B19,-1)", NUM, "rounded down to 10"),
]
for i, (lab, f_, fmt, how) in enumerate(a_rows, start=5):
    put(wsA, f"A{i}", lab, BOLD if lab.isupper() or lab.startswith("API COST") or lab.startswith("COST") else BLACK)
    c = put(wsA, f"B{i}", f_, BLACK, fmt)
    if lab.startswith("COST") or lab.startswith("API COST"):
        c.fill = TOT_FILL
        c.font = BOLD
    put(wsA, f"C{i}", how)
names["api_month"] = ref(A, "B", 17)
names["api_solve"] = ref(A, "B", 15)
names["api_cap"] = ref(A, "B", 18)
names["api_be_day"] = ref(A, "B", 20)
names["api_be_cap"] = ref(A, "B", 21)

# scenario table
r = 28
put(wsA, f"A{r}", "Scenarios: API cost per user per month (same cache assumptions)", BOLD)
r += 1
for i, lab in enumerate(["Model", "Cost per solve", "Light 2/day", "Typical (Inputs)", "Heavy 10/day", "At the cap"], start=1):
    c = wsA.cell(row=r, column=i, value=lab)
    c.font = BOLD
    c.fill = HDR_FILL
for col in "DEF":
    wsA.column_dimensions[col].width = 15
wsA.column_dimensions["C"].width = 90
put(wsA, f"G{r}", "Light / heavy solves per day:", BOLD)
put(wsA, f"H{r}", 2, BLUE, NUM, INPUT_FILL)
put(wsA, f"I{r}", 10, BLUE, NUM, INPUT_FILL)
light, heavy = f"$H${r}", f"$I${r}"
for k, m in enumerate(models):
    rr = r + 1 + k
    mrow = mt_first + k
    pin, pout, pc, ot = (f"'{S}'!$B${mrow}", f"'{S}'!$C${mrow}", f"'{S}'!$D${mrow}", f"'{S}'!$E${mrow}")
    cr = f"{names['cached_tokens']}*{names['cache_hit']}"
    cw = f"{names['cached_tokens']}*(1-{names['cache_hit']})"
    put(wsA, f"A{rr}", f"='{S}'!$A${mrow}")
    put(wsA, f"B{rr}", f"=(({names['in_tokens']}-{cr})*{pin}+{cr}*{pc}+{cw}*{pin}*({names['cache_write_mult']}-1)+{ot}*{pout})/1000000*(1-{names['batch_disc']})", BLACK, '"$"0.0000')
    put(wsA, f"C{rr}", f"=B{rr}*{light}*{names['days_month']}", BLACK, USD)
    put(wsA, f"D{rr}", f"=B{rr}*{names['solves_day']}*{names['days_month']}", BLACK, USD)
    put(wsA, f"E{rr}", f"=B{rr}*{heavy}*{names['days_month']}", BLACK, USD)
    put(wsA, f"F{rr}", f"=B{rr}*{names['monthly_cap']}", BLACK, USD)
names["scen_first"] = r + 1
put(wsA, f"A{r+5}", "Column C is narrow-looking because it also holds the notes above; numbers are $ per user per month.", NOTE)

# ---------------------------------------------------------------- Margins & payback
wsM = wb.create_sheet("Margins & payback")
M = "Margins & payback"
put(wsM, "A1", "Margin on the $225 calculator, margin on the $15 subscription, break-even and payback", TITLE)
put(wsM, "A2", "Columns = batch size. Unit cost includes the selling-legally costs where 'Selling legally' = 1 on 'Unit cost'.", NOTE)
header(wsM, 4, ["Item", "2 units", "10 units", "100 units", "1,000 units", "How it's worked out"], [58, 13, 13, 13, 13, 80])
MC = ["B", "C", "D", "E"]
rows_m = []


def mrow(label, fn, fmt=USD, how="", bold=False):
    rows_m.append((label, fn, fmt, how, bold))


mrow("Batch size", lambda uc, i: f"='{U}'!{uc}$5", NUM, "")
mrow("HARDWARE", None)
mrow("Price", lambda uc, i: f"={names['hw_price']}", USD, "Inputs")
mrow("Card fee", lambda uc, i: f"={names['hw_price']}*{names['pay_pct']}+{names['pay_fixed']}", USD, "2.9 % + 30¢")
mrow("Shipping to the customer", lambda uc, i: f"={names['ship_out']}", USD, "Inputs")
mrow("Returns / warranty allowance", lambda uc, i: f"={names['hw_price']}*{names['warranty_pct']}", USD, "% of price")
mrow("API cost of the free first month", lambda uc, i: f"={names['api_month']}*{names['free_months']}", USD, "Every buyer gets it, paying or not")
mrow("Unit cost (incl. selling-legally costs)", lambda uc, i: f"={names['cpu_all'][uc]}", USD, "'Unit cost' sheet")
mrow("HARDWARE PROFIT PER UNIT", lambda uc, i: f"={MC[i]}7-{MC[i]}8-{MC[i]}9-{MC[i]}10-{MC[i]}11-{MC[i]}12", USD, "price − everything above", True)
mrow("Hardware margin %", lambda uc, i: f"={MC[i]}13/{MC[i]}7", PCT, "profit ÷ price")
mrow("SUBSCRIPTION (per paying user per month)", None)
mrow("Price", lambda uc, i: f"={names['sub_price']}", USD, "Inputs")
mrow("Card fee", lambda uc, i: f"={names['sub_price']}*{names['pay_pct']}+{names['pay_fixed']}", USD, "the 30¢ hurts most on small charges")
mrow("Claude API (typical user)", lambda uc, i: f"={names['api_month']}", USD, "'API cost' sheet")
mrow("Hosting share", lambda uc, i: f"={names['server_month']}/MAX(1,{MC[i]}5*{names['attach']})", USD, "server $/month ÷ paying users (batch × attach rate)")
mrow("SUBSCRIPTION PROFIT PER USER PER MONTH", lambda uc, i: f"={MC[i]}16-{MC[i]}17-{MC[i]}18-{MC[i]}19", USD, "", True)
mrow("Subscription margin %", lambda uc, i: f"={MC[i]}20/{MC[i]}16", PCT, "")
mrow("Same, for a user at the 500-solve cap", lambda uc, i: f"={MC[i]}16-{MC[i]}17-{names['api_cap']}-{MC[i]}19", USD, "negative = that user costs you money")
mrow("Does $15 cover the API? (typical user)", lambda uc, i: f'=IF({MC[i]}18<{MC[i]}16-{MC[i]}17,"YES","NO")', None, "")
mrow("LIFETIME (per calculator sold)", None)
mrow("Expected paid months in the horizon", lambda uc, i: f"={names['attach']}*(1-(1-{names['churn']})^({names['horizon']}-{names['free_months']}))/{names['churn']}", NUM2, "attach × (1 − (1−churn)^(months−free)) ÷ churn")
mrow("Subscription profit over the horizon", lambda uc, i: f"={MC[i]}25*{MC[i]}20", USD, "")
mrow("TOTAL PROFIT PER CALCULATOR (hardware + subscription)", lambda uc, i: f"={MC[i]}13+{MC[i]}26", USD, "", True)
mrow("BREAK-EVEN AND PAYBACK", None)
mrow("One-time costs this batch carries (samples, jig, QC, certification)", lambda uc, i: f"='{U}'!{uc}{once_tot}+'{U}'!{uc}{sell_row}*{names['cert_total']}", USD0, "'Unit cost' one-time + certification if selling")
mrow("Variable cost per unit", lambda uc, i: f"='{U}'!{uc}{varc_row}", USD, "")
mrow("Profit per unit before one-time costs", lambda uc, i: f"={MC[i]}7-{MC[i]}8-{MC[i]}9-{MC[i]}10-{MC[i]}11-{MC[i]}30", USD, "")
mrow("Units to sell to cover the one-time costs (hardware only)", lambda uc, i: f"=IF({MC[i]}31>0,ROUNDUP({MC[i]}29/{MC[i]}31,0),\"never\")", NUM, "one-time ÷ profit per unit")
mrow("Months for one paying user to repay the free month's API", lambda uc, i: f"=IF({MC[i]}20>0,{MC[i]}11/{MC[i]}20,\"never\")", NUM2, "free-month API ÷ subscription profit")
mrow("Months of subscription that would repay the unit cost alone", lambda uc, i: f"=IF({MC[i]}20>0,{MC[i]}12/{MC[i]}20,\"never\")", NUM2, "if the calculator were sold at cost")
mrow("Calculators to sell to get the $2,000 budget back", lambda uc, i: f"=IF({MC[i]}13>0,ROUNDUP({names['budget']}/{MC[i]}13,0),\"never\")", NUM, "$2,000 ÷ hardware profit per unit")

for i, (lab, fn, fmt, how, bold) in enumerate(rows_m, start=5):
    if fn is None:
        put(wsM, f"A{i}", lab, BOLD)
        for col in MC:
            wsM[f"{col}{i}"].fill = HDR_FILL
        wsM[f"A{i}"].fill = HDR_FILL
        continue
    put(wsM, f"A{i}", lab, BOLD if bold else BLACK)
    for k, (col, uc) in enumerate(zip(MC, VOLS)):
        c = put(wsM, f"{col}{i}", fn(uc, k), BLACK, fmt)
        if bold:
            c.font = BOLD
            c.fill = TOT_FILL
    put(wsM, f"F{i}", how)
# sanity: verify row labels align with the fixed row numbers used in formulas
expect = {7: "Price", 13: "HARDWARE PROFIT", 16: "Price", 20: "SUBSCRIPTION PROFIT", 25: "Expected paid", 26: "Subscription profit", 29: "One-time", 31: "Profit per unit"}
for rr, txt in expect.items():
    assert str(wsM[f"A{rr}"].value).startswith(txt), (rr, wsM[f"A{rr}"].value)
wsM.freeze_panes = "B5"

# ---------------------------------------------------------------- Cash
wsC = wb.create_sheet("Cash budget")
C = "Cash budget"
put(wsC, "A1", "Where the $2,000 goes (supply budget), in order", TITLE)
put(wsC, "A2", "Cash actually leaving your account. The Casio shells you own are not cash. Nothing is ordered as of 2026-10-06.", NOTE)
header(wsC, 4, ["#", "What", "When", "Amount", "Source / notes"], [5, 62, 18, 13, 95])
cash = [
    ("Prototype build: JLCPCB v14 (5 PCBs, 2 assembled) + Adafruit + Waveshare + Seeed + bench tools", "Now (after dry fit)", 426.20, True,
     "AI_Calculator_Shopping_List.xlsx total (=SUM(H5:H23) = $426.20)."),
    ("Knockoff shell samples: 4 clones, 2 sellers (KNOCKOFF_SHELL_PLAN.md §1)", "This week", 60, True,
     "Alibaba pair $40-60 incl. DHL + AliExpress/Amazon pair $10-16 (estimate)."),
    ("Other supplier samples for the 100 batch (camera, e-paper, battery, magnet pair + cable)", "After shells pass", 90, True,
     "Alibaba sheet row 16 total $150 minus the shells (estimate)."),
    ("10-unit pilot on the v15 board (all parts, freight, scrap; samples counted above)", "After go / v15", f"='{U}'!D{varc_row}*'{U}'!D5", False,
     "Variable cost at 10 × 10 ('Unit cost' column 10). Includes 10 assembled v15 boards (≈ $18 each) + $8/unit freight and duty."),
    ("Proxy hosting, first 3 months", "At beta", f"=3*{names['server_month']}", False, "Inputs: server $/month × 3."),
    ("Claude API for the beta: 12 users' free month", "At beta", f"=12*{names['api_month']}", False, "2 prototypes + 10 pilot units × 1 month at the typical rate."),
    ("Lawyer / IP clinic: trade-dress check before any unit leaves the house", "Before beta", 150, True,
     "KNOCKOFF_SHELL_PLAN.md §5. University clinic / SCORE can be $0; private $200-500."),
    ("Indiana LLC", "Before taking money", f"={names['llc']}", False, "Inputs (roadmap §4)."),
    ("Domain + email for the link page", "Before beta", 20, True, "Estimate."),
]
r = 5
for i, (what, when, amt, is_input, src) in enumerate(cash, start=1):
    put(wsC, f"A{r}", i)
    put(wsC, f"B{r}", what).alignment = WRAP
    put(wsC, f"C{r}", when)
    if is_input:
        put(wsC, f"D{r}", amt, BLUE, USD, INPUT_FILL)
    else:
        put(wsC, f"D{r}", amt, BLACK, USD)
    put(wsC, f"E{r}", src).alignment = WRAP
    r += 1
c_last = r - 1
put(wsC, f"B{r}", "Subtotal", BOLD)
put(wsC, f"D{r}", f"=SUM(D5:D{c_last})", BLACK, USD, TOT_FILL)
c_sub = r
r += 1
put(wsC, f"B{r}", "Contingency, % of subtotal (re-orders, a dead camera, express shipping)")
put(wsC, f"C{r}", 0.10, BLUE, PCT, INPUT_FILL)
put(wsC, f"D{r}", f"=D{c_sub}*C{r}", BLACK, USD)
c_cont = r
r += 1
put(wsC, f"B{r}", "TOTAL PLANNED SPEND", BOLD)
put(wsC, f"D{r}", f"=D{c_sub}+D{c_cont}", BLACK, USD, TOT_FILL, bold=True)
c_tot = r
r += 1
put(wsC, f"B{r}", "Budget")
put(wsC, f"D{r}", f"={names['budget']}", BLACK, USD)
r += 1
put(wsC, f"B{r}", "LEFT OVER (negative = over budget)", BOLD)
put(wsC, f"D{r}", f"={names['budget']}-D{c_tot}", BLACK, USD, TOT_FILL, bold=True)
c_left = r
names["cash_total"] = ref(C, "D", c_tot)
names["cash_left"] = ref(C, "D", c_left)
r += 2
put(wsC, f"A{r}", "Not inside the $2,000: what the first 100 public units need", BOLD)
r += 1
put(wsC, f"B{r}", "100-unit batch: variable cost × 100 + its one-time costs (samples, jig)")
put(wsC, f"D{r}", f"='{U}'!E{varc_row}*'{U}'!E5+'{U}'!E{once_tot}", BLACK, USD0)
n_batch = r
r += 1
put(wsC, f"B{r}", "Selling legally: FCC SDoC, lawyer, LLC, trademark, insurance")
put(wsC, f"D{r}", f"={names['cert_total']}", BLACK, USD0)
n_cert = r
r += 1
put(wsC, f"B{r}", "Cash needed before the first public sale", BOLD)
put(wsC, f"D{r}", f"=D{n_batch}+D{n_cert}", BLACK, USD0, TOT_FILL, bold=True)
n_need = r
names["need100"] = ref(C, "D", r)
r += 1
put(wsC, f"B{r}", "Pre-orders at $225 (after card fees) that fund it")
put(wsC, f"D{r}", f"=ROUNDUP((D{n_need}-MAX(0,D{c_left}))/({names['hw_price']}*(1-{names['pay_pct']})-{names['pay_fixed']}),0)", BLACK, NUM)
names["preorders"] = ref(C, "D", r)
put(wsC, f"E{r}", "Uses any money left from the $2,000 first. Roadmap §6: take pre-orders before each step up.")

# ---------------------------------------------------------------- Checks
wsK = wb.create_sheet("Checks")
K = "Checks"
put(wsK, "A1", "Cross-checks against earlier files", TITLE)
header(wsK, 3, ["Check", "This model", "Earlier figure", "Difference", "Earlier source"], [60, 13, 13, 13, 70])
checks = [
    ("Cost per unit at 10 (before selling-legally)", names["cpu"]["D"], 92, "Alibaba_Bulk_Sourcing.xlsx 'Cost by batch' (≈ $92)"),
    ("Cost per unit at 100 (before selling-legally)", names["cpu"]["E"], 46, "Alibaba_Bulk_Sourcing.xlsx (≈ $46)"),
    ("Cost per unit at 1,000 (before selling-legally)", names["cpu"]["F"], 34, "Alibaba_Bulk_Sourcing.xlsx (≈ $34)"),
    ("Opus 5.5 cost per solve, no caching (roadmap §3.3)", f"(({names['in_tokens']})*4+2100*20)/1000000", 0.052, "LAUNCH_ROADMAP.md §3.3 (~$0.052)"),
    ("Certification etc. one-time", names["cert_total"], 3500, "LAUNCH_ROADMAP.md §3 ('about $3.5k')"),
]
for i, (lab, f_, earlier, src) in enumerate(checks, start=4):
    put(wsK, f"A{i}", lab)
    put(wsK, f"B{i}", f"={f_}", BLACK, USD if earlier > 1 else '"$"0.0000')
    put(wsK, f"C{i}", earlier, BLUE, USD if earlier > 1 else '"$"0.0000', INPUT_FILL)
    put(wsK, f"D{i}", f"=B{i}-C{i}", BLACK, USD if earlier > 1 else '"$"0.0000')
    put(wsK, f"E{i}", src)
put(wsK, "A10", "Differences come from the 2 prototype-only rows, rounding in the earlier sheet, and the cache saving on the system prompt.", NOTE)

# ---------------------------------------------------------------- Summary (first)
wsS = wb.create_sheet("Summary", 0)
put(wsS, "A1", "AI Calculator cost model: headline numbers", TITLE)
put(wsS, "A2", "All values are formulas pointing at the other sheets. Edit blue cells on 'Inputs', 'Unit cost' and 'Cash budget'.", NOTE)
header(wsS, 4, ["", "2 units", "10 units", "100 units", "1,000 units"], [62, 14, 14, 14, 14])
summ = [
    ("Cost per unit (parts, freight, scrap, batch one-time)", lambda uc, mc: f"={names['cpu'][uc]}", USD),
    ("Cost per unit incl. FCC / lawyer / LLC / trademark / insurance", lambda uc, mc: f"={names['cpu_all'][uc]}", USD),
    ("Hardware profit per $225 unit", lambda uc, mc: f"='{M}'!{mc}13", USD),
    ("Hardware margin %", lambda uc, mc: f"='{M}'!{mc}14", PCT),
    ("Subscription profit per paying user per month", lambda uc, mc: f"='{M}'!{mc}20", USD),
    ("Subscription margin %", lambda uc, mc: f"='{M}'!{mc}21", PCT),
    ("Total profit per calculator (hardware + 12 months' subscription)", lambda uc, mc: f"='{M}'!{mc}27", USD),
    ("Units to cover this batch's one-time costs", lambda uc, mc: f"='{M}'!{mc}32", NUM),
]
for i, (lab, fn, fmt) in enumerate(summ, start=5):
    put(wsS, f"A{i}", lab)
    for uc, mc, col in zip(VOLS, MC, ["B", "C", "D", "E"]):
        put(wsS, f"{col}{i}", fn(uc, mc), BLACK, fmt)
r = 5 + len(summ) + 1
put(wsS, f"A{r}", "Claude API", BOLD)
singles = [
    ("Model", f"={names['model']}", None),
    ("Cost per solve", f"={names['api_solve']}", '"$"0.0000'),
    ("API cost per subscriber per month (typical use)", f"={names['api_month']}", USD),
    ("API cost per month for a user at the 500 cap", f"={names['api_cap']}", USD),
    ("Solves per day that $15 pays for (break-even)", f"={names['api_be_day']}", NUM2),
    ("Suggested fair-use cap (solves/month) for break-even", f"={names['api_be_cap']}", NUM),
    ("Same API cost per month on Sonnet 5.5 (typical)", f"='{A}'!D{names['scen_first']+1}", USD),
]
for lab, f_, fmt in singles:
    r += 1
    put(wsS, f"A{r}", lab)
    put(wsS, f"B{r}", f_, BLACK, fmt)
r += 2
put(wsS, f"A{r}", "The $2,000", BOLD)
for lab, f_, fmt in [("Planned spend (prototypes → 10-unit pilot)", f"={names['cash_total']}", USD),
                     ("Left over", f"={names['cash_left']}", USD),
                     ("Cash needed before the first 100 public units", f"={names['need100']}", USD0),
                     ("Pre-orders at $225 that would fund it", f"={names['preorders']}", NUM)]:
    r += 1
    put(wsS, f"A{r}", lab)
    put(wsS, f"B{r}", f_, BLACK, fmt)

for ws in wb.worksheets:
    ws.sheet_view.showGridLines = True
    for row in ws.iter_rows():
        for c in row:
            if c.font is None or c.font.name != F:
                c.font = Font(name=F, size=c.font.size if c.font else 10, bold=c.font.bold if c.font else False,
                              italic=c.font.italic if c.font else False, color=c.font.color if c.font else None)

wb.save(OUT)
print("saved", OUT)
