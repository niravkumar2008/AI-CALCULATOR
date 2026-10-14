-- Board enrolment and once-only free months (security review S1, PLAN section 4c).

-- Boards enrolled at QA: hw_id plus SHA-256 of the per-board secret written to NVS.
CREATE TABLE hw_allow (
  hw_id         TEXT PRIMARY KEY,
  secret_hash   TEXT NOT NULL,
  enrolled_at   INTEGER NOT NULL,
  trial_used_at INTEGER                 -- set once, never reset (also not by re-enrolment)
);

-- E-mails (SHA-256 of the lower-cased address) that ever got a free month. Kept after
-- account deletion, so deleting and re-creating an account doesn't give a second month.
CREATE TABLE trials_used (
  email_hash TEXT PRIMARY KEY,
  used_at    INTEGER NOT NULL
);
CREATE INDEX trials_used_at ON trials_used (used_at);

-- 1 when the device registered with a matching (hw_id, hw_secret) pair.
ALTER TABLE devices ADD COLUMN enrolled INTEGER NOT NULL DEFAULT 0;
