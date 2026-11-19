-- Store stock (STORE_STOCK): how many calculators each order holds, so the Worker can stop
-- selling when a batch is sold out. Rows from before this migration count as 1.
-- An open Checkout page is held as status 'open' (with its quantity) until it is paid,
-- fails, or expires (checkout.session.expired, or 31 minutes after it was made).
ALTER TABLE orders ADD COLUMN quantity INTEGER;
CREATE INDEX IF NOT EXISTS orders_status ON orders (status, created_at);
