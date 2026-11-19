// Back office (2026-11-18): inbound customer e-mail, the business numbers, shipping with a
// tracking e-mail, and the inbox (draft -> Nirav approves -> reply sent). docs/web/BACK_OFFICE.md

import { env } from "cloudflare:workers";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { inboundEmail } from "../src/index";
import type { Env } from "../src/env";
import { ADMIN_TOKEN, call, expectError } from "./helpers";

const now = () => Math.floor(Date.now() / 1000);
const admin = { "x-admin-token": ADMIN_TOKEN };
const OFFICE = { RESEND_API_KEY: "re_test", MAIL_REPLY_FROM: "Proofline <hello@proofline.test>" };

function mime(o: { from?: string; subject?: string; body?: string; headers?: string[]; html?: string; id?: string }) {
  const lines = [
    `From: ${o.from ?? "Sam Buyer <sam@example.com>"}`,
    `To: hello@proofline.test`,
    `Subject: ${o.subject ?? "Where is my order?"}`,
    `Message-ID: ${o.id ?? `<m${Math.random().toString(36).slice(2)}@example.com>`}`,
    ...(o.headers ?? []),
  ];
  if (o.html) return [...lines, `Content-Type: text/html; charset=utf-8`, ``, o.html].join("\r\n");
  return [...lines, `Content-Type: text/plain; charset=utf-8`, ``, o.body ?? "Hi, I ordered last week. When will it ship?"].join("\r\n");
}

function message(raw: string, forward = vi.fn(async () => ({}))) {
  return {
    from: "sam@example.com",
    to: "hello@proofline.test",
    rawSize: raw.length,
    raw: new Response(raw).body!,
    headers: new Headers(),
    forward,
    setReject: vi.fn(),
  } as unknown as ForwardableEmailMessage & { forward: typeof forward };
}

const testEnv = (over: Partial<Env> = {}) => ({ ...(env as unknown as Env), ...over });
const receive = (raw: string, over: Partial<Env> = {}, msg = message(raw)) => inboundEmail(msg, testEnv(over), now());
const dbAll = async (sql: string, ...b: unknown[]) => (await env.DB.prepare(sql).bind(...b).all<any>()).results;

/** Every mail sent through Resend (fetch is mocked), and an optional failure. */
function fakeResend(status = 200) {
  const sent: any[] = [];
  vi.spyOn(globalThis, "fetch").mockImplementation(async (input: RequestInfo | URL, init?: RequestInit) => {
    const req = new Request(input as RequestInfo, init);
    expect(req.url).toBe("https://api.resend.com/emails");
    sent.push(await req.json());
    return status === 200 ? Response.json({ id: "re_1" }) : new Response("nope", { status });
  });
  return sent;
}

beforeEach(async () => {
  await env.DB.batch([
    env.DB.prepare(`DELETE FROM mail`),
    env.DB.prepare(`DELETE FROM orders`),
    env.DB.prepare(`DELETE FROM waitlist`),
  ]);
});
afterEach(() => vi.restoreAllMocks());

