// POST /v1/solve: validate, reserve the worst-case cost atomically (fail closed), call Claude,
// stream the SSE reply through byte for byte, then settle the real cost (PLAN section 4).

import { type Config, type Env, STALE_INFLIGHT_SEC } from "./env";
import { newId } from "./crypto";
import { authDevice, checkLimits, checkLinkedAndEntitled } from "./device";
import { HttpError, MSG, errorBody } from "./http";
import { type TokenUsage, actualCostMicro, worstCaseMicro } from "./plan";
import { ASK_ONE_IMAGE, ASK_TWO_IMAGES, SOLVE_INSTRUCTIONS, SOLVE_SCHEMA, TUTOR_NOTE } from "./prompt";
import { dayOf, hitRateLimit, monthOf, rateLimited, readBody } from "./util";

export const MAX_BODY_BYTES = 8 * 1024 * 1024;
export const MAX_DECODED_BYTES = 5 * 1024 * 1024; // contract: 5 MiB decoded in total, else 400
export const BURST_PER_MIN = 6;
export { STALE_INFLIGHT_SEC };
/** Give up on a silent upstream stream after this long (the calculator gives up after 120 s). */
export const STREAM_IDLE_MS = 300_000;

type MediaType = "image/jpeg" | "image/png";
export interface SolveImage {
  media_type: MediaType;
  data: string;
}
export interface ParsedSolve {
  images: SolveImage[];
  tutor: boolean;
}

const B64_RE = /^[A-Za-z0-9+/]+={0,2}$/;

function bad(message: string): HttpError {
  return new HttpError(400, "bad_request", message);
}

function isObj(v: unknown): v is Record<string, unknown> {
  return typeof v === "object" && v !== null && !Array.isArray(v);
}

function checkMagic(media: MediaType, data: string): boolean {
  let head: string;
  try {
    head = atob(data.slice(0, 12));
  } catch {
    return false;
  }
  const b = (i: number) => head.charCodeAt(i);
  if (media === "image/jpeg") return b(0) === 0xff && b(1) === 0xd8 && b(2) === 0xff;
  return b(0) === 0x89 && head.slice(1, 4) === "PNG" && b(4) === 0x0d && b(5) === 0x0a && b(6) === 0x1a && b(7) === 0x0a;
}

/** Reads ONLY the images and `tutor` from either body shape (SolveRequest or LegacySolveRequest). */
export function parseSolveBody(body: unknown): ParsedSolve {
  if (!isObj(body)) throw bad("The request body must be a JSON object.");
  if (body.tutor !== undefined && typeof body.tutor !== "boolean") throw bad("tutor must be true or false.");
  const tutor = body.tutor === true;
  const images: SolveImage[] = [];

  if ("messages" in body) {
    // LegacySolveRequest (firmware before stage 2): a Messages-API body. Everything except the
    // base64 images in messages[0].content and the top-level tutor is ignored.
    const msgs = body.messages;
    if (!Array.isArray(msgs) || msgs.length < 1) throw bad("messages must be a non-empty list.");
    for (const m of msgs) {
      if (!isObj(m) || !Array.isArray(m.content) || !m.content.every(isObj)) {
        throw bad("Each message needs a content list.");
      }
    }
    for (const block of (msgs[0] as { content: Record<string, unknown>[] }).content) {
      if (block.type !== "image" || !isObj(block.source) || block.source.type !== "base64") continue;
      const { media_type, data } = block.source;
      if (media_type !== "image/jpeg" && media_type !== "image/png") throw bad("Photos must be JPEG or PNG.");
      if (typeof data !== "string") throw bad("A photo is damaged. Please take it again.");
      images.push({ media_type, data });
    }
  } else {
    // SolveRequest: { images: [{media_type, data}], tutor? }, no other fields.
    for (const k of Object.keys(body)) if (k !== "images" && k !== "tutor") throw bad(`Unknown field: ${k.slice(0, 40)}`);
    if (!Array.isArray(body.images)) throw bad("images must be a list of 1 or 2 photos.");
    for (const im of body.images) {
      if (!isObj(im)) throw bad("Each image needs media_type and data.");
      for (const k of Object.keys(im)) if (k !== "media_type" && k !== "data") throw bad(`Unknown field: ${k.slice(0, 40)}`);
      if (im.media_type !== "image/jpeg" && im.media_type !== "image/png") throw bad("Photos must be JPEG or PNG.");
      if (typeof im.data !== "string") throw bad("A photo is damaged. Please take it again.");
      images.push({ media_type: im.media_type, data: im.data });
    }
  }

  if (images.length < 1 || images.length > 2) throw bad("Send 1 or 2 photos.");
  let decoded = 0;
  for (const im of images) {
    const d = im.data;
    if (d.length < 8 || d.length % 4 !== 0 || !B64_RE.test(d)) throw bad("A photo is damaged. Please take it again.");
    decoded += (d.length / 4) * 3 - (d.endsWith("==") ? 2 : d.endsWith("=") ? 1 : 0);
    if (!checkMagic(im.media_type, d)) throw bad("A photo is damaged or not a JPEG/PNG.");
  }
  if (decoded > MAX_DECODED_BYTES) throw bad(MSG.tooLarge);
  return { images, tutor };
}

