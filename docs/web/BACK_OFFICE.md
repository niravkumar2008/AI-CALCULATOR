# Back office: business e-mail, Penny, and Business HQ

What it does (built 2026-11-18):

```
customer ──e-mail──▶ hello@your-domain ──Cloudflare Email Routing──▶ Worker email()
                                                                     ├─ stores it (D1 "mail" table)
                                                                     └─ forwards a copy to your Gmail
Proofline Farm (your PC) ──every minute──▶ Worker /v1/admin/mail ──▶ Penny drafts a reply (no tools, no sending)
                                                                     └─ draft saved back on the Worker
You, in Business HQ → Inbox: read, edit, "Approve & send" ──▶ Worker sends it through Resend, from hello@your-domain
```

Nothing goes out without your click. Penny's run has no tools, and the admin token never reaches her. The app also
won't send a draft that still has one of her `[[ask Nirav: ...]]` notes in it.

Business HQ (the green **Business HQ** button in the farm app, or click the Post Office) has these tabs:
- **Today:** sales, orders to ship, stock left, AI-plan subscribers, waitlist, inbox, and what needs you.
- **Inbox:** customer e-mail with Penny's drafts.
- **Orders:** paid orders with addresses. "Mark shipped" e-mails the customer the tracking link.
- **Money:** Stripe balance and payouts.
- **Website:** Worker up or down, AI solves and cost, accounts, calculators.
- **Launch steps:** read live from the Worker's settings.
- **Settings:** the Worker address and admin token.

