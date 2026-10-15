# Subscriptions and the online store: the plan, step by step

Written 2026-10-11 for Nirav. This is the "how we're going to do it" guide for selling the calculator and the
AI tutor plan. It goes from where things stand today to taking real money, in the order you'd actually do it.
Every step that costs money, creates an account, or publishes something is marked **(you)**: those are
yours to do, never Claude's.

Not legal or tax advice. Where the law matters, the guide says "ask the lawyer".

---

## 0. Where things stand

| Piece | Status |
|---|---|
| Server (Cloudflare Worker): calculators sign up, link, solve with a spending cap | Done (stage 1), tested |
| Payments: subscription checkout, billing page, calculator checkout, signed Stripe webhooks, account deletion that stops billing | **Done (stage 3), tested, security-reviewed** |
| Waitlist sign-ups from the website (`POST /api/waitlist`) | Done |
| Website (29 pages): home, shop, product pages, pricing, specs, features (incl. `/features/apps/`), FAQ, support, teachers, legal templates | Done, checked by `node web/tools/check-site.mjs` |
| Account pages: sign in, link a calculator, your account, subscribe, manage billing, delete | Done |
| Everything running together on your computer with a fake Stripe (`node web/dev/run-local.mjs`) | Done |
| Calculator firmware talking to this server (sign-up, the link screen under MODE 4 → Settings → Link calculator, auto-update) | **Built, not yet on a real board** (stage 2 done 2026-10-12 on branch `claude/v15-mode4-menu` and tested end to end against the local stack; the firmware still defaults to the old proxy until switched with `api worker <url>`. The v15 colour-LCD boards are ordered: 5 samples first, then bulk; preorders planned for 2027) |
| Stripe account, Cloudflare account, domain, lawyer review | **Not yet (you)** |

Nothing has been deployed, nothing costs money, and nothing is committed: you review and commit.

---

## 1. How the money flows (in plain words)

```
 Customer on the website ──"Subscribe $10/mo or $100/yr"──▶ Stripe's checkout page (card details go to Stripe only)
                                                         │
                                                         │ payment succeeds
                                                         ▼
 Our server ◀──── signed message from Stripe ("subscription is active") ───┘
     │   checks the signature, re-reads the subscription from Stripe, saves "active"
     ▼
 The calculator asks "can I solve?" ──▶ server says yes ──▶ AI Solve works
```

- **Card numbers never touch our code.** Stripe hosts the checkout page and the "Manage billing" page.
- **The website never decides who paid.** Only Stripe's signed messages (webhooks) change a plan, so nobody
  can unlock AI Solve by faking a "thank you" page.
- **Customers see one simple thing:** the calculator ($225) and the AI tutor plan ($10/month or $100/year,
  first month free; updates come with the plan). The site never mentions which AI or payment companies are behind it. (The privacy policy still has
  to list who processes photos and payments: that's a legal requirement, and it's the only place it appears.)

### The rules the server follows

