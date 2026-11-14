-- A new solve from the same calculator replaces the one still running (the user pressed AC or
-- the connection dropped). The new request inserts the old usage id here; the old request's
-- stream checks this table every few seconds, stops reading from Claude and settles what it used.
-- The old row stays 'reserved' until then, so the spend caps keep counting it (no CHECK change).
-- The hourly cron deletes rows older than 1 day.

CREATE TABLE usage_superseded (
  usage_id      TEXT PRIMARY KEY,                     -- the replaced usage row
  superseded_at INTEGER NOT NULL,
  by_usage_id   TEXT NOT NULL                         -- the usage row that replaced it
);
