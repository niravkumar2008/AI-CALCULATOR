-- Stripe integration review (docs/web/STRIPE_REVIEW_2026-10-14.md), fixes 1, 3, 4 and 7.
-- All new columns are nullable or defaulted: rows from before this migration keep working.

-- Fix 1 (G5): a renewal that needs the customer to confirm the payment (3-D Secure / SCA).
-- Set by invoice.payment_action_required while the subscription is past_due / incomplete /
-- unpaid; cleared as soon as Stripe reports the subscription in any other state.
ALTER TABLE subscriptions ADD COLUMN payment_action_at  INTEGER;  -- when we learned of it
ALTER TABLE subscriptions ADD COLUMN payment_action_url TEXT;     -- the invoice's hosted_invoice_url (https://invoice.stripe.com/...)

-- Fix 3 (G9): Stripe replays a stored 5xx for the same Idempotency-Key for 24 hours. After a
-- failed (5xx / timeout) customer create, the next attempt uses `customer-<user id>-r<n>`.
-- Also bumped when the customer is deleted at Stripe (customer.deleted), so a new customer
-- is never answered with the deleted one's cached result.
ALTER TABLE users ADD COLUMN stripe_customer_attempt INTEGER NOT NULL DEFAULT 0;

-- Fix 4 (G6): refunds and disputes point at a PaymentIntent, not the Checkout Session.
ALTER TABLE orders ADD COLUMN payment_intent  TEXT;     -- pi_... from checkout.session.completed
ALTER TABLE orders ADD COLUMN amount_refunded INTEGER;  -- cents, from charge.refunded
CREATE INDEX orders_payment_intent ON orders (payment_intent) WHERE payment_intent IS NOT NULL;

-- Fix 7 (G9): the hourly cron deletes processed event ids older than 30 days.
CREATE INDEX stripe_events_received ON stripe_events (received_at);
