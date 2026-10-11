#include "ota.h"

#include <Arduino.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

#include <cctype>

#include "claude_client.h"
#include "json.h"
#include "log.h"

// The Arduino core (esp32-hal-misc.c, a C file: hence the C linkage) confirms a new
// image at start unless this returns true; the firmware confirms it itself once the
// board has proved to work (main.cpp: serviceOtaConfirm).
extern "C" bool verifyRollbackLater() { return true; }

namespace {
std::string hexOf(const unsigned char* d, size_t n) {
  static const char* kHex = "0123456789abcdef";
  std::string s;
  for (size_t i = 0; i < n; ++i) {
    s += kHex[d[i] >> 4];
    s += kHex[d[i] & 15];
  }
  return s;
}

std::string lower(std::string s) {
  for (char& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
  return s;
}
}  // namespace

bool otaCheck(const Settings& s, OtaInfo& info, std::string& error) {
  info = OtaInfo{};
  std::string reply;
  const int status = proxyRequest(s, "GET", "/v1/firmware", "", reply);
  if (status == 0) {
    error = reply.empty() ? "No reply from the AI server." : reply;
    return false;
  }
  calc::Json j;
  std::string jsonErr;
  if (!calc::Json::parse(reply, j, jsonErr)) {
    error = "The update manifest couldn't be read (HTTP " + std::to_string(status) + ").";
    return false;
  }
  if (status != 200) {
    error = j["error"]["message"].isString() ? j["error"]["message"].asString()
                                             : "The AI server refused the update check (HTTP " + std::to_string(status) + ").";
    return false;
  }
  info.available = j["update"].isBool() && j["update"].asBool();
  if (j["version"].isString()) info.version = j["version"].asString();
  if (!info.available) return true;
  if (!j["sha256"].isString() || !j["size"].isNumber() || !j["path"].isString()) {
    error = "The update manifest is incomplete.";
    info.available = false;
    return false;
  }
  info.sha256 = lower(j["sha256"].asString());
  info.size = size_t(j["size"].asNumber());
  info.path = j["path"].asString();
  if (info.sha256.size() != 64 || info.size < 100000 || info.path.empty() || info.path[0] != '/') {
    error = "The update manifest looks wrong.";
    info.available = false;
    return false;
  }
  return true;
}

bool otaInstall(const Settings& s, const OtaInfo& info, const std::function<void(int)>& progress, std::string& error) {
  if (!info.available) {
    error = "No update to install.";
    return false;
  }
  const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
  if (!next) {
    error = "This flash has only one app slot (partition table without OTA).";
    return false;
  }
  if (info.size > next->size) {
    error = "The update (" + std::to_string(info.size / 1024) + " KB) doesn't fit the " +
            std::to_string(next->size / 1024) + " KB slot.";
    return false;
  }
  if (!Update.begin(info.size, U_FLASH)) {
    error = std::string("Couldn't start the update: ") + Update.errorString();
    return false;
  }
  LOGF("ota", "downloading %s (%u KB) into %s", info.version.c_str(), unsigned(info.size / 1024), next->label);

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts_ret(&sha, 0);
  size_t written = 0, contentLength = 0;
  int lastPercent = -1;
  bool writeFailed = false;
  std::string why;
  const int status = proxyDownload(s, info.path, [&](const char* p, size_t n) {
    if (written + n > info.size) n = info.size - written;  // never past the announced size
    if (n == 0) return false;
    mbedtls_sha256_update_ret(&sha, reinterpret_cast<const unsigned char*>(p), n);
    if (Update.write(reinterpret_cast<uint8_t*>(const_cast<char*>(p)), n) != n) {
      writeFailed = true;
      return false;
    }
    written += n;
    const int percent = int(written * 100 / info.size);
    if (progress && percent != lastPercent && (percent % 5 == 0 || percent == 100)) {
      lastPercent = percent;
      progress(percent);
    }
    return true;
  }, contentLength, why);

  unsigned char digest[32];
  mbedtls_sha256_finish_ret(&sha, digest);
  mbedtls_sha256_free(&sha);
  const std::string got = hexOf(digest, sizeof digest);

  if (status != 200 || writeFailed || written != info.size) {
    Update.abort();
    if (writeFailed) error = std::string("Flash write failed: ") + Update.errorString();
    else if (status == 0) error = why.empty() ? "The download didn't start." : why;
    else if (status != 200) error = "The AI server refused the image (HTTP " + std::to_string(status) + ").";
    else error = "The download stopped at " + std::to_string(written / 1024) + " of " + std::to_string(info.size / 1024) + " KB.";
    LOGF("ota", "failed: %s", error.c_str());
    return false;
  }
  if (got != info.sha256) {
    Update.abort();
    error = "The image's hash doesn't match the manifest: not installed.";
    LOGF("ota", "hash mismatch: got %s want %s", got.c_str(), info.sha256.c_str());
    return false;
  }
  if (!Update.end(true)) {
    error = std::string("Couldn't finish the update: ") + Update.errorString();
    LOGF("ota", "end failed: %s", error.c_str());
    return false;
  }
  LOGF("ota", "installed %s in %s (sha256 ok); restart to run it", info.version.c_str(), next->label);
  return true;
}

bool otaPendingVerify() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  return running && esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY;
}

void otaConfirm() {
  if (!otaPendingVerify()) return;
  if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) LOGF("ota", "new firmware confirmed (%s)", otaSlotName());
  else LOGF("ota", "couldn't confirm the new firmware");
}

const char* otaSlotName() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  return running ? running->label : "?";
}
