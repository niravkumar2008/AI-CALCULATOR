# Shipping setup (US, from Indiana)

Written 2026-11-18. Prices are estimates; check them in Pirate Ship with your real box and weight.

## Read this first: Thanksgiving and Christmas
The site promises "Made to order: ships in 3 to 5 weeks". An order placed on Black Friday (Nov 27) therefore ships
between about Dec 18 and Jan 1, so **most holiday buyers would not get it by Christmas.** Pick one before the push:
- **Build stock first** (for example 20 finished units), then change the ship note to "Ships in 2 business days".
- **Or keep made-to-order and say it plainly** on the product page and in the ads: "Ships late December, after Christmas."

The FTC Mail Order Rule says you must ship within the time you state. If you can't, you must e-mail the customer
before that date with the new date and offer a full refund. Business HQ marks orders at 21+ days as urgent so you
see them in time.

## What to buy (about $60 to start)
| Item | Why | About |
|---|---|---|
| Shipping scale (to 50 lb, 0.1 oz steps) | the label price depends on weight | $20 |
| Boxes: 8 × 6 × 4 in, double-wall | fits the calculator + padding + insert card | $1 each (Uline/Amazon, packs of 25) |
| Kraft paper or bubble wrap | nothing should move when you shake the box | $15 |
| Packing tape + dispenser | | $10 |
| "Fragile" sticker (optional) | | $5 |
| Thermal label printer (later) | 4×6 sticky labels; until then print on paper and tape it on | $150 |

Weigh one fully packed box. Most likely it's under 1 lb, which is the cheapest USPS tier.

## Labels: Pirate Ship (free)
1. Make a free account at pirateship.com. You get USPS and UPS commercial prices with no monthly fee.
2. Settings → **Ship From**: your address. If you don't want your home address on boxes, use a PO box, but returns go
   there too.
3. Settings → **Packages**: save "Proofline box" with the size and weight you measured.
4. Pay by card or with a balance you top up.

Rough price for a 1 lb box sent from Indiana (Pirate Ship shows the exact price):
- **USPS Ground Advantage** (2–5 days): about $5–8
- **USPS Priority Mail** (1–3 days): about $9–14

That's why checkout has Standard $8.95 and Priority $14.95: they cover the label and the box. If the real prices come
in very different, change the rates in Stripe (Product catalog → Shipping rates), not in the code.

## The calculator has a lithium battery
- A lithium-ion battery **installed in the device** (not loose) may be mailed in the US by ground and air.
- Small packages with at most 2 batteries installed in equipment don't need the lithium battery mark.
- When the label tool asks whether the package contains hazardous materials or batteries, answer truthfully.
- Ask your battery supplier for the **UN38.3 test summary** for the exact battery you use, and keep the PDF. Carriers
  and marketplaces can ask for it, and a battery without UN38.3 testing must not be shipped.
- Never ship a spare or loose battery, or a swollen or damaged one.
- Check USPS Publication 52 (section 349 and Packaging Instruction 9D) once yourself. These rules changed in 2025.

## Each order, step by step (5 minutes)
1. **Business HQ → Orders** shows paid orders, oldest first, with name and address.
2. Pack it: calculator in its sleeve, padding all round, the quick-start card on top. Shake test: nothing moves.
3. In Pirate Ship → **Create a label** → paste the name and address → choose your saved box → pick the service the
   customer paid for (Standard = Ground Advantage, Priority = Priority Mail) → buy and print the label.
4. Copy the tracking number. In Business HQ, choose **USPS**, paste it, and press **Mark shipped & e-mail tracking**.
   The customer gets "Your Proofline calculator has shipped" with the tracking link.
5. Drop boxes at the post office, or book a free pickup at usps.com → **Schedule a Pickup**.
6. Pirate Ship also sends its own tracking e-mails if you add the customer's e-mail to the label (optional).

For several orders at once, Pirate Ship → **Import spreadsheet**. A CSV export from Business HQ isn't built yet; ask
Claude if you want it.

## Returns and refunds (the draft policy on the site)
- Cancel before it ships: refund in full in the Stripe Dashboard (Payments → the payment → Refund). The store frees
  the unit by itself.
- 30-day returns: make the customer a return label in Pirate Ship (**Create a return label**), e-mail it (Penny can
  draft that), and refund once the box arrives.
- Warranty (1 year): replace or repair. Keep 1–2 spare units for this.

## Before the first real order
- [ ] Pirate Ship account, Ship From address, saved box
- [ ] Weigh a packed box; check the real price for Standard and Priority to a far-away state (e.g. California)
- [ ] UN38.3 summary from the battery supplier saved
- [ ] Decide on the holiday ship date (see the top of this guide)
- [ ] Send a test order to yourself and run the whole loop once
