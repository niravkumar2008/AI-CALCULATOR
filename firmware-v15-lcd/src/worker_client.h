// The Cloudflare Worker's device calls besides /v1/solve (that one is claudeSolve): register,
// status and a fresh link code. HTTPS through claude_client.cpp's connection code; the
// bodies and replies are core/worker_api.* (unit-tested on a PC). Wi-Fi must be connected.
// Each call blocks ~1-2 s (TLS handshake). Nothing here touches NVS or the Device: main.cpp
// stores the token and updates the Link screen. Each returns the HTTP status (0 = no
// connection); it worked when `error` is empty.
#pragma once
#include <string>

#include "settings.h"
#include "worker_api.h"

// POST /v1/device/register with this board's hw_id (deviceId()) and, when the board was
// enrolled at QA, its hw_secret from NVS "factory" (never logged).
int workerRegister(const Settings& s, calc::WorkerRegistration& out, std::string& error);

// GET /v1/device/status. `action` is Reregister (unknown token) or Revoked after a 401.
int workerStatus(const Settings& s, calc::WorkerStatus& out, std::string& error, calc::WorkerAction& action);

// POST /v1/pair/start: a new link code (replaces the old one) or already_linked.
int workerPairStart(const Settings& s, calc::WorkerPairing& out, std::string& error, calc::WorkerAction& action);
