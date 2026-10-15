#!/usr/bin/env node
// Enrols one calculator board at QA (security review S1, PLAN section 4c).
//
//   set ADMIN_TOKEN=...            (PowerShell: $env:ADMIN_TOKEN = "...")
//   node scripts/enroll-board.mjs <hw_id> [api-url]
//
// hw_id is the board's id as it reports it, e.g. calc-0123456789ab (MAC-derived).
// The script makes a new random secret, registers its SHA-256 with the server
// (POST /v1/admin/hardware), and prints the serial command that writes the secret into the
// board's NVS namespace `factory` (the `enroll` serial command arrives with stage-2 firmware).
// api-url defaults to $API_URL, else http://localhost:8787. Plain http is refused except
// to localhost / 127.0.0.1, so the admin token never crosses the internet unencrypted.
import { randomBytes } from "node:crypto";

const [hwId, urlArg] = process.argv.slice(2);
if (!hwId || !/^calc-[0-9a-f]{12}$/.test(hwId)) {
  console.error("Usage: node scripts/enroll-board.mjs <hw_id like calc-0123456789ab> [api-url]");
  process.exit(2);
}
const token = process.env.ADMIN_TOKEN;
if (!token) {
  console.error("Set ADMIN_TOKEN in the environment first (the same value as the Worker's ADMIN_TOKEN secret).");
  process.exit(2);
}
const base = (urlArg || process.env.API_URL || "http://localhost:8787").replace(/\/+$/, "");
const u = new URL(base);
if (u.protocol !== "https:" && !(u.protocol === "http:" && (u.hostname === "localhost" || u.hostname === "127.0.0.1"))) {
  console.error(`Refusing ${u.protocol}//${u.host}: use https (plain http is only allowed to localhost).`);
  process.exit(2);
}

const hwSecret = "hs_" + randomBytes(32).toString("base64url");
const res = await fetch(`${base}/v1/admin/hardware`, {
  method: "POST",
  headers: { "x-admin-token": token, "content-type": "application/json" },
  body: JSON.stringify({ hw_id: hwId, hw_secret: hwSecret }),
});
const text = await res.text();
if (res.status !== 201) {
  console.error(`Enrol failed: HTTP ${res.status} ${text}`);
  process.exit(1);
}
const out = JSON.parse(text);
console.log(`Enrolled ${out.hw_id} (free month already used: ${out.trial_used ? "yes" : "no"}).`);
console.log("Now type this on the board's serial console (USB, 115200 baud):");
console.log("");
console.log(`enroll ${hwSecret}`);
console.log("");
console.log("Keep the secret only on the board. If it is lost, run this script again (the free-month state is kept).");
