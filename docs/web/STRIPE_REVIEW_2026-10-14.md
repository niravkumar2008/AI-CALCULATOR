# Stripe integration review: 2026-10-14

Read-only review. No code was changed, nothing was written to Stripe, and no git commands were run.
Stripe account: the Proofline sandbox (`acct_1UQFqJFMsMHP8Dwz`, livemode=false).

What was reviewed: `web/api/src/{stripe,billing,plan,account,env,index}.ts`, the billing, store and webhook parts
of `docs/web/openapi.yaml`, `docs/web/GUIDE.md` sections 1, 4, 6, 9 and 10, and `web/api/test/{billing,yearly}.test.ts`.
The sandbox was also checked with read-only API calls (prices and products, portal configurations,
webhook endpoints, tax settings).

---

## 1. Planner summary (Stripe integration planner, guide `iguide_61VZspl4YBYPFo7H441FMsMHP8Dwz`, accepted)

Decision path chosen, and the recommendation for each:

| Area | Choice | Recommended Stripe product | Docs |
|---|---|---|---|
| Collecting payments (calculator) | Stripe only, web browser, not digital-only (Managed Payments doesn't apply because we sell hardware), no invoices for normal buyers, not Payment Links | **Stripe Checkout, hosted** (the planner's default) | https://docs.stripe.com/payments/accept-a-payment?payment-ui=checkout&ui=stripe-hosted |
| Collecting payments (plan) | Web, code integration, no need to control the page layout, a redirect is fine | **Stripe-hosted Checkout in subscription mode** | https://docs.stripe.com/billing/subscriptions/build-subscriptions?payment-ui=checkout&ui=stripe-hosted |
| Pricing model | No seats, no tiers | **Flat rate**, with two prices (monthly and yearly) on one product | https://docs.stripe.com/subscriptions/pricing-models/flat-rate-pricing?dashboard-or-api=api |
| Free access | Free access, no card at signup | **Freemium** (our own free month, outside Stripe) | https://docs.stripe.com/billing/subscriptions/design-an-integration#select-billing-model |
| Changes after the sale | Customers manage the plan themselves, no custom UI, no usage-based billing | **Customer Portal** | https://docs.stripe.com/customer-management/integrate-customer-portal |
| Failed payments | Stripe handles recovery, no custom logic | **Smart Retries + automated emails + automatic card updates** | https://docs.stripe.com/billing/revenue-recovery |
| Tax | Collecting where registered | **Stripe Tax with collection** (or free threshold monitoring until we're registered) | https://docs.stripe.com/tax, https://docs.stripe.com/tax/monitoring |

What the planner recommends on top of that:
- Turn on **Link, dynamic payment methods** (don't hard-code `payment_method_types`; our code already leaves it out)
  and Adaptive Pricing. Adaptive Pricing doesn't matter while we sell only in the US.
- Create new subscriptions in **flexible billing mode** (`billing_mode[type]=flexible`).
- Use **Cancel at period end** in the portal.
- For invoices (schools): Hosted Invoices, through the Dashboard (https://docs.stripe.com/invoicing/dashboard) or the API
  (https://docs.stripe.com/invoicing/integration).

**Verdict:** the overall design already matches what the planner recommends: hosted Checkout, flat-rate prices, a free tier
outside Stripe, the Portal, and webhooks that re-read the subscription. The gaps below are about settings, the product
catalog and how many webhook events we handle. None of them needs a change of architecture.

---

## 2. What is already good (keep it)

- **Webhook signature check** (`stripe.ts` `verifyWebhook`): HMAC-SHA256 over `t.rawBody`, 5-minute tolerance, a
  constant-time compare across all `v1` signatures, and the body is parsed only after the check passes. Events whose
  livemode doesn't match the key are refused (S3-11).
- **Idempotent webhooks:** the event id and the event's effects are written in one D1 batch. A concurrent duplicate is
  detected and answered with 200.
- **Stripe is the source of truth:** every subscription or invoice event re-reads the subscription, matches it to an
  account by the customer id only (metadata is never trusted), and copes with events arriving out of order.
- **Both API shapes:** the period end (`current_period_end` on the subscription or on its items) and the invoice's
  subscription (`subscription` or `parent.subscription_details`) are read in either the old or the new form.
- **Redirects** go only to `checkout.stripe.com` or `billing.stripe.com`. Errors give users fixed text, and Stripe
  details go only to the logs.
- **Second subscriptions are blocked** using both our database and Stripe. Sessions expire after 31 minutes.
- **Account deletion** deletes the Stripe customer first, which fails closed with a 503.
- **The 3-day grace** counts from when we first saw `past_due` (`past_due_since`), not from the period end.
- **Settings fail closed:** keys and prices are checked against regexes, a live key is refused when DEV_MODE=1, and
  Stripe must be reached over https.
- **Restricted keys** (`rk_`) are accepted. GUIDE 4.6 already asks for the smallest set of permissions.

---

## 3. Gaps, in priority order

Effort: **XS** under 30 minutes, **S** a few hours, **M** about a day, **L** several days.

### P0: fix before taking any real money

**G1. The plan's recurring prices sit on the tangible-goods product, so the plan would be taxed as a physical item
(tax, correctness).**
Found in the sandbox: all three prices (`price_1UQUsaFMsMHP8DwzEURtqvsM` one-time $225,
`price_1UQUsaFMsMHP8DwzyQSw4kum` $10/month, `price_1UQUsbFMsMHP8DwzWSiC6VP1` $100/year) belong to
`prod_VRNdtfIoWjy0kt` "Proofline calculator", which has tax code `txcd_99999999` (General – Tangible goods). A price
can't have its own tax code: Stripe Tax always uses the product's code. So when `STRIPE_TAX=1`, the $10 and $100
plan charges would be taxed as **tangible goods**, not as SaaS (`txcd_10103000`). Many US states tax tangible goods
and SaaS differently: some tax only one of them, and some tax them at different rates. This setup would collect too
much tax in some states and too little in others, and the filed returns would be wrong.
Stripe's catalog guidance says to create a new product for anything with a different tax code
(https://docs.stripe.com/products-prices/how-products-and-prices-work#when-to-create-a-new-product). GUIDE 4.1
already describes two products, so the sandbox doesn't match the guide.
*Change:* create a second product, "Proofline AI tutor plan", with tax code `txcd_10103000`, and create monthly
($10) and yearly ($100) prices on it with `tax_behavior=exclusive`. Prices can't be moved between products. Move the
lookup keys `ai_tutor_monthly` and `ai_tutor_yearly` to the new prices with `transfer_lookup_key=true`, archive the
two old recurring prices, and update `STRIPE_PRICE_SUB` and `STRIPE_PRICE_SUB_YEARLY`. The plan can still feel like
part of the calculator. Give the product a neutral name and description, such as "AI Solve for your Proofline
calculator". The name only shows on Checkout, receipts and the portal. Also remove the product-level metadata
`kind=device` from the calculator product. It's harmless, because the code checks session metadata, but it's
misleading. Optionally mark the calculator product `shippable=true`. *Effort: XS (Dashboard), no code.*
Confirm the right tax code with the accountant. The AI tutor might be classed as SaaS for personal use
(`txcd_10103000`) or as a different digital service.

**G2. Stripe Tax isn't set up in the sandbox yet (tax).**
Found: `tax.settings.status = pending`, and `head_office` is missing. With `STRIPE_TAX=1`, every Checkout using
`automatic_tax` would fail until the head office is set. *Change:* set the head office (the ship-from address). Leave
the default tax behaviour as "exclusive", which matches the prices. Then either add registrations, or stay on free
threshold monitoring (https://docs.stripe.com/tax/monitoring) until the accountant says to register, and only then
set `STRIPE_TAX=1`. If you charge for shipping, add a shipping tax code (see G7). *Effort: XS.*

**G3. The Customer Portal isn't configured, and the code doesn't choose a configuration (correctness, best
practice).**
Found: the sandbox has **no** portal configuration, so `/api/billing/portal` would fail with a 503 today. The code
calls `POST /v1/billing_portal/sessions` without `configuration`, so it always gets the account default. Anyone who
later edits that default in the Dashboard would change what customers can do.
*Change:*
1. Create a dedicated configuration (Dashboard, or one API call) with:
   - `features.subscription_update.enabled=true`
   - `default_allowed_updates=["price"]` (no quantity, no promotion codes)
   - `products=[{product: <AI tutor plan product>, prices: [<monthly>, <yearly>]}]`. Only those two prices can be
     switched between, and the calculator's one-time price can never appear.
   - `proration_behavior=create_prorations`, or `always_invoice` to charge the difference right away when someone
     switches from monthly to yearly
   - `subscription_cancel.enabled=true`, `mode=at_period_end`, and `cancellation_reason` turned on
   - `payment_method_update` and `invoice_history` turned on
   - `customer_update.allowed_updates=["address"]` (needed for tax). Leave **email** off, so the Stripe email
     can't drift away from the account email.
   - `business_profile.terms_of_service_url` and `privacy_policy_url`
2. Add an optional env var `STRIPE_PORTAL_CONFIG` (`bpc_…`, checked like the other ids) and pass
   `configuration` in `billingPortal()`. *Effort: S (about 10 lines plus a test).*

The two G1 products make this cleaner: the portal's `products` list then contains nothing but the plan.

**G4. The API version isn't pinned (best practice, correctness).**
`STRIPE_API_VERSION` is optional, and an empty value means "the account's default version". The account default
changes when someone upgrades it in the Dashboard, and the webhook endpoint's version is set separately. The code
copes with both shapes for the fields it reads today, but any new field won't be covered.
*Change:* in `parseStripe()`, require `STRIPE_API_VERSION` whenever `secretKey` is set and `DEV_MODE` is off.
Create the webhook endpoint with **the same** `api_version`. Record the version in GUIDE 4.7 and 5. Add a test
that the `stripe-version` header is always sent in production. *Effort: XS–S.*

### P1: before launch

**G5. The webhook handles too few events (correctness, SCA).**
The code handles `checkout.session.*`, `customer.subscription.*`, `invoice.paid`, `invoice.payment_succeeded` and
`invoice.payment_failed`. These are missing:
| Event | Why | Recommended handling |
|---|---|---|
| `invoice.payment_action_required` | A renewal needs 3-D Secure (SCA). The subscription goes `past_due`, our 3-day grace starts, and the customer may never find out. | Re-read the subscription (same as `payment_failed`). Store `hosted_invoice_url` so the account page can show "Confirm your payment". **Also** turn on *Settings → Billing → Subscriptions and emails → "Send a Stripe-hosted link for customers to confirm their payments when required"*. |
| `customer.deleted` | If someone deletes the customer in the Dashboard, `users.stripe_customer_id` still points to a deleted customer. Every later checkout and portal call then answers 503 for good. | Set `stripe_customer_id = NULL` where it matches. Subscription rows already become `canceled` through `customer.subscription.deleted`. |
| `charge.refunded`, `refund.updated` | Store orders never show refunds (see G6). | Move the order to `refunded` or `partially_refunded`. |
| `charge.dispute.created`, `charge.dispute.closed` | A chargeback on a calculator order should stop shipping or flag the order. | Move the order to `disputed` and record the outcome. |
| `invoice.upcoming` (optional) | Renewal reminders for the yearly plan (California's auto-renewal law). | Usually enough: turn on Stripe's own renewal-reminder emails (GUIDE 4.3). Only handle the event if you send your own e-mail. |
| `checkout.session.expired` (optional) | Analytics, abandoned carts | Ignore it, or count it. |

Also add `invoice.payment_succeeded` to the GUIDE 4.7 list, or drop it from the code. It duplicates `invoice.paid`,
so subscribing to only `invoice.paid` is enough. *Effort: S for the first two, M together with G6.*

**G6. No refund or dispute handling for calculator orders (correctness, operations).**
`orderUpsert()` treats `paid` as final, the `orders` table has no refunded or disputed state, and the order doesn't
store `payment_intent`. Refund and dispute events point to a charge or payment intent, not to the Checkout Session,
so they can't be matched to an order today. *Change:* store `payment_intent` from the completed session in
`orders`. Allow `paid → refunded | partially_refunded | disputed` (and `disputed → paid | lost`), while keeping
"never back to unpaid". Handle the G5 events. Do refunds in the Dashboard, so the restricted key needs no Refunds
permission. For the plan, refunds need no code, because entitlement follows the subscription's status. Write a
policy for yearly refunds, which the lawyer and the Refunds page decide. *Effort: M.*

**G7. Checkout settings for tax and shipping (best practice).**
Calculator (`storeCheckout`):
- The OpenAPI file says "phone optional", but the code doesn't collect a phone number. Add
  `phone_number_collection[enabled]=true`, which carriers need, or change the docs.
- Add `shipping_options` with a shipping rate: "Free US shipping" for $0, or a paid rate with
  `tax_code=txcd_92010001` (shipping). Without one, Checkout shows no delivery line, and tax on shipping is never
  decided.
- `automatic_tax` together with `shipping_address_collection` is correct: Stripe Tax uses the shipping address.
  Optionally add `billing_address_collection=auto`.
- Optionally use `invoice_creation[enabled]=true`, which gives a PDF invoice for school reimbursement. It has an
  Invoicing fee, so turn it on only if buyers ask for it.
- Optionally pass `{CHECKOUT_SESSION_ID}` in `success_url`, so `/order/complete/` can show the order. The webhook
  stays the source of truth.

Plan (`billingCheckout`):
- `customer_update[address]=auto` and `[name]=auto` are already set when tax is on, which is correct. Add
  `billing_address_collection=required` while `STRIPE_TAX=1`, so US local rates use a full address and not just a
  ZIP code.
- Add `subscription_data[billing_mode][type]=flexible`, which the planner recommends for new subscriptions. Check
  that your API version supports it.
- Leave out `allow_promotion_codes` until you need coupons for Kickstarter (G11) or schools.
*Effort: S.*

**G8. Restricted key permissions (security).** GUIDE 4.6 is correct for today's code: Customers Write, Checkout
Sessions Write, Customer portal Write, Subscriptions Write, everything else None. Keep it at that. Notes:
- G3: the portal configuration is created in the Dashboard, so no extra permission is needed.
- G5/G6: everything is read from the event payload, so no Charges, Refunds or Invoices read is needed. Add
  **Invoices Read** only if you decide to re-read invoices.
- Never give the Worker key Webhook Endpoints, Products/Prices or Refunds write.
- Use a different restricted key for each environment (sandbox, live). Rotate any key that was ever pasted into a
  chat or a log.
- The webhook secret is per endpoint. Keep the sandbox and live secrets apart, which the S3-11 livemode check
  already enforces in code.
*Effort: XS (none if GUIDE 4.6 is followed).*

**G9. Idempotency details (correctness, low).**
- `customer-<user id>`: Stripe stores the result of a request that started executing, **including 5xx errors**,
  for 24 hours. One temporary Stripe 500 on the first customer creation would replay as a 500 for a day. That user
  couldn't subscribe for 24 hours, though the 503 would retry fine after that. *Change:* on a 5xx answer for that
  call, retry once with a key like `customer-<user id>-r<n>`. The DB guard (`stripe_customer_id IS NULL`) already
  stops a second customer from being linked, but an orphan customer could still be created and should be deleted.
  Alternatively, accept the risk and write it down.
- Checkout Sessions are created without a key. That's fine, because a duplicate session just expires after 31
  minutes.
- `cancel-<sub id>` on delete is correct.
- The `stripe_events` table is never pruned (the `cleanup()` cron doesn't touch it). Stripe retries for at most 3
  days, so delete rows older than 30 days. *Effort: XS.*

**G10. The free month and paid billing overlap (product decision, GUIDE section 9 item 4).**
Today, subscribing during the free month charges straight away, and the remaining free days are wasted. A cheap fix
that keeps decision D2 (no card needed for the free month): when the user subscribes and `trial_ends_at` is more
than 48 hours away, pass `subscription_data[trial_end]=<trial_ends_at>`. Checkout then collects the card, and the
first charge happens when our free month ends. `planOf()` already treats `trialing` as entitled. The catch: this is a
*card-on-file* free-to-paid conversion, which California's auto-renewal law covers. It needs the renewal-reminder
e-mail and clear wording at checkout, so ask the lawyer. Otherwise, keep today's behaviour with the warning on the
account page. *Effort: S.*

### P2: later (Kickstarter, schools, preorders)

**G11. Prepaid 12-month plan bundles for Kickstarter (no code yet).** The options:
| Option | How | Pros | Cons |
|---|---|---|---|
| **A. Our own entitlement (recommended)** | A redeem code (or the backer's e-mail) sets a `prepaid_until` date on the account (or pushes `trial_ends_at` 12 months out) when the calculator is linked. Nothing at Stripe until they choose to subscribe afterwards. | It's exactly how the no-card free month already works. No card needed, no $0 invoices, no automatic conversion to paid, and no tax recorded at Stripe (Kickstarter collected the money). | We have to build redeem codes, and the end-of-prepaid reminders are ours. |
| B. 100%-off coupon for 12 months | Coupon `percent_off=100`, `duration=repeating`, `duration_in_months=12`, as a promotion code in Checkout (`allow_promotion_codes`) or set on the subscription | Uses Stripe's machinery and turns into a paid plan by itself | Needs a card at signup. Sends 12 $0 invoices with $0 tax, so the tax on the bundle has to be reported separately. Counts as a free-to-paid conversion (California's law). The portal can't change the plan without dropping the coupon. |
| C. Customer balance credit | Credit −$100 to the customer balance | Simple | The credit is used up by tax and by whichever invoice comes first, and it doesn't fit the monthly price neatly. Not recommended. |
| D. `trial_end` 12 months out | `subscription_data[trial_end]` up to 2 years | One field | Shows as a "trial", needs a card, and has the same conversion rules as B. |
| E. Subscription schedule | Phase 1: yearly price + 100% coupon for 1 iteration; phase 2: yearly price | Exact control of the timeline | The most code, it can't be done through Checkout alone, and the portal has limits with schedules. |
Recommendation: **A** now. Use **B** only if you later want backers to roll onto a paid plan by themselves.
Sales tax on Kickstarter bundles is outside Stripe in every case. Ask the accountant, and import off-Stripe sales into
Stripe Tax for reporting if needed (https://docs.stripe.com/tax/imports). *Effort: M for A.*

**G12. Invoicing for schools (purchase orders).** Start with **Dashboard-only Stripe Invoicing**
(https://docs.stripe.com/invoicing/dashboard):
- Create a customer for the school with `tax_exempt=exempt` once you have its certificate (or use the EXEMPTAX app).
- Create a one-time invoice with the calculator price (quantity above 3 is allowed there),
  `collection_method=send_invoice`, `days_until_due=30`, and the PO number as an invoice custom field.
- The school pays by card or ACH on the hosted invoice page.

Code impact: `invoice.paid` for an invoice with no subscription is ignored today (`invoiceSubscriptionId` returns
null, so nothing is written). The paid school order therefore never reaches `orders`. When you start doing this, add
a branch: an invoice with `metadata.kind=device_po` creates an order row. School plan bundles fit G11-A, a code per
calculator. *Effort: XS to start (Dashboard), S for the webhook branch.*

**G13. Preorders.** Checkout in payment mode charges right away. A manual capture
(`payment_intent_data[capture_method]=manual`) holds the money for only about 7 days, so it's no good for preorders
that wait months. Options: charge right away and refund on cancel (simplest, and you already have the refund
handling from G6), or use a Checkout `mode=setup` to save the card and charge when it ships (watch for failed and
SCA-required charges at that point). Ask the lawyer and the accountant about rules for taking money long before
shipping. *Effort: S–M when needed.*

**G14. Small items.**
- The grace starts when *we process* the first `past_due` event, so long webhook outages (503 retries) lengthen it.
  You could use the latest invoice's first failed attempt time instead. Low priority.
- Turn on Radar's default rules and keep Checkout's card-testing protection. The store checkout needs no sign-in, but
  it already has per-IP and global limits.
- Deleting an account in the middle of a yearly subscription cancels it at once with no refund. Write this into the
  Terms and Refunds pages.
- Stripe customers are created with the account email, and nothing keeps them in sync if the account email changes.
  Not an issue while the email can't be changed, and portal email editing stays off (G3).

---

## 4. Sandbox setup checklist (what the code expects)

**Stripe Dashboard (sandbox `acct_1UQFqJFMsMHP8Dwz`):**
- [ ] Product "Proofline calculator", tax code `txcd_99999999`: one-time price $225, `tax_behavior=exclusive`.
      Exists: `price_1UQUsaFMsMHP8DwzEURtqvsM` (lookup `calculator_one_time`). Remove the product metadata
      `kind=device` (optional).
- [ ] **New** product "Proofline AI tutor plan" (or another neutral name), tax code `txcd_10103000`: $10/month and
      $100/year prices, `tax_behavior=exclusive`, with the lookup keys `ai_tutor_monthly` and `ai_tutor_yearly`
      moved over (G1). Archive the old recurring prices `price_1UQUsaFMsMHP8DwzyQSw4kum` and
      `price_1UQUsbFMsMHP8DwzWSiC6VP1`.
- [ ] Settings → Public details: business name, support e-mail, **Terms of service URL** (needed because
      `STRIPE_REQUIRE_TOS` defaults to on) and Privacy URL.
- [ ] Settings → Tax: head office (G2). Registrations, or threshold monitoring only.
- [ ] Billing → Revenue recovery: Smart Retries on, "if all retries fail" → cancel the subscription. E-mails:
      receipts, failed payments, **3DS confirmation link** (G5), renewal reminders.
- [ ] Checkout settings: "Limit customers to one subscription" if it's offered. Payment methods: cards, Link, Apple
      Pay and Google Pay (dynamic payment methods).
- [x] Customer portal: a dedicated configuration as in G3 (only the two plan prices). Copy the `bpc_…` id.
      Done 2026-10-14 in the sandbox (API): `bpc_1UQV4YFMsMHP8DwzXeHqC23Y` (is_default, metadata app=proofline).
      Address-only customer edits, invoices, payment method, cancel at period end with reasons, switch between
      the monthly and yearly prices with prorations, quantity changes off. Privacy and Terms URLs not set yet
      (optional); default return URL is the local `http://127.0.0.1:8787/account/` and must change for live.
- [ ] Restricted key `rk_test_…` with the GUIDE 4.6 permissions (G8).
- [ ] Webhook endpoint `https://<worker>/api/stripe/webhook`, **with an explicit API version equal to
      `STRIPE_API_VERSION`**. Events: `checkout.session.completed`, `checkout.session.async_payment_succeeded`,
      `checkout.session.async_payment_failed`, `customer.subscription.created|updated|deleted|paused|resumed`,
      `invoice.paid`, `invoice.payment_failed`, `invoice.payment_action_required`,
      `customer.deleted`, `charge.refunded`, `charge.dispute.created`, `charge.dispute.closed`
      (the last five are handled by the code since the 2026-10-14 fixes; GUIDE 4.7 has the same list).
      Today: **no endpoint exists** in the sandbox.

**Worker settings (`wrangler secret put` / `[vars]`):**
| Name | Value | Required |
|---|---|---|
| `STRIPE_SECRET_KEY` | `rk_test_…` (restricted) | yes (otherwise billing and store answer 503) |
| `STRIPE_WEBHOOK_SECRET` | `whsec_…` from the endpoint | yes (otherwise the webhook answers 503 and Stripe retries) |
| `STRIPE_PRICE_SUB` | the new monthly price on the plan product | yes |
| `STRIPE_PRICE_SUB_YEARLY` | the new yearly price (must be different from the monthly and device prices) | optional (empty = monthly only) |
| `STRIPE_PRICE_DEVICE` | `price_1UQUsaFMsMHP8DwzEURtqvsM` | yes for the store |
| `STRIPE_API_VERSION` | the same version as the webhook endpoint, e.g. `YYYY-MM-DD.name` | **yes** with a key unless DEV_MODE=1 (G4, done) |
| `STRIPE_TAX` | `1` only after G1 and G2 | optional |
| `STRIPE_REQUIRE_TOS` | leave unset (on); `0` only if no Terms URL is set | default on |
| `STORE_OPEN` | `1` to open the store | default closed |
| `STORE_GLOBAL_PER_HOUR` | default 300 | optional |
| `STRIPE_API_BASE` | leave unset in the sandbox and live (only a local mock with DEV_MODE=1) | no |
| `STRIPE_PORTAL_CONFIG` | `bpc_…` (**new**, G3, supported by the code) | after G3 |
| `STRIPE_SHIPPING_RATE` | `shr_…` shipping rate for the calculator checkout (**new**, G7) | optional (empty = no delivery line) |
| `PUBLIC_URL` | https origin (it builds the success, cancel, return and Terms URLs) | yes |

**Smoke test in the sandbox** (Stripe CLI `stripe listen --forward-to …` or a public worker URL):
1. Subscribe monthly with `4242 4242 4242 4242`. The account becomes active.
2. Switch to yearly in the portal. AI stays on, and `plan_interval` becomes `year`.
3. Use the 3DS card `4000 0027 6000 3184` for a renewal (use a test clock). The confirmation e-mail or link appears.
4. Use the declining card `4000 0000 0000 0341` and move the test clock past renewal. The account becomes
   `past_due`, then loses AI after 3 days.
5. Buy 2 calculators. An order row is written with the shipping address and the tax line.
6. Refund one calculator order in the Dashboard. The order becomes `refunded` (after G6).
7. Delete the account. The customer is deleted at Stripe, and so is the subscription.

---

## 5. Top 5 recommendations

1. **Move the AI tutor plan prices onto their own product with tax code `txcd_10103000`** (G1). Today the sandbox
   would tax the plan as a tangible good. XS, Dashboard only, then update the two env vars.
2. **Create a dedicated Customer Portal configuration** that can only switch between the monthly and yearly plan
   prices, cancels at period end, and doesn't allow email edits. Pass its id from the code (G3). Today there is
   none, so "Manage billing" would fail.
3. **Pin the API version:** make `STRIPE_API_VERSION` required in production and create the webhook endpoint with
   the same version (G4).
4. **Handle `invoice.payment_action_required` and `customer.deleted`,** and turn on Stripe's 3DS confirmation e-mails
   (G5), so SCA renewals don't fall silently into the 3-day grace and a customer deleted in the Dashboard doesn't
   leave an account stuck on 503s.
5. **Add refund and dispute handling for calculator orders and finish the store Checkout settings**: store
   `payment_intent`, handle the `charge.refunded` and `charge.dispute.*` events, collect a phone number, and add a
   shipping rate with a shipping tax code. Set up the Stripe Tax head office before `STRIPE_TAX=1` (G2, G6, G7).
