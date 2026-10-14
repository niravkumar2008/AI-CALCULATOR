#include "ota.h"

#include <Arduino.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

#include <cctype>
#include <vector>

#include "claude_client.h"
#include "log.h"
#include "worker_api.h"

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

// ---- the image's own version mark (review F-18) ----
// The ESP-IDF app descriptor in an Arduino image only says "arduino-lib-builder", so every
// image carries this mark instead: STX "AICALC-FWID:" <version> ETX. otaInstall() finds it
// in the downloaded bytes. Referenced below, so the linker keeps it.
const char kFirmwareMark[] = "\x02" "AICALC-FWID:" CALC_FIRMWARE_VERSION "\x03";
constexpr size_t kMarkPrefix = 13;  // STX + "AICALC-FWID:"
constexpr size_t kMarkVersionMax = 48;

// Finds the first complete mark in a byte stream fed in pieces.
struct MarkScanner {
  size_t matched = 0;  // prefix characters matched so far
  bool inVersion = false;
  std::string version, found;

  void feed(const char* p, size_t n) {
    for (size_t i = 0; i < n && found.empty(); ++i) {
      const char c = p[i];
      if (inVersion) {
        if (c == '\x03' && !version.empty()) {
          found = version;
        } else if (static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) > 0x7e ||
                   version.size() >= kMarkVersionMax) {
          inVersion = false;  // not a mark (e.g. the bare prefix): keep looking
          matched = c == kFirmwareMark[0] ? 1 : 0;
        } else {
          version += c;
        }
        continue;
      }
      // STX appears only at the start of the prefix, so a mismatch restarts at 0 or 1.
      if (c == kFirmwareMark[matched]) ++matched;
      else matched = c == kFirmwareMark[0] ? 1 : 0;
      if (matched == kMarkPrefix) {
        inVersion = true;
        version.clear();
        matched = 0;
      }
    }
  }
};

using calc::versionFamily;  // core/worker_api: the text before the first '-'

// The numbers after the family ("v15lcd-2026.10.10" -> 2026, 10, 10). False if there are none.
bool versionNumbers(const std::string& v, std::vector<long>& out) {
  out.clear();
  const size_t dash = v.find('-');
  if (dash == std::string::npos) return false;
  long n = -1;
  for (size_t i = dash + 1; i <= v.size(); ++i) {
    const char c = i < v.size() ? v[i] : '.';
    if (c >= '0' && c <= '9') {
      n = (n < 0 ? 0 : n * 10) + (c - '0');
      if (n > 100000000) return false;
    } else if (n >= 0) {
      out.push_back(n);
      n = -1;
    }
  }
  return !out.empty();
}

// "" if the downloaded image may replace the running one, else why not.
std::string imageRefusal(const std::string& image, const OtaInfo& info) {
  const std::string running = kFirmwareVersion;
  if (image.empty()) return "The image has no AI Calculator version mark: not installed.";
  if (versionFamily(image) != versionFamily(running))
    return "The image is for another board (" + versionFamily(image) + "): not installed.";
  if (image != info.version) return "The image is " + image + ", not the announced " + info.version + ".";
  if (image == running) return "The image is the version already running.";
  std::vector<long> a, b;
  if (versionNumbers(image, a) && versionNumbers(running, b) && a < b && !info.rollback)
    return "The image (" + image + ") is older than this firmware: not installed.";
  return "";
}
}  // namespace

bool otaCheck(const Settings& s, OtaInfo& info, std::string& error) {
  info = OtaInfo{};
  std::string reply;
  // "?family=v15lcd": the Worker answers this board family's channel only (the old proxy
  // ignores the query and goes by x-firmware). The manifest's path carries it on.
  const int status = proxyRequest(s, "GET", calc::firmwareCheckPath(kFirmwareVersion), "", reply);
  if (status == 0) {
    error = reply.empty() ? "No reply from the AI server." : reply;
    return false;
  }
  // The proxy and the Worker answer the same shape (core/worker_api: parseFirmwareManifest,
  // unit-tested). It also drops an image of another board family before any download: the
  // Worker announces whatever was published last, for every board.
  calc::FirmwareManifest m;
  if (!calc::parseFirmwareManifest(status, reply, kFirmwareVersion, m, error)) return false;
  if (!m.note.empty()) LOGF("ota", "%s", m.note.c_str());
  info.available = m.available;
  info.version = m.version;
  info.sha256 = m.sha256;
  info.size = m.size;
  info.path = m.path;
  info.rollback = m.rollback;
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
  MarkScanner mark;
  const int status = proxyDownload(s, info.path, [&](const char* p, size_t n) {
    if (written + n > info.size) n = info.size - written;  // never past the announced size
    if (n == 0) return false;
    mbedtls_sha256_update_ret(&sha, reinterpret_cast<const unsigned char*>(p), n);
    mark.feed(p, n);
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
  // The hash only proves the bytes are the ones the proxy announced; this checks they are
  // a firmware for this board, newer than this one (review F-18).
  const std::string refusal = imageRefusal(mark.found, info);
  if (!refusal.empty()) {
    Update.abort();
    error = refusal;
    LOGF("ota", "refused (image mark \"%s\", running %s): %s", mark.found.c_str(), kFirmwareVersion, refusal.c_str());
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
