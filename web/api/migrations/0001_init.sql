-- Initial schema (docs/web/PLAN.md section 3).
-- Money is integer micro-USD, times are integer Unix seconds.

CREATE TABLE users (
  id                 TEXT PRIMARY KEY,                 -- 'usr_' + 16 hex
  email              TEXT NOT NULL UNIQUE COLLATE NOCASE, -- 'deleted:<id>' after account deletion
  created_at         INTEGER NOT NULL,
  trial_ends_at      INTEGER,
  stripe_customer_id TEXT UNIQUE,
  deleted_at         INTEGER,
  plan_override      TEXT                              -- set only by /__dev/user (local tests)
);

CREATE TABLE login_tokens (
  token_hash TEXT PRIMARY KEY,                         -- SHA-256 hex of the e-mailed token
  email      TEXT NOT NULL COLLATE NOCASE,
  created_at INTEGER NOT NULL,
  expires_at INTEGER NOT NULL,                         -- 15 min
  used_at    INTEGER                                   -- single use
);
CREATE INDEX login_tokens_expires ON login_tokens (expires_at);

CREATE TABLE sessions (
  id_hash    TEXT PRIMARY KEY,                         -- SHA-256 hex of the cookie value
  user_id    TEXT NOT NULL REFERENCES users (id),
  created_at INTEGER NOT NULL,
  expires_at INTEGER NOT NULL                          -- 30 days
);
CREATE INDEX sessions_user ON sessions (user_id);
CREATE INDEX sessions_expires ON sessions (expires_at);

CREATE TABLE devices (
  id           TEXT PRIMARY KEY,                       -- 'dev_' + 16 hex
  token_hash   TEXT NOT NULL UNIQUE,                   -- SHA-256 hex of 'dt_...'
  hw_id        TEXT,
  fw_version   TEXT,
  user_id      TEXT REFERENCES users (id),
  label        TEXT,
  created_at   INTEGER NOT NULL,
  last_seen_at INTEGER,
  revoked_at   INTEGER
);
CREATE INDEX devices_user ON devices (user_id, revoked_at);
CREATE INDEX devices_unlinked ON devices (user_id, last_seen_at) WHERE user_id IS NULL;

CREATE TABLE pair_codes (
  code       TEXT PRIMARY KEY,                         -- 8 chars, stored without the dash
  device_id  TEXT NOT NULL REFERENCES devices (id),
  expires_at INTEGER NOT NULL,                         -- 15 min
  claimed_at INTEGER
);
CREATE INDEX pair_codes_device ON pair_codes (device_id);

CREATE TABLE subscriptions (
  id                   TEXT PRIMARY KEY,               -- Stripe subscription id (stage 3)
  user_id              TEXT NOT NULL REFERENCES users (id),
  status               TEXT NOT NULL,
  price_id             TEXT,
  current_period_end   INTEGER,
  cancel_at_period_end INTEGER NOT NULL DEFAULT 0,
  updated_at           INTEGER NOT NULL
);
CREATE INDEX subscriptions_user ON subscriptions (user_id, updated_at);

CREATE TABLE orders (
  id            TEXT PRIMARY KEY,                      -- Stripe Checkout session id (stage 3)
  email         TEXT,
  user_id       TEXT,
  amount_total  INTEGER,
  currency      TEXT,
  status        TEXT,
  shipping_json TEXT,
  created_at    INTEGER NOT NULL
);

CREATE TABLE usage (
  id             TEXT PRIMARY KEY,                     -- 'use_' + 16 hex
  device_id      TEXT NOT NULL,
  user_id        TEXT NOT NULL,
  started_at     INTEGER NOT NULL,
  state          TEXT NOT NULL CHECK (state IN ('reserved', 'settled', 'failed')),
  reserved_micro INTEGER NOT NULL,                     -- worst case, held while in flight
  cost_micro     INTEGER NOT NULL DEFAULT 0,           -- actual, after settlement
  model          TEXT,
  input_tokens   INTEGER,
  output_tokens  INTEGER,
  day            TEXT NOT NULL,                        -- 'YYYY-MM-DD' (UTC), for spend_daily
  month          TEXT NOT NULL                         -- 'YYYY-MM' (UTC), for monthly limits
);
CREATE INDEX usage_device_state ON usage (device_id, state, started_at);
CREATE INDEX usage_user_month ON usage (user_id, month, state);

CREATE TABLE spend_daily (
  day            TEXT PRIMARY KEY,                     -- 'YYYY-MM-DD' (UTC)
  reserved_micro INTEGER NOT NULL DEFAULT 0,           -- open reservations
  settled_micro  INTEGER NOT NULL DEFAULT 0            -- actual cost of finished solves
);

CREATE TABLE stripe_events (
  id          TEXT PRIMARY KEY,
  type        TEXT NOT NULL,
  received_at INTEGER NOT NULL
);

-- Fixed-window counters for rate limits (no paid Rate Limiting binding needed).
CREATE TABLE rate_limits (
  key          TEXT NOT NULL,
  window_start INTEGER NOT NULL,
  count        INTEGER NOT NULL,
  PRIMARY KEY (key, window_start)
);
CREATE INDEX rate_limits_window ON rate_limits (window_start);

-- Sign-in e-mails captured when DEV_MODE=1 (read by GET /__dev/outbox in local tests).
CREATE TABLE dev_outbox (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  email      TEXT NOT NULL COLLATE NOCASE,
  link       TEXT NOT NULL,
  token      TEXT NOT NULL,
  created_at INTEGER NOT NULL
);
CREATE INDEX dev_outbox_email ON dev_outbox (email, id);
