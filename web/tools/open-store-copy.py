"""One-off copy change (2026-11-18): $225 -> $215 and "preorders planned for 2027" -> an open store.

Run from the repo root: python web/tools/open-store-copy.py
Safe to run twice (it only replaces text that is still there). Prints what it changed.
"""
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]  # web/
SITE = ROOT / "site"
ORDER_LINK = '<a href="/shop/calculator/">Order the calculator</a> ($215), or get an email about new batches and updates.'
SHIP_ANSWER = ("Orders are open for our first small batch. Each calculator is hand-built after you order; "
               "the ship time is shown on the product page and again before you pay. We email you tracking when it ships.")

# Applied to every page and partial, in this order.
GLOBAL = [
    # first-year sums before the plain price
    ("$345", "$335"),
    ("$390", "$380"),
    ("$225", "$215"),
    ('"price": "225.00"', '"price": "215.00"'),
    ("In development; preorders planned for 2027.", "Hand-built in small batches. Ships to US addresses."),
    ("Get an email when preorders open", "Get an email about new batches"),
    ('No payment now. <a href="/privacy/">Privacy policy</a>', '<a href="/privacy/">Privacy policy</a>'),
    ("We’ll email you when preorders open. They’re planned for 2027. No payment now.", ORDER_LINK),
    ("We’ll email you when preorders open. No payment now.", ORDER_LINK),
    ("Join the waitlist and we’ll email you when preorders open. No payment now.", ORDER_LINK),
    ("Not yet. Our first prototype boards are ordered. We’re building 5 samples first, then moving to production; preorders are planned for 2027. Join the waitlist to hear first.", SHIP_ANSWER),
    ("Not yet. Our first prototype boards are ordered. We’re building 5 samples first, then moving to production; preorders are planned for 2027. We’re not taking payment yet. Orders will ship to US addresses only.", SHIP_ANSWER + " US addresses only."),
    ("Not yet. Our first prototype boards are ordered. We’re building 5 samples first, then moving to production; preorders are planned for 2027.", SHIP_ANSWER),
    ("We’re not taking payment yet. Preorders are planned for 2027.", "Plus tax and shipping. Made to order in small batches; you see the ship time before you pay."),
    ("They may change before preorders open.", "Later batches may change."),
    ("they may change before preorders open.", "later batches may change."),
    ("details may change before preorders open.", "later batches may change."),
]

