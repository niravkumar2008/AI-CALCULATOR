#!/usr/bin/env node
// One-command Stripe setup for the Proofline store and plan (GUIDE sections 4.1, 4.4 and 4.7).
//
// Run it in YOUR OWN terminal (PowerShell), never inside a chat, because it prints the webhook
// signing secret once:
//
//   cd web\api
//   $env:STRIPE_SECRET_KEY = "sk_live_..."     # or sk_test_... for a sandbox; Dashboard -> Developers -> API keys
//   node scripts/stripe-setup.mjs --site https://ai-calc-api.ai-calc-api.workers.dev
//   Remove-Item Env:STRIPE_SECRET_KEY          # forget the key again
//
// What it makes (each only once; running it again finds what's there and makes nothing twice):
//   - product "Proofline calculator", price $215.00 one-time, tax added on top
//   - product "AI tutor plan", prices $15/month and $120/year
//   - shipping rates: Standard $8.95 and Priority $14.95 (change with --standard / --priority)
//   - a Customer Portal configuration (cancel at period end, switch monthly <-> yearly)
//   - the webhook endpoint at <site>/api/stripe/webhook with every event the Worker reads
// Then it prints the wrangler.toml lines and the one secret to set.
//
// It does NOT: activate the account, add a bank, turn on Stripe Tax, or make the restricted key
// the Worker uses (Dashboard steps; see docs/web/GUIDE.md section 4).

const args = parseArgs(process.argv.slice(2));
if (args.help) {
  console.log("node scripts/stripe-setup.mjs --site https://your.site [--standard 8.95] [--priority 14.95] [--no-priority]");
  process.exit(0);
}

const KEY = (process.env.STRIPE_SECRET_KEY ?? "").trim();
if (!/^sk_(test|live)_[A-Za-z0-9]+$/.test(KEY)) {
  fail("Set STRIPE_SECRET_KEY to your secret key (sk_live_... or sk_test_...) first. A restricted rk_ key can't create products.");
}
const LIVE = KEY.startsWith("sk_live_");
const SITE = String(args.site ?? "").replace(/\/+$/, "");
if (!/^https:\/\/[^/]+$/.test(SITE) && !(args["api-base"] && /^http:\/\/127\.0\.0\.1(:\d+)?$/.test(SITE))) {
  fail('--site must be the site\'s https address with no path, e.g. --site https://ai-calc-api.ai-calc-api.workers.dev');
}
const API = String(args["api-base"] ?? "https://api.stripe.com").replace(/\/+$/, "");
if (API !== "https://api.stripe.com" && !/^http:\/\/127\.0\.0\.1:\d+$/.test(API)) fail("--api-base is only for local tests");
const STANDARD = cents(args.standard ?? "8.95", "--standard");
const PRIORITY = args["no-priority"] ? null : cents(args.priority ?? "14.95", "--priority");

const TAG = "proofline_setup"; // metadata key on everything this script makes
const EVENTS = [
  "checkout.session.completed",
  "checkout.session.async_payment_succeeded",
  "checkout.session.async_payment_failed",
  "checkout.session.expired",
  "customer.subscription.created",
  "customer.subscription.updated",
  "customer.subscription.deleted",
  "customer.subscription.paused",
  "customer.subscription.resumed",
  "invoice.paid",
  "invoice.payment_failed",
  "invoice.payment_action_required",
  "customer.deleted",
  "charge.refunded",
  "charge.dispute.created",
  "charge.dispute.closed",
];

let apiVersion = ""; // the account's default version, read from the first response

