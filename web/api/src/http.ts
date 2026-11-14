// Responses, the error shape and the fixed device-facing messages (PLAN section 4b).

import type { Config } from "./env";

export type ErrorType =
  | "bad_request"
  | "unauthorized"
  | "forbidden"
  | "not_found"
  | "device_revoked"
  | "device_not_linked"
  | "subscription_inactive"
  | "quota_exceeded"
  | "rate_limited"
  | "spend_cap"
  | "payload_too_large"
  | "upstream_error"
  | "unavailable"
  | "invalid_token"
  | "invalid_code"
  | "device_limit"
  | "already_subscribed"
  | "no_customer"
  | "bad_signature"
  | "sold_out"
  | "needs_replace"
  | "link_token_invalid"
  | "link_token_expired"
  | "link_token_used"
  | "link_token_state"
  | "already_linked"
  | "superseded";

const BASE_HEADERS: Record<string, string> = {
  "cache-control": "no-store",
  "x-content-type-options": "nosniff",
};

/** Messages are at most 110 characters (the e-paper shows them as is). */
export function clamp110(s: string): string {
  return s.length <= 110 ? s : s.slice(0, 110);
}

export function json(body: unknown, status = 200, headers: Record<string, string> = {}): Response {
  return new Response(JSON.stringify(body), {
    status,
    headers: { ...BASE_HEADERS, "content-type": "application/json", ...headers },
  });
}

export function empty(status = 204, headers: HeadersInit = {}): Response {
  const h = new Headers(BASE_HEADERS);
  new Headers(headers).forEach((v, k) => h.append(k, v));
  return new Response(null, { status, headers: h });
}

/** `extra` adds top-level fields next to `error` (e.g. `replaces` on 409 needs_replace). */
export function errorBody(type: ErrorType, message: string, extra: Record<string, unknown> = {}) {
  return { type: "error", error: { type, message: clamp110(message) }, ...extra };
}

export function error(
  status: number,
  type: ErrorType,
  message: string,
  headers: Record<string, string> = {},
  extra: Record<string, unknown> = {},
): Response {
  return json(errorBody(type, message, extra), status, headers);
}

/** Thrown anywhere in a handler to answer with an error immediately. */
export class HttpError extends Error {
  constructor(
    readonly status: number,
    readonly type: ErrorType,
    readonly userMessage: string,
    readonly headers: Record<string, string> = {},
    readonly extra: Record<string, unknown> = {},
  ) {
    super(type);
  }
  response(): Response {
    return error(this.status, this.type, this.userMessage, this.headers, this.extra);
  }
}

// ------------------------------------------------------------ fixed messages
export const MSG = {
  revoked: "This calculator was removed from its account. Link it again in Apps > Settings.",
  quota: "Monthly AI limit reached. It resets on the 1st.",
  spendCap: "AI is busy right now. Please try again later.",
  unknownDevice: "This calculator isn't registered. It will register again on its own.",
  burst: "Too many solves in a minute. Please wait a moment.",
  inFlight: "Another solve is still running. Please wait for it to finish.",
  superseded: "A newer solve replaced this one.",
  tooLarge: "Photo too large. Please take it again.",
  upstream: "The AI service had a problem. Please try again.",
  upstreamMidStream: "The AI reply was cut off. Please try again.",
  unavailable: "The AI server is having trouble. Please try again later.",
  badRequest: "The request was not valid.",
  rateLimited: "Too many requests. Please try again later.",
  notFound: "Not found.",
  methodNotAllowed: "Method not allowed.",
  forbidden: "Request refused.",
  signIn: "Please sign in.",
  invalidToken: "This sign-in link is invalid or expired. Please ask for a new one.",
  invalidCode: "That code is unknown, expired or already used. Get a new code on the calculator.",
  needsReplace: "Your plan covers one calculator. Replace the one on your account with this one?",
  linkTokenInvalid: "This code isn't valid. Get a new code on your phone.",
  linkTokenExpired: "This code has expired. Get a new code on your phone.",
  linkTokenUsed: "This code was already used. Get a new code on your phone.",
  alreadyLinked: "This calculator is linked to another account. Remove it there first.",
  linkTokenState: "This request isn't waiting for approval any more. Make a new code and scan again.",
};

export function msgTrialOver(c: Config): string {
  return clamp110(`Your free month is over. Subscribe at ${c.host}/account to keep using AI.`);
}
export function msgSubEnded(c: Config): string {
  return clamp110(`Your subscription has ended. Renew at ${c.host}/account to keep using AI.`);
}
/** Linked, but this board/account can't get a free month (S1). */
export function msgNoTrial(c: Config): string {
  return clamp110(`No free month for this calculator. Subscribe at ${c.host}/account to use AI.`);
}
export function msgNotLinked(c: Config, code: string): string {
  return clamp110(`Link this calculator: go to ${c.host}/link and enter code ${code}`);
}
