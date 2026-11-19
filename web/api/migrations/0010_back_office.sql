-- Back office (the Proofline Farm "Business HQ", docs/web/BACK_OFFICE.md):
-- 1. customer e-mail that arrives through Cloudflare Email Routing -> this Worker's email() handler;
-- 2. shipping for store orders (carrier + tracking, and when the customer was told).

CREATE TABLE mail (
  id           TEXT PRIMARY KEY,                -- 'mail_' + 16 hex
  received_at  INTEGER NOT NULL,
  from_addr    TEXT NOT NULL COLLATE NOCASE,
  from_name    TEXT,
  to_addr      TEXT,
  subject      TEXT,
  body         TEXT,                            -- plain text, at most 20,000 characters (files aren't kept)
  message_id   TEXT,
  thread_id    TEXT NOT NULL,                   -- the first mail's id in a conversation
  attachments  TEXT,                            -- JSON [{name, type, size}] (names only)
  auto         INTEGER NOT NULL DEFAULT 0,      -- 1 = auto-reply, bounce or bulk mail (no draft, no reply)
  status       TEXT NOT NULL CHECK (status IN ('new', 'drafted', 'replied', 'ignored')),
  category     TEXT,                            -- the drafter's guess: order, shipping, support, refund, sales, press, spam, other
  summary      TEXT,                            -- the drafter's one-line summary
  draft        TEXT,
  drafted_at   INTEGER,
  reply_text   TEXT,                            -- what was really sent (after Nirav's edits)
  replied_at   INTEGER
);
CREATE INDEX mail_status ON mail (status, received_at);
CREATE INDEX mail_from ON mail (from_addr, received_at);
CREATE INDEX mail_thread ON mail (thread_id, received_at);
CREATE INDEX mail_received ON mail (received_at);

ALTER TABLE orders ADD COLUMN shipped_at       INTEGER;
ALTER TABLE orders ADD COLUMN carrier          TEXT;     -- usps, ups, fedex, dhl, other
ALTER TABLE orders ADD COLUMN tracking         TEXT;
ALTER TABLE orders ADD COLUMN ship_notified_at INTEGER;  -- the "it shipped" e-mail went out