async function main() {
  console.log(`\nStripe setup in ${LIVE ? "LIVE mode (real money)" : "a sandbox / test mode"} for ${SITE}\n`);

  // 1. The account's API version: the webhook and the Worker must use the same one (GUIDE 4.7).
  await stripe("GET", "/v1/products", { limit: 1 });
  if (!/^\d{4}-\d{2}-\d{2}(\.[a-z]+)?$/.test(apiVersion)) fail("Stripe didn't say which API version this account uses.");
  console.log(`API version: ${apiVersion}`);

  // 2. Products and prices (found again by lookup_key).
  const calc = await price("proofline_calculator_215", {
    product: { name: "Proofline calculator", tax_code: "txcd_99999999" },
    unit_amount: 21500,
  });
  const plan = await product("proofline_plan", { name: "AI tutor plan", tax_code: "txcd_10103000" });
  const monthly = await price("proofline_plan_month_15", { product: plan, unit_amount: 1500, recurring: { interval: "month" } });
  const yearly = await price("proofline_plan_year_120", { product: plan, unit_amount: 12000, recurring: { interval: "year" } });

  // 3. Shipping rates.
  const rates = [await shippingRate("standard", "Standard shipping (USPS Ground Advantage)", STANDARD)];
  if (PRIORITY !== null) rates.push(await shippingRate("priority", "Priority shipping (USPS Priority Mail)", PRIORITY));

  // 4. Customer Portal configuration (GUIDE 4.4).
  const portal = await portalConfig(plan, [monthly, yearly]);

  // 5. Webhook endpoint (GUIDE 4.7).
  const hook = await webhook();

  console.log("\nDone. Paste these into web/api/wrangler.toml under [vars]:\n");
  console.log(`STRIPE_PRICE_DEVICE = "${calc}"`);
  console.log(`STRIPE_PRICE_SUB = "${monthly}"`);
  console.log(`STRIPE_PRICE_SUB_YEARLY = "${yearly}"`);
  console.log(`STRIPE_SHIPPING_RATE = "${rates.join(",")}"`);
  console.log(`STRIPE_PORTAL_CONFIG = "${portal}"`);
  console.log(`STRIPE_API_VERSION = "${apiVersion}"`);
  if (hook.secret) {
    console.log("\nThen, in web/api, run this and paste the secret below when it asks (it is shown only now):");
    console.log("  npx wrangler secret put STRIPE_WEBHOOK_SECRET");
    console.log(`  secret: ${hook.secret}`);
  } else {
    console.log("\nThe webhook already existed, so its secret isn't shown again. If STRIPE_WEBHOOK_SECRET isn't set on the");
    console.log("Worker yet: Dashboard -> Workbench -> Webhooks -> this endpoint -> Signing secret -> Reveal, then");
    console.log("  npx wrangler secret put STRIPE_WEBHOOK_SECRET");
  }
  console.log("\nAnd the key the Worker uses (a restricted key, GUIDE 4.6): npx wrangler secret put STRIPE_SECRET_KEY");
  console.log('Last: STORE_OPEN = "1" in wrangler.toml, then npm.cmd run deploy.\n');
}

// ------------------------------------------------------------ objects

async function product(key, fields) {
  // A list, not search: search lags about a minute, so a quick second run would make it twice.
  const found = await stripe("GET", "/v1/products", { active: "true", limit: 100 });
  const hit = (found.data ?? []).find((p) => p.metadata?.[TAG] === key);
  if (hit) return say("product", fields.name, hit.id, false);
  const p = await stripe("POST", "/v1/products", { ...fields, metadata: { [TAG]: key } }, `${TAG}-product-${key}`);
  return say("product", fields.name, p.id, true);
}

async function price(lookupKey, { product: prod, unit_amount, recurring }) {
  const found = await stripe("GET", "/v1/prices", { "lookup_keys[]": lookupKey, active: "true", limit: 1 });
  const hit = found.data?.[0];
  const label = `${dollars(unit_amount)}${recurring ? ` / ${recurring.interval}` : " one-time"}`;
  if (hit) {
    if (hit.unit_amount !== unit_amount) fail(`price ${lookupKey} exists with another amount (${dollars(hit.unit_amount)}); archive it in the Dashboard first.`);
    return say("price", label, hit.id, false);
  }
  const productField = typeof prod === "string" ? { product: prod } : { product_data: { name: prod.name, tax_code: prod.tax_code, metadata: { [TAG]: lookupKey } } };
  const p = await stripe(
    "POST",
    "/v1/prices",
    { ...productField, currency: "usd", unit_amount, recurring, tax_behavior: "exclusive", lookup_key: lookupKey, metadata: { [TAG]: lookupKey } },
    `${TAG}-price-${lookupKey}`,
  );
  return say("price", label, p.id, true);
}

async function shippingRate(key, name, amount) {
  const list = await stripe("GET", "/v1/shipping_rates", { active: "true", limit: 100 });
  const hit = (list.data ?? []).find((r) => r.metadata?.[TAG] === key && r.fixed_amount?.amount === amount);
  if (hit) return say("shipping rate", `${name} ${dollars(amount)}`, hit.id, false);
  const r = await stripe(
    "POST",
    "/v1/shipping_rates",
    {
      display_name: name,
      type: "fixed_amount",
      fixed_amount: { amount, currency: "usd" },
      tax_behavior: "exclusive",
      tax_code: "txcd_92010001",
      metadata: { [TAG]: key },
    },
    `${TAG}-ship-${key}-${amount}`,
  );
  return say("shipping rate", `${name} ${dollars(amount)}`, r.id, true);
}