| Rule | What it means for a customer |
|---|---|
| First month free starts when the calculator is first linked, no card | Try AI Solve for 30 days; nothing to cancel if they don't want it |
| One free month per calculator and per e-mail, only for calculators we enrolled at QA | Fakes and resellers can't farm free months |
| $10/month or $100/year (2 months free), up to 300 solves and $5 of AI cost per account per month | Fair use; resets on the 1st |
| Firmware updates only with the plan | Without it the calculator still works fully offline, on the version it has |
| Payment fails → 3 days of grace, counted from the failed payment | "Payment problem" on the account page, then AI stops until the card is fixed |
| Cancel anytime from the account page (Stripe's billing page) | Cancels at the end of the paid month |
| Delete account → Stripe customer deleted first | Billing can't continue after an account is gone |
| $50 a day total AI spending cap | Protects you from a runaway bill |

---

## 2. Try the whole thing on your computer (free, no accounts)

1. In a terminal at the repo: `cd web/api` then `npm install` (once).
2. Back at the repo root: `node web/dev/run-local.mjs`
   (or in the Claude desktop app's browser pane, start the "local-all" server).
3. Open **http://127.0.0.1:8787**. This is the real site and the real server, with:
   - a fake Stripe (its checkout says "Local test only. No money moves"),
   - a fake AI that always answers with the same worked example,
   - sign-in e-mails shown by a "Local test only: open the sign-in e-mail" button on the login page.
4. Walk through it like a customer:
   1. Shop → The calculator → Add to cart → Cart → Checkout → fake Stripe "Pay with test card" → Order complete.
   2. In a second terminal: `node web/dev/calculator.mjs new`. It prints a code like `ABCD-EFGH`.
   3. Sign in at `/login/` with any e-mail, open `/link/`, type the code. You now have the free month.
   4. `node web/dev/calculator.mjs status` (free month), then `node web/dev/calculator.mjs solve` (an answer).
   5. On `/account/`: Subscribe → fake checkout → back to your account: "Your subscription is active".
   6. Manage billing → try "Simulate: next payment fails", "Cancel at period end", "Cancel now", and watch the
      account page and `calculator.mjs status` change.
   7. Delete the account at the bottom of `/account/`.
5. `Ctrl+C` stops everything. Your test data stays for next time; `--fresh` starts empty.

---

## 3. The Stripe account **(you)**

**Who buys.** Buyers are adults: checkout requires "I am 18 or older and agree to the Terms" (decided
2026-10-11). The account holder is an adult who pays; the calculator can be used by students 13+.

**Who owns it.** The person who opens the Stripe account must meet Stripe's own age rule for account owners
(18+, or a parent or guardian as the owner). That's Stripe's requirement, not ours, and it applies to the
company's owner, not to customers. Opening it in the name of Proofline Corporation (once it's registered)
also needs the company's details.

**What Stripe asks for (sole proprietor in the US).** The owner's name, date of birth, address, and SSN (or an
EIN if you have one; Stripe still asks for the representative's SSN), and a bank account for payouts. Many
founders get a free EIN from the IRS first, so a business number goes on forms instead of an SSN. Ask the
accountant about an LLC (see PLAN section 9).

**Start in a sandbox.** Stripe's 2026 dashboard uses **Sandboxes** (they replaced "test mode"). A sandbox is a
separate practice copy of your account: fake cards, no real money, no activation needed. Do everything below in
a sandbox first, then repeat it in live mode at launch.

---

## 4. Set up Stripe (in the sandbox first) **(you)**

### 4.1 Products and prices
Dashboard → Product catalog → Add product:
1. **AI tutor plan**, two prices on the same product, both with tax code "Software as a service (SaaS) –
   personal use" (`txcd_10103000`):
   - Recurring, Monthly, $10.00 USD → `STRIPE_PRICE_SUB`
   - Recurring, Yearly, $100.00 USD → `STRIPE_PRICE_SUB_YEARLY` (leave empty to offer monthly only)
2. **The calculator**: One time, $225.00 USD. Tax code: "General – Tangible goods" (`txcd_99999999`).
3. Copy each **price ID** (starts with `price_`). These become `STRIPE_PRICE_SUB`, `STRIPE_PRICE_SUB_YEARLY`
   and `STRIPE_PRICE_DEVICE`.

### 4.2 Public details
Settings → Public details: business name, support e-mail, and the **Terms of service URL**
(`https://<your domain>/terms/`). Checkout needs this, because ours shows "I am 18 or older and agree to the
Terms of service" and makes it required.

### 4.3 Billing settings (important)
- **Failed payments:** Billing → Revenue recovery (or "Manage failed payments"): turn on Smart Retries, and
  set "If all retries fail" to **cancel the subscription** (or "mark as unpaid"). Don't choose "leave as past
  due": our server gives 3 days of grace, and a subscription left past due forever would just sit there.
- **Customer e-mails:** turn on receipts, failed-payment e-mails, and **renewal reminders** (California's
  auto-renewal law expects a reminder; ask the lawyer about timing).
- **3-D Secure confirmations (important):** Settings → Billing → Subscriptions and emails → turn on **"Send a
  Stripe-hosted link for customers to confirm their payments when required"**. Some banks make the customer
  confirm a renewal (3-D Secure / SCA). Stripe then sends that customer an e-mail with the link, and our
  server (event `invoice.payment_action_required`) shows **"Confirm your payment"** on the account page with
  the same link, instead of the plan quietly entering its 3-day grace. Test it with the card
  `4000 0027 6000 3184` and a test clock.
- **One subscription per customer:** in Checkout settings, turn on "Limit customers to one subscription" if
  it's offered (our server also blocks a second subscription itself).

### 4.4 Customer portal ("Manage billing")
Settings → Billing → Customer portal:
- On: update payment method, invoice history, **cancel subscription → at end of billing period**, ask for a
  cancellation reason (optional).
- **Switch plans: on**, between the monthly and yearly prices of the AI tutor plan only (prorated). Off:
  quantities, promotion codes.
- Customer information: address on (needed for tax), **e-mail off** (the Stripe e-mail must stay the account
  e-mail).
- Save it; the "Manage billing" button won't work until a configuration exists. Better: create a **dedicated
  configuration** with the settings above (Dashboard, or one API call), copy its id (`bpc_…`) and set
  `STRIPE_PORTAL_CONFIG` to it. Our server then always asks for that configuration, so later edits to the
  account's default can't change what customers can do. Empty = the account default.

### 4.5 Stripe Tax (optional at first)
Settings → Tax: head-office address (where you ship from), default tax behaviour, then add registrations
under Tax → Locations only where you're registered to collect tax. Costs 0.5% per transaction where it
calculates tax. Turn it on in our server with `STRIPE_TAX=1`. Ask the accountant which states you must
register in.

### 4.6 A restricted API key
Developers/Workbench → API keys → **Create restricted key** (Stripe now recommends these over the full
secret key). Permissions: Customers **Write**, Checkout Sessions **Write**, Customer portal **Write**,
Subscriptions **Write**, everything else **None**. Copy it once (`rk_test_…`) into your password manager.
This becomes `STRIPE_SECRET_KEY`.

### 4.7 The webhook
Workbench → Webhooks → Create an event destination → your account → **pick an explicit API version** (write
it down: it becomes `STRIPE_API_VERSION`, e.g. `2026-09-30.clover`) → events:
`checkout.session.completed`, `checkout.session.async_payment_succeeded`, `checkout.session.async_payment_failed`,
`customer.subscription.created`, `customer.subscription.updated`, `customer.subscription.deleted`,
`customer.subscription.paused`, `customer.subscription.resumed`, `invoice.paid`, `invoice.payment_failed`,
`invoice.payment_action_required`, `customer.deleted`, `charge.refunded`, `charge.dispute.created`,
`charge.dispute.closed`
→ endpoint URL `https://<your worker address>/api/stripe/webhook` → Reveal the **signing secret** (`whsec_…`).
That becomes `STRIPE_WEBHOOK_SECRET`. (Do this after section 5 gives you the address.)

**The API version must match.** Our server sends `STRIPE_API_VERSION` with every call, and it refuses to do
any Stripe work while a key is set without it (the page answers "not available right now" and the log says
`config_error`). Create the webhook endpoint with **the same** version, so the events and our calls have the
same shape. When you upgrade later, upgrade both together (new endpoint with the new version, then the
variable).

What the new events do: `invoice.payment_action_required` shows "Confirm your payment" on the account page
(section 4.3); `customer.deleted` (someone deleted the customer in the Dashboard) unlinks it, so the next
checkout makes a new one instead of failing; `charge.refunded` and `charge.dispute.*` move a calculator order
to `partially_refunded` / `refunded` or `disputed` → `dispute_won` / `dispute_lost` (don't ship a disputed
order). Make refunds in the Dashboard; the key needs no Refunds permission. `invoice.payment_succeeded` is
also understood but not needed (it repeats `invoice.paid`).

Stripe retries failed deliveries for up to 3 days in live mode, so short outages are fine: our server answers
"try again" whenever it can't save something.

---

## 5. Put it online for free **(you)**

One Cloudflare Worker serves both the website and the server, on one address. (Cloudflare now recommends
Workers over Pages for new projects, and static pages are free and unlimited. PLAN.md said Pages; this is the
one change, and it's simpler: one deploy, one address, first-party cookies.)

1. Free Cloudflare account, then in `web/api`: `npx wrangler login`.
2. Create the database and storage, and paste the printed ids into `web/api/wrangler.toml`:
   `npx wrangler d1 create ai-calc` and `npx wrangler kv namespace create FIRMWARE`.
3. `npx wrangler d1 migrations apply DB --remote`
4. Secrets, typed in when asked (never into a file or a chat):
   ```
   npx wrangler secret put ANTHROPIC_API_KEY
   npx wrangler secret put TRIAL_PEPPER
   npx wrangler secret put ADMIN_TOKEN
   npx wrangler secret put STRIPE_SECRET_KEY
   npx wrangler secret put STRIPE_WEBHOOK_SECRET
   ```
5. In `wrangler.toml` `[vars]`: `STRIPE_PRICE_SUB`, `STRIPE_PRICE_SUB_YEARLY`, `STRIPE_PRICE_DEVICE` (sandbox price ids),
   `STRIPE_API_VERSION` (**required** once the key is set: the webhook endpoint's version, section 4.7),
   `STRIPE_PORTAL_CONFIG` (`bpc_…`, section 4.4), `STRIPE_SHIPPING_RATE` (optional `shr_…`: a shipping rate,
   e.g. "Free US shipping", with the shipping tax code `txcd_92010001`; empty = no delivery line), `STORE_OPEN = "0"`
   (the store stays closed until preorders), `PUBLIC_URL = "https://ai-calc-api.<you>.workers.dev"` for now.
6. `npm run deploy`, then open the address: the website should load, and `/healthz` should say `{"ok":true}`.
7. Create the webhook (section 4.7) with that address, add its secret (step 4), deploy again.
8. **Test with Stripe's test card** `4242 4242 4242 4242` (any future date, any CVC): sign in, subscribe, check
   the account page says active and the webhook deliveries show 200 in Stripe.
9. Set a monthly spending limit in the Anthropic console as a backstop.

Free-plan limits that matter: 100,000 requests a day, 10 ms of CPU per request (waiting on the network
doesn't count), D1 with 5 GB. Upgrade to Workers Paid ($5/month) when real customers' photos start hitting
the CPU limit.

---

## 6. Before taking real money **(you)**

1. **Lawyer:** the Terms, Privacy and Refunds templates (marked NEEDS LAWYER REVIEW), California's automatic
   renewal law (it now covers free-to-paid conversions; our no-card free month avoids an automatic conversion,
   but the paid plan still renews monthly), COPPA and student privacy, and the AI provider's policy on users
   under 18. The FTC's "click to cancel" rule was struck down in 2025, but a new proposal is open, and the
   federal ROSCA law still applies. Online sign-up needs online cancellation: we have it.
2. **Accountant:** registering **Proofline Corporation** (a corporation must actually be incorporated before
   the name can say "Corporation"; an LLC is the common alternative), EIN, sales tax registrations, Stripe Tax.
   Also a trademark search on "Proofline" before printing anything.
3. **Domain** (~$10–15/year): point it at the Worker, set `PUBLIC_URL`, replace the placeholder domain
   (`proofline.example` is the placeholder; checklist in `web/NAMES.md`).
4. **Live mode:** activate the Stripe account, recreate products, portal settings, restricted key and webhook
   in live mode (Stripe's "Copy to live mode" helps with products), and swap the secrets.
5. **E-mail sending:** a free Resend account for sign-in e-mails (`RESEND_API_KEY`, `MAIL_FROM`).
6. **Turnstile** (free bot check) on sign-in and the waitlist: **required before launch.** Without it one
   address can use up the day's sign-in e-mail budget in about 4 hours (security review S4, round 2).
   Also ask the lawyer how long to keep store orders (they hold the buyer's e-mail and address).
7. **FCC** testing before selling hardware.
8. **Open the store** for preorders: `STORE_OPEN = "1"` and deploy. The site switches from "Join the waitlist"
   to "Preorder" by itself.

---

## 7. What it costs

| Item | When | Cost |
|---|---|---|
| Everything above in sandbox | now | $0 |
| Domain | at launch | ~$10–15 / year |
| Cloudflare Workers Paid | when the free plan's CPU limit bites | $5 / month |
| Stripe per $10 monthly payment | each month | 2.9% + 30¢ + 0.7% Billing ≈ $0.66 (+0.5% with Stripe Tax ≈ $0.71) |
| Stripe per $100 yearly payment | each year | ≈ $3.90 (≈ $0.33 a month; + $0.50 with Stripe Tax) |
| Stripe per $225 calculator | each sale | ≈ $6.83 (+ tax fee if used) |
| AI per subscriber | each month | ~$3.90 at 150 solves (cap: $5) |
| Lawyer review, FCC | before launch | see PLAN section 6 |

At 50 monthly subscribers: $500 of plan revenue a month against roughly $33 of Stripe fees and up to $195
of AI (at 150 solves each). Yearly subscribers pay less per month ($8.33) but cost less in fees and don't
drop off month to month. PLAN section 6 was written at $15 and is now out of date on revenue.

---

## 8. Running it day to day

| Task | How |
|---|---|
| See who's on the waitlist | `npx wrangler d1 execute DB --remote --command "SELECT email, source, utm_source, datetime(created_at,'unixepoch') FROM waitlist ORDER BY created_at"` (numbers by channel: 8.1) |
| Remove someone from the waitlist | `... --command "DELETE FROM waitlist WHERE email = 'them@example.com'"` |
| Orders to ship | Stripe dashboard → Payments (shipping address is on each payment), or `SELECT * FROM orders WHERE status = 'paid'` |
| Refund an order | Stripe dashboard → the payment → Refund |
| A customer can't use AI | Ask for their account e-mail; check the plan on Stripe; the calculator's screen message says why |
| Cancel someone's subscription for them | Stripe dashboard → the customer → the subscription → Cancel (the webhook updates us) |
| Webhook failures | Stripe → Workbench → Webhooks → the endpoint → failed deliveries → Resend |
| Turn the store off | `STORE_OPEN = "0"`, deploy |

### 8.1 Waitlist numbers by channel (admin only)

Every sign-up stores which form (`source`: hero, footer, site), the link's `utm_source`,
`utm_medium`, `utm_campaign` (the site also reads the short `?src=tt` as `utm_source`), a referral
code `ref` (`?ref=`), and `age_ok = 1` (ticked "I'm 13 or older"). The site keeps the first link of
a visit (first touch); a repeat sign-up changes nothing. Tags are lower-cased and kept only if they
match `[a-z0-9_-]{1,40}`. There is deliberately no public endpoint for these numbers: run the
queries with Wrangler (`npx wrangler d1 execute DB --remote --command "..."`; `--local` for the dev
database).

| Question | SQL |
|---|---|
| Sign-ups by channel | `SELECT coalesce(utm_source,'(none)') AS channel, count(*) AS n FROM waitlist GROUP BY 1 ORDER BY n DESC` |
| By channel, medium and campaign | `SELECT utm_source, utm_medium, utm_campaign, count(*) AS n FROM waitlist GROUP BY 1,2,3 ORDER BY n DESC` |
| By form on the site | `SELECT source, count(*) AS n FROM waitlist GROUP BY source` |
| New this week, by channel | `SELECT coalesce(utm_source,'(none)') AS channel, count(*) AS n FROM waitlist WHERE created_at > unixepoch() - 7*86400 GROUP BY 1 ORDER BY n DESC` |
| Per day (last 30 days) | `SELECT date(created_at,'unixepoch') AS day, count(*) AS n FROM waitlist WHERE created_at > unixepoch() - 30*86400 GROUP BY day ORDER BY day` |
| One creator's link (`?utm_source=c-alex` or `?src=c-alex`) | `SELECT count(*) FROM waitlist WHERE utm_source = 'c-alex'` |
| Referral codes used | `SELECT ref, count(*) AS n FROM waitlist WHERE ref IS NOT NULL GROUP BY ref ORDER BY n DESC` |
| Rows from before the 13+ checkbox | `SELECT count(*) FROM waitlist WHERE age_ok IS NULL` |

**Limits:** 5 sign-ups per IP per hour, and `WAITLIST_DAILY_CAP` (default 20,000) new addresses a
day for everyone. The cap is only an abuse backstop, sized so a viral video doesn't turn people
away; when it is hit, nothing is stored and the form says the list is busy today. Turnstile is still
**required before launch** (section 6): the cap alone doesn't stop a bot from filling the list.

**Referrals later (not built, no rewards yet):** each member would get a personal code (for
example a short random id stored in a new column), their link would be `/?ref=<code>`, and the
site already sends `ref` with every sign-up. Counting is then
`SELECT ref, count(*) FROM waitlist WHERE ref = '<code>'`; the reward is a real one only (an
earlier batch, MARKETING_GUIDE section 8), never a prize. The sign-up answer stays the same for new
and repeated addresses, so a referral page must never show whether a given e-mail joined.

---

## 9. Decisions for you

1. **Domain and contact e-mail** for Proofline (placeholders `proofline.example`, `hello@proofline.example`).
2. **Who owns the Stripe account** (see section 3; buyers are 18+, decided).
3. **One Worker for site + server** instead of Cloudflare Pages + a Worker (recommended, section 5).
4. **Subscribing during the free month:** today it's allowed and billing starts straight away (the account
   page warns about this). Alternative: hide the button until the free month ends.
5. **Failed payments:** cancel after Stripe's retries (recommended) or mark unpaid.
6. **Store opening date** (`STORE_OPEN`), and the return window and warranty length for the Refunds page.
7. **Stripe Tax** now or later.
8. ~~Updates without the plan~~ **Decided 2026-10-11:** updates come only with the plan. Suggestion to
   consider later: still push urgent security fixes to everyone (a lawyer may say you must), even if feature
   updates stay plan-only.

---

## 10. What the security review found and what changed (stage 3)

| ID | Severity | Problem | Fix |
|---|---|---|---|
| S3-1 | High | Grace period for failed payments was counted from the period end, which Stripe moves forward, so it was ~a month | Grace counts from the failed payment (`past_due_since`) |
| S3-2 | High | Deleting an account with a checkout still open could leave billing running | Deletion deletes the Stripe customer first; a subscription that appears for a deleted account is cancelled |
| S3-3 | Medium | Two checkout tabs could create two subscriptions | Server checks Stripe before a new checkout; checkouts expire after 31 minutes |
| S3-4 | Medium | Events arriving in the same second, out of order, could leave a paid account looking unpaid | Every event re-reads the subscription from Stripe |
| S3-5 | Low | Any subscription on the Stripe account would unlock AI | Only the AI tutor plan's price unlocks AI |
| S3-6 | Low | Orders could move back from paid to unpaid; other payments looked like orders | Orders only move forward; only our store's checkouts count |
| S3-9 | Low | Store checkout had no global limit | 300 per hour across everyone, plus 10 per person |
| S3-11 | Info | Live and test events weren't told apart | Mismatched events are refused |
| S3-8, S3-12 | Low/info | Waitlist spam; a test key in production | Turnstile and double opt-in before any marketing e-mail; a deploy checklist item |

### Round 2 (2026-10-11, with the yearly plan)

| ID | Severity | Problem | Fix |
|---|---|---|---|
| S4-1 | Medium | One address could use up the store's hourly checkout budget for everyone | Only checkouts that pass the per-address limit count toward the global one |
| S4-2 | Low | A malformed calculator id in a URL answered "AI server is having trouble" | Answers "not found" |
| S4-3 | Low | Deleting an account left the calculators' names behind | Names are erased with the account |
| S4-4 | Low | Deleting the account in one tab while checking out in another could leave a payment customer behind | That customer is deleted and the checkout refused |
| — | Medium (open) | Sign-in e-mail daily budget can be used up by one address | Turnstile, required before launch (section 6) |