describe("inbound e-mail", () => {
  it("stores the mail and forwards a copy", async () => {
    const msg = message(mime({}));
    await inboundEmail(msg, testEnv({ MAIL_FORWARD_TO: "owner@gmail.test" }), now());
    expect(msg.forward).toHaveBeenCalledWith("owner@gmail.test");
    const [m] = await dbAll(`SELECT * FROM mail`);
    expect(m.from_addr).toBe("sam@example.com");
    expect(m.from_name).toBe("Sam Buyer");
    expect(m.subject).toBe("Where is my order?");
    expect(m.body).toContain("When will it ship?");
    expect(m.status).toBe("new");
    expect(m.auto).toBe(0);
    expect(m.thread_id).toBe(m.id);
  });

  it("turns HTML-only mail into plain text", async () => {
    await receive(mime({ html: "<html><head><style>p{}</style></head><body><p>Hello&nbsp;there</p><script>x()</script><p>Bye &amp; thanks</p></body></html>" }));
    const [m] = await dbAll(`SELECT * FROM mail`);
    expect(m.body).toBe("Hello there\nBye & thanks");
  });

  it("marks auto-replies, bounces and lists as automatic (never drafted)", async () => {
    await receive(mime({ headers: ["Auto-Submitted: auto-replied"], subject: "Out of office" }));
    await receive(mime({ from: "MAILER-DAEMON@mx.example.com", subject: "Undelivered" }));
    await receive(mime({ headers: ["List-Unsubscribe: <https://x.example/u>"], subject: "Newsletter" }));
    const rows = await dbAll(`SELECT status, auto FROM mail`);
    expect(rows).toHaveLength(3);
    for (const r of rows) expect(r).toEqual({ status: "ignored", auto: 1 });
  });

  it("keeps a conversation together (In-Reply-To, or the same sender and subject)", async () => {
    await receive(mime({ id: "<first@example.com>", subject: "Battery question" }));
    await receive(mime({ id: "<second@example.com>", subject: "Re: Battery question", headers: ["In-Reply-To: <first@example.com>"] }));
    await receive(mime({ subject: "RE: battery question" }));
    await receive(mime({ subject: "Something else" }));
    const rows = await dbAll(`SELECT subject, thread_id FROM mail ORDER BY rowid`);
    expect(new Set(rows.slice(0, 3).map((r) => r.thread_id)).size).toBe(1);
    expect(rows[3].thread_id).not.toBe(rows[0].thread_id);
  });

  it("skips its own replies looping back, and stops storing after the daily cap", async () => {
    await receive(mime({ from: "Proofline <hello@proofline.test>" }), OFFICE);
    expect(await dbAll(`SELECT * FROM mail`)).toHaveLength(0);
    const t = now();
    const stmt = env.DB.prepare(`INSERT INTO mail (id, received_at, from_addr, thread_id, status) VALUES (?1, ?2, 'x@example.com', ?1, 'new')`);
    await env.DB.batch(Array.from({ length: 300 }, (_, i) => stmt.bind(`mail_${String(i).padStart(16, "0")}`, t)));
    const msg = message(mime({}));
    await inboundEmail(msg, testEnv({ MAIL_FORWARD_TO: "owner@gmail.test" }), t);
    expect(await dbAll(`SELECT * FROM mail WHERE from_addr = 'sam@example.com'`)).toHaveLength(0);
    expect(msg.forward).toHaveBeenCalled(); // still reaches Nirav's own inbox
  });

  it("a failed forward doesn't lose a stored mail", async () => {
    const msg = message(mime({}), vi.fn(async () => { throw new Error("unverified destination"); }));
    await expect(inboundEmail(msg, testEnv({ MAIL_FORWARD_TO: "owner@gmail.test" }), now())).resolves.toBeUndefined();
    expect(await dbAll(`SELECT * FROM mail`)).toHaveLength(1);
  });
});

describe("admin access", () => {
  it("every back-office route needs the admin token", async () => {
    for (const p of ["/v1/admin/business", "/v1/admin/orders", "/v1/admin/mail", "/v1/admin/stripe", "/v1/admin/mail/mail_0123456789abcdef"]) {
      expectError(await call(p), 401, "unauthorized");
      expectError(await call(p, { headers: { "x-admin-token": "x".repeat(40) } }), 401, "unauthorized");
    }
    expectError(await call("/v1/admin/business", { headers: admin, env: { ADMIN_TOKEN: "" } }), 404, "not_found");
  });
});

