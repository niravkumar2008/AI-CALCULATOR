-- Stage 3: the launch-site waitlist (POST /api/waitlist).
-- Stripe tables (subscriptions, orders, stripe_events) already exist from 0001.

CREATE TABLE waitlist (
  email      TEXT PRIMARY KEY COLLATE NOCASE,   -- lower-cased; one row per address
  source     TEXT NOT NULL,                     -- which form: hero, footer, site
  created_at INTEGER NOT NULL
);
CREATE INDEX waitlist_created ON waitlist (created_at);
