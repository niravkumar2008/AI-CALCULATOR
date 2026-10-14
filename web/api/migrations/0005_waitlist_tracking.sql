-- Waitlist: per-channel tracking, a referral code, and the 13+ confirmation.
-- All new columns are nullable: rows from before this migration have none of them.
-- Values are checked by the API (lower-case, [a-z0-9_-]{1,40}); anything else is stored as NULL.

ALTER TABLE waitlist ADD COLUMN utm_source   TEXT;    -- e.g. tiktok, youtube, c-alex (also ?src=)
ALTER TABLE waitlist ADD COLUMN utm_medium   TEXT;    -- e.g. video, paid, creator
ALTER TABLE waitlist ADD COLUMN utm_campaign TEXT;    -- e.g. launch-week
ALTER TABLE waitlist ADD COLUMN ref          TEXT;    -- referral code from ?ref= (no rewards yet)
ALTER TABLE waitlist ADD COLUMN age_ok       INTEGER; -- 1 = ticked "I'm 13 or older"; NULL = rows from before

CREATE INDEX waitlist_utm ON waitlist (utm_source, utm_medium, utm_campaign);
CREATE INDEX waitlist_ref ON waitlist (ref) WHERE ref IS NOT NULL;