PAGES = {
    "index.html": [
        ('<p class="hero__status" data-hero="0">In development · Preorders planned for 2027</p>',
         '<p class="hero__status" data-hero="0">Hand-built in small batches · $215</p>'),
        ('<form class="waitlist" id="join" action="/api/waitlist" method="post" data-hero="3" data-source="hero">',
         '<p class="hero__buy" data-hero="3"><a class="btn btn--primary" href="/shop/calculator/">Order yours: $215</a></p>\n'
         '          <form class="waitlist" id="join" action="/api/waitlist" method="post" data-hero="3" data-source="hero">'),
        ('<label class="waitlist__label" for="email-hero">Get an email about new batches</label>',
         '<label class="waitlist__label" for="email-hero">Not ready yet? Get an email about new batches</label>'),
        ("<p>Preorders are planned for 2027. AI Solve and Type it need the",
         "<p>Hand-built after you order, in small batches. Plus tax and shipping. AI Solve and Type it need the"),
        ('<a class="btn btn--primary" href="#join" data-focus-email>Join the waitlist</a>\n          <p class="price-card__fine">',
         '<a class="btn btn--primary" href="/shop/calculator/">Order now</a>\n          <p class="price-card__fine">'),
        ("<p>Not yet. Our first prototype boards are ordered. We’re building 5 samples first, then moving to production; preorders are planned for 2027. <a href=\"#join\" data-focus-email>Join the waitlist</a> to hear first.</p>",
         "<p>" + SHIP_ANSWER + " <a href=\"/shop/calculator/\">Order yours</a>.</p>"),
        ('<p class="closing__lede">We’ll email you when preorders open. Nothing else.</p>',
         '<p class="closing__lede"><a href="/shop/calculator/">Order the calculator</a>, or get an email about new batches and updates. Nothing else.</p>'),
    ],
    "pricing/index.html": [
        ("We’re not taking payment yet; preorders are planned for 2027.", "Tax and shipping are added at checkout."),
        ('<a class="btn btn--primary" href="#email-pricing" data-focus-email>Join the waitlist</a>',
         '<a class="btn btn--primary" href="/shop/calculator/">Order now</a>'),
        ("<p>The calculator is $215. Preorders are planned for 2027.</p>", "<p>The calculator is $215, plus tax and shipping.</p>"),
        ("The calculator is $215. AI Solve and Type it need the AI tutor plan",
         "The calculator is $215, plus tax and shipping. AI Solve and Type it need the AI tutor plan"),
    ],
    "shop/index.html": [
        ("Preorders are planned for 2027.\">", "Made to order in small batches.\">"),
        ('<span class="badge">Preorders planned for 2027</span>', '<span class="badge">Made to order</span>'),
        ('<h2 id="waitlist-title">Not on sale yet.</h2>', '<h2 id="waitlist-title">Not ready to order?</h2>'),
        ("Preorders are planned for 2027. We aren’t taking payment yet. Join the waitlist and we’ll email you when preorders open.",
         "Get an email when a new batch opens, plus product updates. Nothing else."),
    ],
    "cart/index.html": [
        ('content="Your Proofline cart. Preorders are planned for 2027; until then, join the waitlist.">',
         'content="Your Proofline cart. The calculator is $215, plus tax and shipping.">'),
        ("<p>The cart needs JavaScript turned on. Preorders open in 2027; until then, <a href=\"#email-sitefoot\">join the waitlist</a> to hear first.</p>",
         "<p>The cart needs JavaScript turned on. Turn it on to check out, or email <a href=\"mailto:hello@proofline.example\">hello@proofline.example</a> to order.</p>"),
        ('<p class="cart-line__unit">$215 each · Preorder, ships in 2027</p>',
         '<p class="cart-line__unit">$215 each · <span data-ship-note>Made to order</span></p>'),
        ("<strong>This is a preorder.</strong> Calculators ship in 2027. We’ll email you a ship date.",
         "<strong>Hand-built for you.</strong> <span data-ship-note>We build each calculator after you order.</span> Tax and shipping are added at secure checkout."),
        ("<strong>Preorders open in 2027.</strong> We aren’t taking payment yet. Join the waitlist to hear first.",
         "<strong>This batch is sold out.</strong> Join the waitlist and we’ll email you when the next one opens."),
        ("Get an email about new batches</label>", "Get an email when the next batch opens</label>"),
        ("<p>The calculator is $215.<span data-store-closed> Preorders are planned for 2027.</span></p>",
         "<p>The calculator is $215, plus tax and shipping.</p>"),
    ],
    "shop/calculator/index.html": [
        ("$215. Preorders are planned for 2027.\">", "$215. Made to order in small batches.\">"),
        ('"availability": "https://schema.org/PreOrder"', '"availability": "https://schema.org/MadeToOrder"'),
        ('<span class="badge" data-store-closed>Preorders planned for 2027</span>',
         '<span class="badge" data-store-closed>This batch is sold out</span>'),
        ('<span class="badge" data-store-open hidden>Preorder: ships in 2027</span>',
         '<span class="badge" data-store-open hidden data-ship-note>Made to order</span>'),
        ('<span class="price__unit">one-time, US dollars</span>', '<span class="price__unit">one-time, plus tax and shipping</span>'),
        ('<button class="btn btn--primary" type="button" data-preorder data-store-open hidden>Preorder</button>',
         '<button class="btn btn--primary" type="button" data-preorder data-store-open hidden>Buy now</button>'),
        ('<p class="shop-note" data-store-closed>Preorders open in 2027. Join the waitlist to hear first.</p>',
         '<p class="shop-note" data-store-closed>This batch is sold out. Join the waitlist to hear when the next one opens.</p>\n'
         '          <p class="shop-note" data-stock-left hidden></p>'),
        ("<p>The terms will be published before preorders open. <a href=\"/refunds/\">Refunds and warranty</a></p>",
         "<p>Cancel before it ships for a full refund. 30-day returns and a 1-year warranty. <a href=\"/refunds/\">Refunds and warranty</a></p>"),
        ('<h2 id="waitlist-title">Hear first when preorders open.</h2>', '<h2 id="waitlist-title">Hear when the next batch opens.</h2>'),
        ("Preorders are planned for 2027. We aren’t taking payment yet.</p>",
         "This batch is sold out. Leave your email and we’ll tell you when the next one opens.</p>"),
        ('<label class="waitlist__label" for="email-pdp">Get an email about new batches</label>',
         '<label class="waitlist__label" for="email-pdp">Get an email when the next batch opens</label>'),
    ],
    "order/complete/index.html": [
        ("Your preorder is in. Here’s what happens next.", "Your order is in. Here’s what happens next."),
        ("<div><h3>We ship it</h3><p>Preorders ship in 2027. We’ll email you a ship date. It goes to the US address you entered at checkout.</p></div>",
         "<div><h3>We build and ship it</h3><p>We hand-build your calculator, test it, and ship it to the US address you entered at checkout. <span data-ship-note>The ship time was shown at checkout.</span> We email you tracking when it ships.</p></div>"),
    ],
    "faq/index.html": [
        ('"name": "Can I preorder now?"', '"name": "Can I order now?"'),
        ('"text": "Not yet. We’re not taking payment. Join the waitlist and we’ll email you when preorders open."',
         '"text": "Yes. Order on the calculator page and pay at secure checkout: $215, plus tax and shipping. Up to 3 per order."'),
        ('"text": "US addresses only, when orders open."', '"text": "US addresses only, for now."'),
        ('"text": "Refund, return and warranty terms aren’t decided yet. They’ll be published before preorders open, on the refunds and warranty page."',
         '"text": "Cancel any time before it ships for a full refund. Return it within 30 days of delivery for a refund of the calculator price. It has a 1-year warranty against defects. Details are on the refunds and warranty page."'),
        ("Can I preorder now?<span", "Can I order now?<span"),
        ("<p>Not yet. We’re not taking payment. <a href=\"#join\" data-focus-email>Join the waitlist</a> and we’ll email you when preorders open.</p>",
         "<p>Yes. <a href=\"/shop/calculator/\">Order on the calculator page</a> and pay at secure checkout: $215, plus tax and shipping. Up to 3 per order.</p>"),
        ("<p>US addresses only, when orders open.</p>", "<p>US addresses only, for now.</p>"),
        ("<p>Refund, return and warranty terms aren’t decided yet. They’ll be published before preorders open, on the <a href=\"/refunds/\">refunds and warranty page</a>.</p>",
         "<p>Cancel any time before it ships for a full refund. Return it within 30 days of delivery for a refund of the calculator price. It has a 1-year warranty against defects. Details are on the <a href=\"/refunds/\">refunds and warranty page</a>.</p>"),
        ("Shipping and preorders", "Shipping and orders"),
        ('<h2 id="cta-title">Hear when preorders open.</h2>', '<h2 id="cta-title">Ready when you are.</h2>'),
        ("Straight answers about exams, the AI tutor plan, setup and Wi-Fi, AI Solve, privacy, the hardware and preorders.",
         "Straight answers about exams, the AI tutor plan, setup and Wi-Fi, AI Solve, privacy, the hardware and orders."),
    ],
    "support/index.html": [
        ("Exams, the AI tutor plan, privacy, the hardware and preorders.", "Exams, the AI tutor plan, privacy, the hardware and orders."),
    ],
    "refunds/index.html": [
        ("<p class=\"meta\">Template, last edited 10 October 2026</p>", "<p class=\"meta\">Draft, last edited 18 November 2026</p>"),
        ("<h2>Before preorders</h2>\n      <p>We’re not taking payment yet. Joining the waitlist is free and commits you to nothing.</p>",
         "<h2>Cancelling an order</h2>\n      <p>Each calculator is built after you order. You can cancel any time before it ships for a full refund, including tax and shipping: email us your order number.</p>"),
        ("<p>You can return the calculator within <span class=\"todo\">N days</span> of delivery for a refund of the device price. <span class=\"todo\">Who pays return shipping; condition requirements.</span></p>",
         "<p>You can return the calculator within 30 days of delivery for a refund of the calculator price and its tax. Email us first and we’ll send return instructions. If it arrived faulty or damaged, we pay return shipping; otherwise you do. It should come back with its parts and without damage beyond normal use.</p>"),
        ("<li>Cancel anytime from the account page. <span class=\"todo\">Refund rule for a partly used month.</span></li>",
         "<li>Cancel anytime from the account page. The plan stays on until the end of the period you paid for; partly used periods aren’t refunded.</li>"),
        ("for <span class=\"todo\">N months</span> from delivery. We’ll repair or replace it. <span class=\"todo\">Exclusions, for example drops, liquid, or opening the case.</span></p>",
         "for 1 year from delivery. We’ll repair or replace it. Not covered: drops, liquid, or damage from opening the case.</p>"),
        ("<p>Battery: <span class=\"todo\">whether the 150&nbsp;mAh battery is covered, and for how long</span>.</p>",
         "<p>Battery: covered for the same 1 year against defects. Normal battery wear over time isn’t a defect.</p>"),
    ],
}


def patch(path: pathlib.Path, pairs):
    text = path.read_text(encoding="utf-8")
    before = text
    for a, b in pairs:
        if a in text and b not in text:  # b already there = applied on an earlier run
            text = text.replace(a, b)
    if text != before:
        path.write_text(text, encoding="utf-8", newline="")
        return True
    return False


def main():
    changed = []
    for rel, pairs in PAGES.items():
        if patch(SITE / rel, pairs):
            changed.append(rel)
    files = list(SITE.rglob("*.html")) + list((ROOT / "site-src" / "partials").glob("*.html"))
    for f in files:
        if patch(f, GLOBAL):
            changed.append(str(f.relative_to(ROOT)))
    print(f"{len(set(changed))} files changed")
    leftovers = [str(f.relative_to(ROOT)) for f in files if "2027" in f.read_text(encoding="utf-8") or "$225" in f.read_text(encoding="utf-8")]
    if leftovers:
        print("still mention 2027 or $225:", *leftovers, sep="\n  ")
    return 0


if __name__ == "__main__":
    sys.exit(main())
