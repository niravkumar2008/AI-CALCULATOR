#!/usr/bin/env node
// Publishes a calculator firmware image for over-the-air updates.
//
//   set ADMIN_TOKEN=...            (PowerShell: $env:ADMIN_TOKEN = "...")
//   node scripts/publish-firmware.mjs <firmware.bin> <version> [api-url]
//
// api-url defaults to $API_URL, else http://localhost:8787. The server checks the size
// (100 KB to 0x1E0000 bytes) and the 0xE9 magic byte, and computes the SHA-256 itself.
import { readFile } from "node:fs/promises";
import { createHash } from "node:crypto";

const [file, version, urlArg] = process.argv.slice(2);
if (!file || !version) {
  console.error("Usage: node scripts/publish-firmware.mjs <firmware.bin> <version> [api-url]");
  process.exit(2);
}
if (!/^[0-9A-Za-z.\-]{1,32}$/.test(version)) {
  console.error("version must be 1-32 characters from 0-9 A-Z a-z . -  (e.g. stage14-2026.10.08)");
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
const image = await readFile(file);
const localSha = createHash("sha256").update(image).digest("hex");

const res = await fetch(`${base}/v1/admin/firmware?version=${encodeURIComponent(version)}`, {
  method: "POST",
  headers: { "x-admin-token": token, "content-type": "application/octet-stream" },
  body: image,
});
const text = await res.text();
if (res.status !== 201) {
  console.error(`Publish failed: HTTP ${res.status} ${text}`);
  process.exit(1);
}
const out = JSON.parse(text);
if (out.sha256 !== localSha) {
  console.error(`SHA-256 mismatch! local ${localSha}, server ${out.sha256}`);
  process.exit(1);
}
console.log(`Published ${out.version}: ${out.size} bytes, sha256 ${out.sha256}`);