async function portalConfig(plan, prices) {
  const list = await stripe("GET", "/v1/billing_portal/configurations", { active: "true", limit: 100 });
  const hit = (list.data ?? []).find((c) => c.metadata?.[TAG] === "portal");
  if (hit) return say("portal configuration", "Manage billing", hit.id, false);
  const c = await stripe(
    "POST",
    "/v1/billing_portal/configurations",
    {
      business_profile: { privacy_policy_url: `${SITE}/privacy/`, terms_of_service_url: `${SITE}/terms/` },
      default_return_url: `${SITE}/account/`,
      features: {
        customer_update: { enabled: true, allowed_updates: ["address"] },
        invoice_history: { enabled: true },
        payment_method_update: { enabled: true },
        subscription_cancel: { enabled: true, mode: "at_period_end", cancellation_reason: { enabled: true, options: ["too_expensive", "unused", "missing_features", "other"] } },
        subscription_update: {
          enabled: true,
          default_allowed_updates: ["price"],
          proration_behavior: "create_prorations",
          products: [{ product: plan, prices }],
        },
      },
      metadata: { [TAG]: "portal" },
    },
    `${TAG}-portal`,
  );
  return say("portal configuration", "Manage billing", c.id, true);
}

async function webhook() {
  const url = `${SITE}/api/stripe/webhook`;
  const list = await stripe("GET", "/v1/webhook_endpoints", { limit: 100 });
  const hit = (list.data ?? []).find((w) => w.url === url && w.status !== "disabled");
  if (hit) {
    if (hit.api_version && hit.api_version !== apiVersion) {
      fail(`the webhook at ${url} uses API version ${hit.api_version}, the account uses ${apiVersion}. Use ${hit.api_version} for STRIPE_API_VERSION, or delete that endpoint and run this again.`);
    }
    const missing = EVENTS.filter((e) => !hit.enabled_events?.includes(e) && !hit.enabled_events?.includes("*"));
    if (missing.length) {
      await stripe("POST", `/v1/webhook_endpoints/${hit.id}`, { enabled_events: [...new Set([...(hit.enabled_events ?? []), ...EVENTS])] });
      console.log(`  webhook: added ${missing.length} event(s)`);
    }
    say("webhook", url, hit.id, false);
    return { id: hit.id, secret: "" };
  }
  const w = await stripe(
    "POST",
    "/v1/webhook_endpoints",
    { url, enabled_events: EVENTS, api_version: apiVersion, description: "Proofline Worker", metadata: { [TAG]: "webhook" } },
    `${TAG}-webhook-${url}`,
  );
  say("webhook", url, w.id, true);
  return { id: w.id, secret: w.secret ?? "" };
}

// ------------------------------------------------------------ helpers

async function stripe(method, path, params = {}, idempotencyKey) {
  const body = encode(params);
  const url = method === "GET" && body ? `${API}${path}?${body}` : `${API}${path}`;
  const headers = { authorization: `Bearer ${KEY}` };
  if (apiVersion) headers["stripe-version"] = apiVersion;
  if (method !== "GET") headers["content-type"] = "application/x-www-form-urlencoded";
  if (idempotencyKey) headers["idempotency-key"] = idempotencyKey;
  const res = await fetch(url, { method, headers, body: method === "GET" ? undefined : body });
  if (!apiVersion) apiVersion = res.headers.get("stripe-version") ?? "";
  const data = await res.json().catch(() => ({}));
  if (!res.ok) {
    const msg = data?.error?.message ?? `HTTP ${res.status}`;
    throw new Error(`${method} ${path}: ${msg}`);
  }
  return data;
}

/** Stripe's form encoding: a[b][0][c]=v. */
function encode(obj, prefix = "", out = []) {
  for (const [k, v] of Object.entries(obj)) {
    if (v === undefined || v === null) continue;
    const key = prefix ? `${prefix}[${k}]` : k;
    if (Array.isArray(v)) {
      v.forEach((item, i) => {
        if (item !== null && typeof item === "object") encode(item, `${key}[${i}]`, out);
        else out.push(`${encodeURIComponent(`${key}[${i}]`)}=${encodeURIComponent(String(item))}`);
      });
    } else if (typeof v === "object") {
      encode(v, key, out);
    } else {
      out.push(`${encodeURIComponent(key)}=${encodeURIComponent(String(v))}`);
    }
  }
  return out.join("&");
}

function say(kind, label, id, made) {
  console.log(`  ${made ? "made " : "found"} ${kind}: ${label} -> ${id}`);
  return id;
}

function cents(v, flag) {
  const n = Math.round(Number(v) * 100);
  if (!Number.isInteger(n) || n < 0 || n > 100000) fail(`${flag} must be a dollar amount like 8.95`);
  return n;
}

function dollars(c) {
  return `$${(c / 100).toFixed(2)}`;
}

function parseArgs(list) {
  const out = {};
  for (let i = 0; i < list.length; i++) {
    const a = list[i];
    if (!a.startsWith("--")) continue;
    const [k, v] = a.slice(2).split("=", 2);
    if (v !== undefined) out[k] = v;
    else if (list[i + 1] && !list[i + 1].startsWith("--")) out[k] = list[++i];
    else out[k] = true;
  }
  return out;
}

function fail(msg) {
  console.error(`\nStopped: ${msg}\n`);
  process.exit(1);
}

main().catch((e) => fail(e.message));