/** The upstream Messages API request. The server owns everything here except the photos. */
export function buildUpstreamBody(cfg: Config, p: ParsedSolve): Record<string, unknown> {
  const content: unknown[] = p.images.map((im) => ({
    type: "image",
    source: { type: "base64", media_type: im.media_type, data: im.data },
  }));
  content.push({ type: "text", text: p.images.length === 2 ? ASK_TWO_IMAGES : ASK_ONE_IMAGE });
  if (p.tutor) content.push({ type: "text", text: TUTOR_NOTE });
  const body: Record<string, unknown> = {
    model: cfg.model,
    max_tokens: cfg.maxTokens,
    stream: true,
    thinking: { type: "adaptive" },
    system: SOLVE_INSTRUCTIONS,
    output_config: { effort: cfg.effort, format: { type: "json_schema", schema: SOLVE_SCHEMA } },
    messages: [{ role: "user", content }],
  };
  // Server-side refusal fallback (Claude API beta), as server/proxy did. REFUSAL_FALLBACK=0 turns it off.
  if (cfg.refusalFallback) body.fallbacks = "default";
  return body;
}

// ------------------------------------------------------------ the atomic reservation
interface Reservation {
  id: string;
  reservedMicro: number;
  day: string;
}

async function reserve(
  env: Env,
  cfg: Config,
  deviceId: string,
  userId: string,
  images: number,
  now: number,
): Promise<Reservation> {
  const id = newId("use_");
  const worst = worstCaseMicro(cfg, images);
  const day = dayOf(now);
  const month = monthOf(now);
  // One D1 batch = one transaction. The INSERT happens only if every limit holds, so the checks
  // and the reservation can't interleave with another request. Any DB error throws (-> 503).
  const [ins] = await env.DB.batch([
    env.DB.prepare(
      `INSERT INTO usage (id, device_id, user_id, started_at, state, reserved_micro, cost_micro, model, day, month)
       SELECT ?1, ?2, ?3, ?4, 'reserved', ?5, 0, ?6, ?7, ?8
       WHERE NOT EXISTS (SELECT 1 FROM usage WHERE device_id = ?2 AND state = 'reserved' AND started_at > ?9)
         AND (SELECT COUNT(*) FROM usage WHERE user_id = ?3 AND month = ?8 AND state IN ('reserved','settled')) < ?10
         AND (SELECT COALESCE(SUM(CASE state WHEN 'reserved' THEN reserved_micro WHEN 'settled' THEN cost_micro ELSE 0 END), 0)
              FROM usage WHERE user_id = ?3 AND month = ?8) + ?5 <= ?11
         AND COALESCE((SELECT reserved_micro + settled_micro FROM spend_daily WHERE day = ?7), 0) + ?5 <= ?12`,
    ).bind(
      id,
      deviceId,
      userId,
      now,
      worst,
      cfg.model,
      day,
      month,
      now - STALE_INFLIGHT_SEC,
      cfg.monthlySolves,
      cfg.monthlySpendMicro,
      cfg.globalDailyCapMicro,
    ),
    env.DB.prepare(
      `INSERT INTO spend_daily (day, reserved_micro, settled_micro)
       SELECT ?1, ?2, 0 WHERE EXISTS (SELECT 1 FROM usage WHERE id = ?3)
       ON CONFLICT (day) DO UPDATE SET reserved_micro = reserved_micro + excluded.reserved_micro`,
    ).bind(day, worst, id),
  ]);
  if (ins?.meta.changes === 1) return { id, reservedMicro: worst, day };

  // Refused. Work out which limit, in contract order, for the message.
  const busy = await env.DB.prepare(
    `SELECT 1 AS x FROM usage WHERE device_id = ?1 AND state = 'reserved' AND started_at > ?2 LIMIT 1`,
  )
    .bind(deviceId, now - STALE_INFLIGHT_SEC)
    .first();
  if (busy) throw new HttpError(429, "rate_limited", MSG.inFlight, { "retry-after": "30" });
  await checkLimits(env, cfg, userId, now, images); // throws quota_exceeded or spend_cap
  throw new HttpError(503, "spend_cap", MSG.spendCap, { "retry-after": "60" }); // lost a race: fail closed
}

