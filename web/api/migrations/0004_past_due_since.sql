-- Stage 3 security review S3-1: the 3-day past_due grace counts from when we first saw the
-- subscription go past_due (the failed renewal), not from current_period_end. Stripe moves
-- current_period_end forward at renewal even when the charge fails, so the old rule allowed
-- about a month of unpaid AI.
ALTER TABLE subscriptions ADD COLUMN past_due_since INTEGER;
