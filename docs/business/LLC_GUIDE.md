# Should Proofline be an LLC? What you gain

Written 2026-11-18. This is general information, not legal or tax advice. A 30-minute call with an Indiana accountant
is worth it before your first big sales month.

## Right now
Without an LLC, Proofline is a **sole proprietorship**: you and the business are legally the same person. The
website's footer also says "Corporation", which isn't true until you register something. Either form the LLC, or let
Claude change the wording.

## What an LLC gives you
1. **It protects your personal money.** If the business gets sued or owes money, the claim is normally against the
   LLC's money, not your own savings, car or future wages. This matters for an electronics product with a lithium
   battery: a battery fire or injury claim is the kind of risk an LLC is for.
2. **A real business identity.** "Proofline LLC" can sign supplier deals, hold the domain and the trademark, and own
   the Stripe account. Stripe, banks, Kickstarter and wholesale suppliers take a registered business more seriously.
3. **A business bank account.** With an LLC + EIN (free from the IRS), Stripe pays out to a business account. Keeping
   business money separate makes taxes far easier, and it's required to keep the liability protection.
4. **Your SSN stays off paperwork.** You give suppliers and Stripe the LLC's EIN instead.
5. **Business insurance.** Product liability insurance (often $300–800 a year for a small electronics seller) is
   usually written for a business. Many stores and schools ask for it before they carry your product.
6. **The same taxes by default.** A one-owner LLC is "pass-through": profit goes on your personal return (Schedule C),
   the same as now. Later, at roughly $40k+ profit a year, an accountant can make it an S-corp to cut self-employment
   tax.
7. **Your name in Indiana.** No other Indiana business can register "Proofline LLC" once you have it. It's not a
   trademark, though: file a federal trademark (about $350 per class) once you're sure of the name.

## What it doesn't do
- It doesn't protect you if you mix money (paying personal bills from the business account), or if you personally do
  something careless or wrong.
- It doesn't replace insurance, or the FCC test the calculator needs before you ship it.
- It doesn't lower taxes on its own at first.

## Cost in Indiana
| Item | Cost |
|---|---|
| Articles of Organization (online at INBiz, inbiz.in.gov) | $95 (+ about $1–2 card fee) |
| Business Entity Report, every 2 years | about $32 online |
| EIN from the IRS (irs.gov, "Apply for an EIN online") | free (beware sites that charge) |
| Registered agent | free if you use yourself (needs an Indiana street address, open in business hours); about $50–125 a year for a service |
| Indiana Retail Merchant Certificate (to collect sales tax) | $25 |

## Steps (about 1–2 evenings)
1. Check that "Proofline LLC" is free: INBiz → **Business Search**.
2. INBiz → **Start a new business** → Domestic LLC → fill in the name, registered agent, your address, and "member
   managed". Pay $95. Approval is usually within 1 business day.
3. Get the EIN at irs.gov (about 10 minutes, free).
4. Write a one-page operating agreement (free templates exist). Banks often ask for it.
5. Open a business checking account with the Articles + EIN.
6. In Stripe → Settings → **Business details**: change from individual to company (LLC), add the EIN and the business
   bank account.
7. Register for Indiana sales tax with the LLC's EIN (INTIME → Retail Merchant Certificate, $25), then set
   `STRIPE_TAX = "1"`.
8. Update the site's legal name (footer, terms, privacy, refunds). Claude can do this once you have the exact name.

You need to be 18+ to sign the Stripe, bank and IRS forms yourself.