// R2-9: the usage row and spend_daily change in one batch (one transaction); spend_daily only
// moves when this call really flipped the reservation (changes() = 1).
async function release(env: Env, r: Reservation): Promise<void> {
  await env.DB.batch([
    env.DB.prepare(`UPDATE usage SET state = 'failed', cost_micro = 0 WHERE id = ?1 AND state = 'reserved'`).bind(r.id),
    env.DB.prepare(
      `UPDATE spend_daily SET reserved_micro = MAX(0, reserved_micro - ?1) WHERE day = ?2 AND changes() = 1`,
    ).bind(r.reservedMicro, r.day),
  ]);
}

async function settle(env: Env, r: Reservation, costMicro: number, model: string | null, u: TokenUsage | null) {
  await env.DB.batch([
    env.DB.prepare(
      `UPDATE usage SET state = 'settled', cost_micro = ?1, model = COALESCE(?2, model), input_tokens = ?3, output_tokens = ?4
       WHERE id = ?5 AND state = 'reserved'`,
    ).bind(costMicro, model, u ? u.input + u.cacheWrite + u.cacheRead : null, u ? u.output : null, r.id),
    env.DB.prepare(
      `UPDATE spend_daily SET reserved_micro = MAX(0, reserved_micro - ?1), settled_micro = settled_micro + ?2
       WHERE day = ?3 AND changes() = 1`,
    ).bind(r.reservedMicro, costMicro, r.day),
  ]);
}

// ------------------------------------------------------------ SSE usage scanner
/** Watches the SSE bytes (without changing them) for usage, the model, and the end of the message. */
export class UsageScanner {
  private dec = new TextDecoder();
  private buf = "";
  sawStart = false;
  sawDelta = false;
  sawStop = false;
  sawError = false;
  model: string | null = null;
  usage: TokenUsage = { input: 0, cacheWrite: 0, cacheRead: 0, output: 0 };

  feed(chunk: Uint8Array): void {
    this.buf += this.dec.decode(chunk, { stream: true }).replace(/\r/g, "");
    let end: number;
    while ((end = this.buf.indexOf("\n\n")) !== -1) {
      const ev = this.buf.slice(0, end);
      this.buf = this.buf.slice(end + 2);
      for (const line of ev.split("\n")) {
        if (line.startsWith("data:")) this.onData(line.slice(line[5] === " " ? 6 : 5));
      }
    }
    if (this.buf.length > 1_000_000) this.buf = ""; // never buffer without bound
  }

