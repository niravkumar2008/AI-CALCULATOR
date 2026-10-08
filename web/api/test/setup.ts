import { applyD1Migrations } from "cloudflare:test";
import { env } from "cloudflare:workers";
import { afterEach, beforeEach, vi } from "vitest";

const TABLES = [
  "hw_allow",
  "trials_used",
  "usage",
  "spend_daily",
  "pair_codes",
  "sessions",
  "login_tokens",
  "subscriptions",
  "orders",
  "stripe_events",
  "rate_limits",
  "dev_outbox",
  "devices",
  "users",
];

await applyD1Migrations(env.DB, env.TEST_MIGRATIONS);

beforeEach(async () => {
  // Freeze the clock at the middle of the current hour (also the start of a 10-minute and a
  // 1-minute window), so fixed-window rate limits can't roll over in the middle of a test.
  const t = Math.floor(Date.now() / 3_600_000) * 3_600_000 + 1_800_000;
  vi.spyOn(Date, "now").mockReturnValue(t);
  await env.DB.batch(TABLES.map((t) => env.DB.prepare(`DELETE FROM ${t}`)));
  await env.FIRMWARE.delete("meta");
  await env.FIRMWARE.delete("image");
});

afterEach(() => {
  vi.restoreAllMocks();
});
