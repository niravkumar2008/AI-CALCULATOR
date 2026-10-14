// The calculator's side of the Cloudflare Worker API (stage 2 of the subscribe layer;
// contract: docs/web/openapi.yaml in the launch-site branch, the /v1/* device endpoints).
// Only building request bodies and reading replies: no networking, so every piece is
// unit-tested on a PC (tests/worker_api_tests.cpp). The firmware (worker_client.cpp,
// claude_client.cpp, ota.cpp) does the HTTPS and stores what these return.
//
//   POST /v1/device/register  {hw_id, hw_secret?}       -> token, device_id, pair code, link URL
//   GET  /v1/device/status    (Bearer token)            -> linked, plan, can_solve, reason ...
//   POST /v1/pair/start       (Bearer token)            -> a fresh pair code (or already_linked)
//   POST /v1/solve            {images:[{media_type,data}], tutor?} -> Messages-API SSE stream
//   GET  /v1/firmware?family=F (Bearer token, x-firmware)-> {update, version, size, sha256, path, family?, rollback?}
//
// Errors are {"type":"error","error":{"type":...,"message":...}}; the message is at most 110
// characters and written for the calculator's screen.
#pragma once
#include <string>

#include "app.h"
#include "claude_api.h"

namespace calc {

// ---- identity ----
bool isWorkerHwId(const std::string& s);        // calc-<12 lower-case hex>
bool isWorkerHwSecret(const std::string& s);    // hs_ + 43 base64url characters
bool isWorkerDeviceToken(const std::string& s);  // dt_ + 43 base64url characters
bool isWorkerDeviceId(const std::string& s);     // dev_ + 16 lower-case hex

// ---- register ----
// The register body. hwSecret is left out when empty or malformed (the board is not enrolled:
// it still registers and can link, but never gets the free month).
std::string buildRegisterBody(const std::string& hwId, const std::string& hwSecret);

struct WorkerRegistration {
  std::string deviceId, token;  // token: keep in NVS, send as "authorization: Bearer <token>"
  std::string pairCode;         // "ABCD-EFGH", for Settings > Link calculator
  std::string linkUrl;          // "https://site/link?code=ABCD-EFGH"
  std::string site;             // "https://site": what Device::setLinkInfo() wants
  long long expiresAt = 0;      // pair code expiry, Unix seconds
};
// 201 with a well-formed token and device id -> true. Else false and `error` is a sentence
// for the serial log / screen (the server's own message when it sent one).
bool parseRegisterReply(int httpStatus, const std::string& body, WorkerRegistration& out, std::string& error);

// ---- status ----
struct WorkerStatus {
  bool linked = false;
  std::string plan;         // none | trial | active | past_due | inactive
  bool canSolve = false;
  std::string reason;       // why not, ready for the screen (present when canSolve is false)
  std::string accountHint;  // "n***@gmail.com" when linked
  long long trialEndsAt = 0, periodEnd = 0;
  int solvesThisMonth = -1, solvesLimit = -1;  // -1 = not sent
  // Not linked: the device's current code and the site. From `pair_code` / `link_url` when
  // the server sends them (added 2026-10-11), else read out of `reason` ("Link this
  // calculator: go to <host>/link and enter code ABCD-EFGH"). Both "" when not found.
  // `site` is a link_url's site ("https://host") or the reason's ("host").
  std::string pairCode, site;
  std::string linkUrl;     // `link_url` as sent ("https://host/link/"), "" when absent
  // `plan_reason` (added 2026-10-11): not_linked | trial | active | past_due | free_month_over
  // | inactive | no_trial. "" when absent or not one of these (then only `reason` is shown).
  // It is the plan state, not whether AI works right now (can_solve).
  std::string planReason;
};
// 200 -> true. Other statuses -> false with `error` (the server's message if any).
bool parseWorkerStatus(int httpStatus, const std::string& body, WorkerStatus& out, std::string& error);

// The Link screen's plan line for a `plan_reason` (WorkerStatus::planReason): "Free month
// active", "Payment problem: check your account", "Free month over: subscribe at <site>" ...
// `site` is shown without its scheme. not_linked -> "" (the link code screen says it all).
// Unknown or absent planReason -> `reason` (the server's own sentence, may be "").
std::string planStatusText(const std::string& planReason, const std::string& site, const std::string& reason);

// The address a QR code on the Link screen would carry: link_url + "?code=" + code
// ("https://host/link/?code=ABCD-EFGH"). "" when either is missing.
std::string pairQrUrl(const std::string& linkUrl, const std::string& code);

// ---- pair/start ----
struct WorkerPairing {
  bool alreadyLinked = false;
  std::string code, linkUrl, site;
  long long expiresAt = 0;
};
bool parsePairStart(int httpStatus, const std::string& body, WorkerPairing& out, std::string& error);

// ---- solve ----
// The Worker's SolveRequest: {"images":[{"media_type":"image/jpeg","data":"<base64>"}, ...],
// "tutor":true}. The server owns the prompt, model, effort and output format. `detail` (the
// enhanced close-up, focus.h) becomes the second image when not empty. PNG bytes (a typed
// problem, png.h) are sent as image/png.
std::string buildWorkerSolveRequest(const std::string& image, const std::string& detail = "", bool tutor = false);

// What the firmware must do after a failed Worker call, besides showing the failure.
enum class WorkerAction {
  None,
  Reregister,  // 401 unauthorized: the token is unknown: forget it and register again
  Revoked,     // 401 device_revoked: removed from its account: clear history, forget the token
  Unlinked     // 402 device_not_linked: show the link code (WorkerFailure::pairCode)
};

// A finished /v1/solve (or any device call) -> a Failure with neutral wording (no vendor names),
// or false when it succeeded (status 200 and a complete stream). Account problems (not linked,
// trial over, subscription ended, monthly limit, too many solves, revoked) are Failure::Account
// with the server's message as `detail`, which the AI ACCOUNT screen shows.
struct WorkerFailure {
  Failure failure = Failure::ApiError;
  std::string detail;
  WorkerAction action = WorkerAction::None;
  std::string pairCode, site;  // from a device_not_linked message
};
bool classifyWorkerFailure(int httpStatus, const std::string& body, const StreamReader& stream, WorkerFailure& out);

// The error type of an {"type":"error","error":{...}} body, "" if it isn't one.
std::string workerErrorType(const std::string& body);
// Its message (at most 110 bytes, vendor names removed), "" if none.
std::string workerErrorMessage(const std::string& body);

// ---- firmware ----
struct FirmwareManifest {
  bool available = false;  // a different version is published FOR THIS BOARD FAMILY
  std::string version, sha256;
  std::string path;        // GET it as is: "/v1/firmware/image?family=v15lcd" when the check sent a family
  size_t size = 0;
  bool rollback = false;   // "rollback": true allows an older version (old proxy; Worker since 2026-10-11)
  std::string family;      // the manifest's `family` (Worker since 2026-10-11), "" when absent or null
  std::string note;        // why an announced image was skipped (other board family), for the log
};
// The board family of a firmware version: the text before the first '-'
// ("v15lcd-2026.10.11" -> "v15lcd").
std::string versionFamily(const std::string& version);
// GET /v1/firmware's path for this firmware: "/v1/firmware?family=v15lcd", or plain
// "/v1/firmware" (the legacy channel) when the family isn't 1-24 letters and digits.
std::string firmwareCheckPath(const std::string& running);
// GET /v1/firmware's reply (the old proxy and the Worker answer the same shape). `running`
// is this firmware's version ("v15lcd-2026.10.11"): an image of another family (its
// `family` field, or the text before the first '-' of its version) is not offered: the
// legacy channel announces whatever was published last, for every board.
bool parseFirmwareManifest(int httpStatus, const std::string& body, const std::string& running, FirmwareManifest& out,
                           std::string& error);

// ---- helpers ----
// "https://site/link?code=X" or "https://site/link" -> "https://site". Other URLs: trailing '/' removed.
std::string siteFromLinkUrl(const std::string& linkUrl);
// The first pair code in a text ("... enter code ABCD-EFGH") as "ABCD-EFGH", "" if none.
// Codes use ABCDEFGHJKLMNPQRSTUVWXYZ23456789 (no O, 0, I, 1).
std::string findPairCode(const std::string& text);
// Removes vendor names from a message meant for the screen (the Worker never sends any, but
// an upstream error passed through might); "" stays "". One pass over at most the first
// kNeutralMaxInput bytes (cut back to a word's end): callers show 110 at most.
constexpr size_t kNeutralMaxInput = 220;
std::string neutralMessage(const std::string& msg);

}  // namespace calc