async function seedOrder(id: string, o: Record<string, any> = {}) {
  const shipping = { name: "Sam Buyer", address: { line1: "1 Main St", city: "Indy", state: "IN", postal_code: "46201", country: "US" } };
  await env.DB.prepare(
    `INSERT INTO orders (id, email, amount_total, currency, status, shipping_json, created_at, quantity, shipped_at)
     VALUES (?1, ?2, ?3, 'usd', ?4, ?5, ?6, ?7, ?8)`,
  )
    .bind(id, o.email ?? "sam@example.com", o.cents ?? 23345, o.status ?? "paid", JSON.stringify(shipping), o.at ?? now(), o.qty ?? 1, o.shipped ?? null)
    .run();
}

describe("business numbers", () => {
  it("counts sales, what's left to ship, stock, the waitlist and the inbox", async () => {
    const t = now();
    await seedOrder("cs_test_a", { at: t - 100 });
    await seedOrder("cs_test_b", { at: t - 3 * 86400, qty: 2, cents: 46690 });
    await seedOrder("cs_test_c", { at: t - 10 * 86400, shipped: t - 86400 });
    await seedOrder("cs_test_d", { status: "open" });
    await env.DB.batch([
      env.DB.prepare(`INSERT INTO waitlist (email, source, created_at, utm_source) VALUES ('a@x.com', 'hero', ?1, 'tiktok')`).bind(t),
      env.DB.prepare(`INSERT INTO waitlist (email, source, created_at) VALUES ('b@x.com', 'footer', ?1)`).bind(t - 2 * 86400),
    ]);
    await receive(mime({}));
    const r = await call("/v1/admin/business", { headers: admin, env: { STORE_STOCK: "20", STORE_OPEN: "1" } });
    expect(r.res.status, r.text).toBe(200);
    const b = r.json;
    expect(b.sales.all).toEqual({ orders: 3, cents: 23345 + 46690 + 23345, refunded_cents: 0 });
    expect(b.sales.week.orders).toBe(2);
    expect(b.orders.to_ship).toMatchObject({ orders: 2, units: 3 });
    expect(b.store).toMatchObject({ open: true, stock: 20, units_left: 20 - 4 - 1 });
    expect(b.waitlist).toMatchObject({ total: 2, week: 2 });
    expect(b.waitlist.sources_30d).toContainEqual({ source: "tiktok", n: 1 });
    expect(b.mail.new).toBe(1);
    expect(b.setup.stripe_key).toBe("test");
    expect(JSON.stringify(b)).not.toContain("sam@example.com"); // numbers only, no customer details
    expect(JSON.stringify(b)).not.toContain("sk_test");
  });
});

