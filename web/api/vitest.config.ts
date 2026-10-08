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
          },
        },
      }),
    ],
    test: {
      setupFiles: ["./test/setup.ts"],
    },
  };
});