Fern (the farm's planner) now also sees the business numbers, so the farm plans around how the business is really
doing.

## Part 1: get the e-mail address (about 45 minutes, about $11/year)

Domain: `prooflinecalc.org`. The site already uses `hello@prooflinecalc.org`; these steps make that address work.

1. **Domain: `prooflinecalc.org` (bought 2026-11-19).** It must be active in the same Cloudflare account as the
   Worker. Bought on Cloudflare: it already is. Bought somewhere else: Cloudflare → **Add a domain** →
   `prooflinecalc.org` → Free plan → copy the two nameservers it shows → at the shop you bought from, replace the
   domain's nameservers with those two → wait until Cloudflare says **Active** (minutes to a few hours).
2. **Put the website on it.** The code is ready: `web/api/wrangler.toml` lists `prooflinecalc.org` and
   `www.prooflinecalc.org` as custom domains, and `npm run deploy` now sets `PUBLIC_URL` to `https://prooflinecalc.org`.
   - Deploy (step 8). Cloudflare makes the DNS records and the certificate; the site works within a few minutes.
   - Send www to the root: dashboard → `prooflinecalc.org` → **Rules → Overview → Create rule → Redirect Rules** →
     template **Redirect from WWW to root** → Deploy. (Sign-in only accepts the root address.)
   - The old `ai-calc-api.ai-calc-api.workers.dev` address keeps working for the calculators, the farm and the Stripe
     webhook, so nothing there has to change. Only browser sign-in moves to the new address.
   - Stripe → **Settings → Business details → Website**: change it to `https://prooflinecalc.org`.
3. **Turn on Email Routing:** dashboard → your domain → **Email → Email Routing → Enable**. Click **Add records** when
   it offers them (MX and SPF).
4. **Verify your Gmail as a destination:** Email Routing → **Destination addresses → Add** → your Gmail address → open
   Cloudflare's e-mail in Gmail and click the link.
5. **Send hello@ to the Worker:** Email Routing → **Routing rules → Create address**: custom address `hello`, action
   **Send to a Worker**, Worker `ai-calc-api` → Save. Do the same for `support` if you like. Leave the catch-all off,
   so spam to random addresses gets rejected.
6. **Make replies come from your domain:** Resend → **Domains → Add domain** → `prooflinecalc.org` → add the DNS records it
   shows (Resend can add them to Cloudflare for you) → **Verify**. The free plan is about 3,000 e-mails a month and 100
   a day, plenty for now.
7. **Set the Worker's mail settings** in `web/api/wrangler.toml` (none of these are secrets):
   ```toml
   MAIL_FROM = "Proofline <login@prooflinecalc.org>"
   MAIL_REPLY_FROM = "Proofline <hello@prooflinecalc.org>"
   MAIL_FORWARD_TO = "your.gmail@gmail.com"   # the address you verified in step 4
   ```
8. **Update the database and deploy** (from this worktree's `web\api`):
   ```bash
   npx wrangler d1 migrations apply DB --remote
   ```
   ```bash
   npm.cmd run deploy
   ```
9. **Contact address:** done 2026-11-19, the site already says `hello@prooflinecalc.org`.
10. **Test it:** from your personal e-mail, send "Where is my order?" to `hello@prooflinecalc.org`.
    - Within a minute it should be in your Gmail (the forwarded copy) and in **Business HQ → Inbox** ("Penny is on it").
    - A minute or two later, Penny's draft appears. Edit it, press **Approve & send**, and check that the reply
      arrives from `hello@prooflinecalc.org`.

Optional: to reply from Gmail too, use Gmail → Settings → Accounts → **Send mail as** → `hello@prooflinecalc.org` with
SMTP server `smtp.resend.com`, port 465, user `resend`, and a Resend API key as the password.

## Part 2: connect the farm (2 minutes)

1. Open **Proofline Farm** from the desktop. The shortcut now opens the upgraded copy in this worktree.
2. **Business HQ → Settings:** the address is `https://ai-calc-api.ai-calc-api.workers.dev` (change it after step 2
   above). For the token, the app uses `%USERPROFILE%\proofline-admin-token.txt` if that file exists. To keep the
   token encrypted instead, paste it here and press **Save & reconnect**; it's stored with Windows' own encryption
   (DPAPI). The farm restarts its link by itself.
3. Until the new Worker code is deployed (step 8), Business HQ shows "not found": the Worker doesn't have
   `/v1/admin/business` yet.
4. **Tell Penny your rules:** **Inbox → Penny's facts**. She only states what's written there plus the customer's own
   order. Keep it true: price, ship time, refunds, warranty.

## How Penny works
- Every minute the farm asks the Worker for new mail.
  - Each mail gets one run with no tools, on Sonnet, using your normal plan usage.
  - The prompt has your facts, the store's state (open or closed, ship note, stock), that customer's orders and
    subscription (no address), and the earlier messages in the conversation.
  - The e-mail itself is labelled as untrusted text.
- She answers with a category (order, shipping, support, refund, sales, press, spam, other), a one-line summary, and a
  draft. If the mail needs no reply (spam, a thank-you), the draft is empty: press **Ignore**, or write your own reply.
- **Ask Penny to redo it** with a note ("offer a replacement", "shorter") → she drafts it again on her next pass.
- Automatic mail is shown but never drafted or answered: auto-replies, bounces, newsletters.
- Penny stops when the farm is paused, at a usage limit, or when the internet is down. The Worker still stores and
  forwards every mail, so nothing is lost.
- You get a Windows notification when a draft is ready and when a new order comes in.

## Worker API (all need the `x-admin-token` header)
| Route | What |
|---|---|
| `GET /v1/admin/business` | the dashboard numbers (counts and sums only, no customer details) and which settings are set |
| `GET /v1/admin/stripe` | balance and the last 5 payouts. The Worker's key needs **Balance: read** and **Payouts: read** (add them to the restricted key) |
| `GET /v1/admin/orders?view=to_ship\|recent` | orders with shipping address |
| `POST /v1/admin/orders/:id/ship` `{carrier, tracking, notify}` | marks shipped and e-mails the customer the tracking link |
| `GET /v1/admin/mail?view=open\|new\|all` | the inbox |
| `GET /v1/admin/mail/:id` | one mail, its conversation, the sender's orders and plan |
| `POST /v1/admin/mail/:id` `{action: draft\|ignore\|reopen}` | save a draft, ignore, reopen |
| `POST /v1/admin/mail/:id/send` `{text}` | send the approved reply (once; a second click is refused) |

Limits: at most 300 stored mails a day (more are only forwarded), a 20,000-character body (attachments aren't kept;
the Gmail copy has them), and mail is deleted after 2 years.