describe("orders and shipping", () => {
  it("lists paid orders to ship with the address, oldest first", async () => {
    await seedOrder("cs_test_new", { at: now() - 10 });
    await seedOrder("cs_test_old", { at: now() - 5000 });
    await seedOrder("cs_test_done", { shipped: now() });
    await seedOrder("cs_test_unpaid", { status: "open" });
    const r = await call("/v1/admin/orders?view=to_ship", { headers: admin });
    expect(r.json.orders.map((o: any) => o.id)).toEqual(["cs_test_old", "cs_test_new"]);
    expect(r.json.orders[0].ship_to).toEqual({ name: "Sam Buyer", address: "1 Main St, Indy, IN 46201, US" });
  });

  it("mark shipped saves the tracking and e-mails the customer a tracking link", async () => {
    await seedOrder("cs_test_ship");
    const sent = fakeResend();
    const r = await call("/v1/admin/orders/cs_test_ship/ship", { headers: admin, env: OFFICE, body: { carrier: "usps", tracking: "9400 1000 0000 0000 0000 00" } });
    expect(r.res.status, r.text).toBe(200);
    expect(r.json.mailed).toBe(true);
    expect(r.json.order).toMatchObject({ carrier: "usps", tracking: "9400100000000000000000" });
    expect(sent).toHaveLength(1);
    expect(sent[0].to).toEqual(["sam@example.com"]);
    expect(sent[0].from).toBe("Proofline <hello@proofline.test>");
    expect(sent[0].text).toContain("https://tools.usps.com/go/TrackConfirmAction?tLabels=9400100000000000000000");
    expect(sent[0].text).toContain("Hi Sam,");
    const [o] = await dbAll(`SELECT shipped_at, ship_notified_at FROM orders WHERE id = 'cs_test_ship'`);
    expect(o.shipped_at).toBeGreaterThan(0);
    expect(o.ship_notified_at).toBeGreaterThan(0);
    expect((await call("/v1/admin/orders?view=to_ship", { headers: admin })).json.orders).toHaveLength(0);
  });

  it("refuses bad carriers, bad tracking, and orders that aren't paid", async () => {
    await seedOrder("cs_test_x");
    await seedOrder("cs_test_open", { status: "open" });
    const ship = (id: string, body: object) => call(`/v1/admin/orders/${id}/ship`, { headers: admin, env: OFFICE, body });
    expectError(await ship("cs_test_x", { carrier: "pigeon", tracking: "123456" }), 400, "bad_request");
    expectError(await ship("cs_test_x", { carrier: "ups", tracking: "<script>" }), 400, "bad_request");
    expectError(await ship("cs_test_open", { carrier: "ups", tracking: "1Z999AA10123456784" }), 404, "not_found");
    expectError(await ship("nope", { carrier: "ups", tracking: "1Z999AA10123456784" }), 404, "not_found");
  });

  it("a mail problem still saves the shipment and says what went wrong", async () => {
    await seedOrder("cs_test_nomail");
    const r = await call("/v1/admin/orders/cs_test_nomail/ship", { headers: admin, body: { carrier: "ups", tracking: "1Z999AA10123456784" } });
    expect(r.res.status).toBe(200);
    expect(r.json.mailed).toBe(false);
    expect(r.json.mail_problem).toMatch(/RESEND_API_KEY/);
  });
});

