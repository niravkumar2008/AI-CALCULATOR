// Over-the-air firmware updates from the AI server (review S1): the proxy or the Worker
// (Settings::api), which answer GET /v1/firmware and /v1/firmware/image the same way.
//
// The flash holds two app slots (platformio.ini: min_spiffs.csv, 1.9 MB each). An update
// is downloaded from the proxy (GET /v1/firmware for the manifest, then the image), its
// SHA-256 is checked against the manifest before it is accepted, and it is written to the
// slot that is not running. The next restart boots it. If that boot crashes before the
// firmware confirms itself (otaConfirm), the bootloader goes back to the old slot
// (CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is on in this SDK; verifyRollbackLater() in
// ota.cpp stops the Arduino core from confirming blindly at start).
//
// The image is sent over TLS to a root-pinned proxy that only answers this calculator's
// device token, and its hash is checked; there is no signing key in the firmware.
// Before the slot is marked bootable the image itself is checked too (review F-18): it must
// carry this firmware's version mark (ota.cpp) for the same board family, the version the
// manifest announced, not the running one, and not older unless the manifest says
// "rollback": true. That guards against a wrong upload, not an attacker: image signing
// (secure boot) is still out of scope. Images built before this check have no mark, so
// they can't be installed over the air from here on.
#pragma once
#include <functional>
#include <string>

#include "settings.h"

struct OtaInfo {
  bool available = false;  // the proxy has an image that differs from the running version
  std::string version;     // its version string
  std::string sha256;      // 64 hex characters
  std::string path;        // where to GET it, e.g. "/v1/firmware/image"
  size_t size = 0;
  bool rollback = false;   // manifest "rollback": true allows an older version than the running one
};

// Only on a cable, or a well-charged cell: a download is a long Wi-Fi burst.
constexpr int kOtaMinBatteryMv = 3800;

// Asks the proxy (Wi-Fi must be connected). False = couldn't ask; `error` says why.
bool otaCheck(const Settings& s, OtaInfo& info, std::string& error);
// Downloads, checks the hash and writes the other slot. True = restart to run it.
// progress(percent) is called as bytes arrive (for the e-paper).
bool otaInstall(const Settings& s, const OtaInfo& info, const std::function<void(int)>& progress, std::string& error);

// This boot is the first run of a just-installed image: it must confirm itself.
bool otaPendingVerify();
// The new image works (keypad answers, the loop runs): keep it. Safe to call any time.
void otaConfirm();
// "app0" / "app1": which slot is running, for status and logs.
const char* otaSlotName();
