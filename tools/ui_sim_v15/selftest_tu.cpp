// The firmware's own self-test (firmware-v15-lcd/src/selftest.cpp), compiled unchanged
// by including it, with the board's sensors answered by the stubs below: every check
// passes and the 50 keys "arrive" one every 150 ms of fake time. Its screens (colour
// bars, key count, PASS) are what the board draws.
#include "../../firmware-v15-lcd/src/selftest.cpp"

#include "host_capture.h"

// ---- the board, as the self-test sees it
int batteryPercent() { return 87; }
int batteryMillivolts() { return 4050; }
bool usbPresent() { return true; }
bool statPinHigh() { return false; }
calc::Charge chargeState() { return calc::Charge::Charging; }
void powerLimitWifi() {}
const char* resetReasonText() { return "power-on"; }
const char* otaSlotName() { return "ota_0"; }
std::string deviceId() { return "A1B2C3"; }
bool keysScannerOk() { return true; }
static int g_nextKey = 0;
static uint32_t g_nextKeyAt = 0;
void keysPoll(void (*press)(calc::DKey)) {
  if (g_nextKey < kKeyCount && host::nowMs >= g_nextKeyAt) {
    g_nextKeyAt = host::nowMs + 150;
    press(calc::DKey(g_nextKey++));
  }
}
bool keysHeld(calc::DKey) { return g_nextKey >= kKeyCount && host::nowMs >= g_nextKeyAt + 3000; }
bool cameraSelfTest(std::string& sensor, size_t& bytes, uint32_t& ms, bool& autofocus) {
  sensor = "OV5640";
  bytes = 38211;
  ms = 412;
  autofocus = true;
  return true;
}
void cameraSleep() {}

// Runs the whole self-test; every frame it pushes is saved as <prefix>_NN.
void hostRunSelfTest(const std::string& prefix) {
  host::autoSave(prefix);
  Serial.quiet = true;
  try {
    selfTestRun("harness");
  } catch (const HostRestart&) {
  }
  Serial.quiet = false;
  host::autoSave("");
}