describe("the inbox: draft, approve, send", () => {
  async function oneMail(subject = "Where is my order?") {
    await receive(mime({ subject, id: "<cust1@example.com>" }));
    return (await dbAll(`SELECT id FROM mail`))[0].id as string;
  }

  it("lists open mail and shows the sender's orders (without their address)", async () => {
    await seedOrder("cs_test_mine", { qty: 2 });
    const id = await oneMail();
    const list = await call("/v1/admin/mail?view=open", { headers: admin });
    expect(list.json.mail.map((m: any) => m.id)).toEqual([id]);
    expect(list.json.mail[0].preview).toContain("When will it ship?");
    const d = await call(`/v1/admin/mail/${id}`, { headers: admin });
    expect(d.json.mail.body).toContain("When will it ship?");
    expect(d.json.customer.orders).toHaveLength(1);
    expect(d.json.customer.orders[0]).toMatchObject({ id: "cs_test_mine", quantity: 2, ship_to: { name: "Sam Buyer" } });
    expect(JSON.stringify(d.json.customer)).not.toContain("Main St");
  });

  it("a saved draft waits; Approve & send replies in the same thread", async () => {
    const id = await oneMail();
    const draft = await call(`/v1/admin/mail/${id}`, {
      headers: admin,
      body: { action: "draft", draft: "Hi Sam, it ships this week.", summary: "Asks when the order ships", category: "shipping" },
    });
    expect(draft.json.mail).toMatchObject({ status: "drafted", category: "shipping", draft: "Hi Sam, it ships this week." });
    const sent = fakeResend();
    const r = await call(`/v1/admin/mail/${id}/send`, { headers: admin, env: OFFICE, body: { text: "Hi Sam, it ships Friday. Nirav" } });
    expect(r.res.status, r.text).toBe(200);
    expect(sent).toHaveLength(1);
    expect(sent[0].to).toEqual(["sam@example.com"]);
    expect(sent[0].subject).toBe("Re: Where is my order?");
    expect(sent[0].headers).toEqual({ "In-Reply-To": "<cust1@example.com>", References: "<cust1@example.com>" });
    expect(sent[0].text).toMatch(/^Hi Sam, it ships Friday\. Nirav\n\nOn .* Sam Buyer wrote:\n> Hi, I ordered/);
    const [m] = await dbAll(`SELECT status, reply_text FROM mail`);
    expect(m).toEqual({ status: "replied", reply_text: "Hi Sam, it ships Friday. Nirav" });
    // No second reply, and a draft can't overwrite a sent mail.
    expectError(await call(`/v1/admin/mail/${id}/send`, { headers: admin, env: OFFICE, body: { text: "again" } }), 400, "bad_request");
    expectError(await call(`/v1/admin/mail/${id}`, { headers: admin, body: { action: "draft", draft: "x" } }), 400, "bad_request");
    expect(sent).toHaveLength(1);
  });

  it("a failed send puts the mail back with its draft", async () => {
    const id = await oneMail();
    await call(`/v1/admin/mail/${id}`, { headers: admin, body: { action: "draft", draft: "Draft text" } });
    fakeResend(422);
    expectError(await call(`/v1/admin/mail/${id}/send`, { headers: admin, env: OFFICE, body: { text: "Hi" } }), 503, "unavailable");
    const [m] = await dbAll(`SELECT status, draft, reply_text FROM mail`);
    expect(m).toEqual({ status: "drafted", draft: "Draft text", reply_text: null });
  });

  it("ignore and reopen; automatic mail can't be answered", async () => {
    const id = await oneMail();
    expect((await call(`/v1/admin/mail/${id}`, { headers: admin, body: { action: "ignore", category: "spam" } })).json.mail.status).toBe("ignored");
    expect((await call("/v1/admin/mail?view=open", { headers: admin })).json.mail).toHaveLength(0);
    expect((await call(`/v1/admin/mail/${id}`, { headers: admin, body: { action: "reopen" } })).json.mail.status).toBe("new");
    await receive(mime({ headers: ["Auto-Submitted: auto-replied"] }));
    const auto = (await dbAll(`SELECT id FROM mail WHERE auto = 1`))[0].id;
    expectError(await call(`/v1/admin/mail/${auto}/send`, { headers: admin, env: OFFICE, body: { text: "Hi" } }), 400, "bad_request");
    expectError(await call(`/v1/admin/mail/${id}`, { headers: admin, body: { action: "explode" } }), 400, "bad_request");
  });
});

describe("money in Stripe", () => {
  it("shows the balance and payouts", async () => {
    vi.spyOn(globalThis, "fetch").mockImplementation(async (input: RequestInfo | URL) => {
      const u = new URL(String(input instanceof Request ? input.url : input));
      if (u.pathname === "/v1/balance") {
        return Response.json({ livemode: false, available: [{ amount: 21500, currency: "usd" }], pending: [{ amount: 43000, currency: "usd" }] });
      }
      if (u.pathname === "/v1/payouts") return Response.json({ data: [{ amount: 10000, arrival_date: 1790000000, status: "paid" }] });
      return new Response("{}", { status: 404 });
    });
    const r = await call("/v1/admin/stripe", { headers: admin });
    expect(r.json).toEqual({
      ok: true,
      mode: "test",
      available_cents: 21500,
      pending_cents: 43000,
      payouts: [{ cents: 10000, arrival: 1790000000, status: "paid" }],
      payouts_error: null,
    });
  });

  it("explains a key that can't read the balance", async () => {
    vi.spyOn(globalThis, "fetch").mockImplementation(async () =>
      Response.json({ error: { type: "invalid_request_error", code: "permission_denied" } }, { status: 403 }),
    );
    const r = await call("/v1/admin/stripe", { headers: admin });
    expect(r.json.ok).toBe(false);
    expect(r.json.why).toMatch(/Balance: read/);
  });
});
