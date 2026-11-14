-- QR linking, way 2 (docs/LINK_QR_CONTRACT.md): the signed-in phone shows a link token as a
-- QR, the calculator redeems it (POST /v1/pair/redeem), the phone taps Approve.
-- Only the token's SHA-256 is stored. The hourly cron deletes rows older than 1 day.

CREATE TABLE link_tokens (
  id                TEXT PRIMARY KEY,                 -- 'lt_' + 16 hex
  user_id           TEXT NOT NULL,
  token_hash        TEXT NOT NULL UNIQUE,             -- SHA-256 hex of 'L1.<22 base64url>'
  created_at        INTEGER,
  expires_at        INTEGER,                          -- created_at + 10 min
  used_at           INTEGER,                          -- redeemed by a calculator (single use)
  used_by_device_id TEXT,
  approve_by        INTEGER,                          -- used_at + 5 min: the phone must tap by then
  approved_at       INTEGER,
  declined_at       INTEGER,
  cancelled_at      INTEGER,                          -- replaced by a newer token of the same user
  ip                TEXT                              -- rate-limit key of the phone that made it
);
CREATE INDEX link_tokens_user ON link_tokens (user_id, created_at);
CREATE INDEX link_tokens_device ON link_tokens (used_by_device_id, used_at) WHERE used_by_device_id IS NOT NULL;
CREATE INDEX link_tokens_created ON link_tokens (created_at);

-- The dev outbox (DEV_MODE only) now also holds the "A calculator was linked" notice.
-- GET /__dev/outbox returns 'login' rows by default and the notice with ?kind=linked.
ALTER TABLE dev_outbox ADD COLUMN kind TEXT NOT NULL DEFAULT 'login';