  private take(u: unknown): void {
    if (typeof u !== "object" || u === null) return;
    const o = u as Record<string, unknown>;
    const n = (k: string) => (typeof o[k] === "number" && Number.isFinite(o[k]) ? (o[k] as number) : undefined);
    // Usage numbers are cumulative; keep the largest seen.
    this.usage.input = Math.max(this.usage.input, n("input_tokens") ?? 0);
    this.usage.cacheWrite = Math.max(this.usage.cacheWrite, n("cache_creation_input_tokens") ?? 0);
    this.usage.cacheRead = Math.max(this.usage.cacheRead, n("cache_read_input_tokens") ?? 0);
    this.usage.output = Math.max(this.usage.output, n("output_tokens") ?? 0);
  }

  private onData(data: string): void {
    if (
      !data.includes('"message_start"') &&
      !data.includes('"message_delta"') &&
      !data.includes('"message_stop"') &&
      !data.includes('"error"')
    ) {
      return; // content deltas: not parsed (cheap)
    }
    let j: Record<string, unknown>;
    try {
      j = JSON.parse(data);
    } catch {
      return;
    }
    if (j.type === "message_start" && typeof j.message === "object" && j.message !== null) {
      const m = j.message as Record<string, unknown>;
      this.sawStart = true;
      if (typeof m.model === "string") this.model = m.model.slice(0, 64);
      this.take(m.usage);
    } else if (j.type === "message_delta") {
      if (j.usage) {
        this.sawDelta = true;
        this.take(j.usage);
      }
    } else if (j.type === "message_stop") {
      this.sawStop = true;
    } else if (j.type === "error") {
      this.sawError = true;
    }
  }
}

const enc = new TextEncoder();

function midStreamError(): Uint8Array {
  // Leading blank line ends any half-sent event, so the calculator sees a clean error event.
  return enc.encode(`\n\nevent: error\ndata: ${JSON.stringify(errorBody("upstream_error", MSG.upstreamMidStream))}\n\n`);
}

async function readWithIdle<T>(reader: ReadableStreamDefaultReader<T>, ms: number) {
  let timer: ReturnType<typeof setTimeout> | undefined;
  const timeout = new Promise<never>((_, rej) => {
    timer = setTimeout(() => rej(new Error("upstream idle")), ms);
  });
  try {
    return await Promise.race([reader.read(), timeout]);
  } finally {
    if (timer !== undefined) clearTimeout(timer);
  }
}

