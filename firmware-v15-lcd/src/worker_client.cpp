#include "worker_client.h"

#include <algorithm>

#include "claude_client.h"

using namespace calc;

namespace {
WorkerAction actionFor(int status, const std::string& body) {
  if (status != 401) return WorkerAction::None;
  const std::string t = workerErrorType(body);
  if (t == "device_revoked") return WorkerAction::Revoked;
  if (t == "unauthorized") return WorkerAction::Reregister;
  return WorkerAction::None;
}
}  // namespace

int workerRegister(const Settings& s, WorkerRegistration& out, std::string& error) {
  std::string reply;
  std::string body = buildRegisterBody(deviceId(), factorySecret());
  const int status = proxyRequest(s, "POST", "/v1/device/register", body, reply, false);
  std::fill(body.begin(), body.end(), '\0');  // the secret doesn't linger in the heap
  if (status == 0) {
    out = WorkerRegistration{};
    error = reply.empty() ? "No reply from the AI server." : reply;
    return 0;
  }
  parseRegisterReply(status, reply, out, error);
  return status;
}

int workerStatus(const Settings& s, WorkerStatus& out, std::string& error, WorkerAction& action) {
  std::string reply;
  const int status = proxyRequest(s, "GET", "/v1/device/status", "", reply);
  action = actionFor(status, reply);
  if (status == 0) {
    out = WorkerStatus{};
    error = reply;
    return 0;
  }
  parseWorkerStatus(status, reply, out, error);
  return status;
}

int workerPairStart(const Settings& s, WorkerPairing& out, std::string& error, WorkerAction& action) {
  std::string reply;
  const int status = proxyRequest(s, "POST", "/v1/pair/start", "", reply);
  action = actionFor(status, reply);
  if (status == 0) {
    out = WorkerPairing{};
    error = reply;
    return 0;
  }
  parsePairStart(status, reply, out, error);
  return status;
}
