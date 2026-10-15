import { fileURLToPath } from "node:url";
import { cloudflareTest, readD1Migrations } from "@cloudflare/vitest-plugin";
import { defineConfig } from "vitest/config";

export default defineConfig(async () => {
  const migrations = await readD1Migrations(fileURLToPath(new URL("./migrations", import.meta.url)));
  return {
    plugins: [
      cloudflareTest({
        main: "./src/index.ts",
        wrangler: { configPath: "./wrangler.toml" },
        miniflare: {
          // Test-only settings. Upstream calls are intercepted by a fetch spy (test/helpers.ts).
          bindings: {
            TEST_MIGRATIONS: migrations,
            ANTHROPIC_API_KEY: "sk-test-not-a-real-key",
            ANTHROPIC_BASE_URL: "https://anthropic.test",
            ADMIN_TOKEN: "admin-test-token-0123456789abcdefghijkl", // >= 32 chars (S10)
            DEV_MODE: "1",
            // Needed by the tests that switch DEV_MODE off (production rules apply then).
            TRIAL_PEPPER: "test-trial-pepper-0123456789abcdefghijklmnop",
            // Stage 3: a fake Stripe at https://stripe.test (intercepted in test/stripe-fake.ts).
            STRIPE_SECRET_KEY: "sk_test_unitTestKey0123456789",
            STRIPE_WEBHOOK_SECRET: "whsec_unitTestSecret0123456789abcdef",
            STRIPE_PRICE_SUB: "price_subMonthly15",
            STRIPE_PRICE_SUB_YEARLY: "price_subYearly100",
            STRIPE_PRICE_DEVICE: "price_device225",
            STRIPE_API_BASE: "https://stripe.test",
            // Required with a key unless DEV_MODE=1 (review G4); tests switch DEV_MODE off.
            STRIPE_API_VERSION: "2026-09-30.clover",
            // Optional (review G3, G7): empty = account default portal, no shipping option.
            STRIPE_PORTAL_CONFIG: "",
            STRIPE_SHIPPING_RATE: "",
            STORE_OPEN: "1",
          },
        },
      }),
    ],
    test: {
      setupFiles: ["./test/setup.ts"],
    },
  };
});