// ------------------------------------------------------------ the handler
export async function solve(
  req: Request,
  env: Env,
  cfg: Config,
  ctx: ExecutionContext,
  now: number,
): Promise<Response> {
  // Order: auth -> linked -> entitled -> burst, all BEFORE the (up to 8 MiB) body is read and
  // parsed (security review S6), then body -> in-flight -> monthly -> global (the reservation).
  const dev = await authDevice(req, env, now);
  const { user } = await checkLinkedAndEntitled(env, cfg, dev, now);
  const burst = await hitRateLimit(env.DB, `solve:dev:${dev.id}`, 60, BURST_PER_MIN, now);
  if (!burst.ok) throw rateLimited(burst, MSG.burst);

  const raw = await readBody(req, MAX_BODY_BYTES, new HttpError(413, "payload_too_large", MSG.tooLarge));
  let body: unknown;
  try {
    body = JSON.parse(new TextDecoder().decode(raw));
  } catch {
    throw bad("The request body is not valid JSON.");
  }
  const parsed = parseSolveBody(body);

  const apiKey = (env.ANTHROPIC_API_KEY ?? "").trim();
  if (!apiKey) {
    console.error(JSON.stringify({ event: "config_error", detail: "ANTHROPIC_API_KEY not set" }));
    throw new HttpError(503, "unavailable", MSG.unavailable);
  }

  const r = await reserve(env, cfg, dev.id, user.id, parsed.images.length, now);

  // ---- upstream call (only after a successful reservation)
  const headers: Record<string, string> = {
    "content-type": "application/json",
    "x-api-key": apiKey,
    "anthropic-version": "2023-06-01",
  };
  if (cfg.refusalFallback) headers["anthropic-beta"] = "server-side-fallback-2026-07-01";
  const abort = new AbortController();
  const timer = setTimeout(() => abort.abort(), cfg.upstreamTimeoutMs);
  let upstream: Response;
  try {
    upstream = await fetch(`${cfg.anthropicBaseUrl}/v1/messages`, {
      method: "POST",
      headers,
      body: JSON.stringify(buildUpstreamBody(cfg, parsed)),
      signal: abort.signal,
    });
  } catch (e) {
    clearTimeout(timer);
    console.error(JSON.stringify({ event: "upstream_fetch_failed", device: dev.id, detail: String(e).slice(0, 200) }));
    ctx.waitUntil(release(env, r).catch(logSettleError));
    throw new HttpError(502, "upstream_error", MSG.upstream);
  }
  clearTimeout(timer);
  if (upstream.status !== 200 || !upstream.body) {
    console.error(JSON.stringify({ event: "upstream_status", device: dev.id, status: upstream.status }));
    await upstream.body?.cancel().catch(() => {});
    ctx.waitUntil(release(env, r).catch(logSettleError));
    throw new HttpError(502, "upstream_error", MSG.upstream);
  }

  // ---- stream through, byte for byte, while scanning for usage
  const { readable, writable } = new TransformStream<Uint8Array, Uint8Array>();
  const writer = writable.getWriter();
  const scanner = new UsageScanner();
  const reader = upstream.body.getReader();
  const deadline = Date.now() + cfg.streamMaxMs; // S12: total stream time cap
  const pump = (async () => {
    let clientGone = false;
    let failed = false;
    let timedOut = false;
    try {
      for (;;) {
        const left = deadline - Date.now();
        if (left <= 0) {
          timedOut = true;
          throw new Error("stream time cap");
        }
        const { done, value } = await readWithIdle(reader, Math.min(STREAM_IDLE_MS, left)).catch((e) => {
          if (Date.now() >= deadline) timedOut = true;
          throw e;
        });
        if (done) break;
        scanner.feed(value);
        try {
          await writer.write(value);
        } catch {
          clientGone = true; // calculator hung up: stop generating (and paying)
          await reader.cancel().catch(() => {});
          break;
        }
      }
    } catch {
      failed = true;
      await reader.cancel().catch(() => {});
    }
    if (!clientGone && (failed || (!scanner.sawStop && !scanner.sawError))) {
      await writer.write(midStreamError()).catch(() => {});
    }
    await writer.close().catch(() => {});

    // Settle: real cost when the final usage arrived; worst case if the message started but its
    // final usage never came (errs on the safe side); release when nothing was generated.
    if (timedOut) {
      // Cut off by the time cap: keep the worst case (the final usage may never be known).
      await settle(env, r, r.reservedMicro, scanner.model, scanner.usage);
      log(dev.id, "settled_worst_case_timeout", r.reservedMicro);
    } else if (scanner.sawDelta) {
      const cost = actualCostMicro(cfg, scanner.usage);
      await settle(env, r, cost, scanner.model, scanner.usage);
      log(dev.id, "settled", cost);
    } else if (scanner.sawStart) {
      await settle(env, r, r.reservedMicro, scanner.model, scanner.usage);
      log(dev.id, "settled_worst_case", r.reservedMicro);
    } else {
      await release(env, r);
      log(dev.id, "released", 0);
    }
  })();
  ctx.waitUntil(pump.catch(logSettleError));

  return new Response(readable, {
    status: 200,
    headers: { "content-type": "text/event-stream", "cache-control": "no-cache", "x-content-type-options": "nosniff" },
  });
}

function log(device: string, state: string, costMicro: number) {
  console.log(JSON.stringify({ event: "solve", device, state, cost_micro: costMicro }));
}

function logSettleError(e: unknown) {
  // The reservation stays at worst case: safe for the caps.
  console.error(JSON.stringify({ event: "settle_failed", detail: String(e).slice(0, 200) }));
}
