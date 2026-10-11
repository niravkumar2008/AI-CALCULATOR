// Factory self-test: one boot mode that checks the whole board and prints the
// result as one JSON line over USB serial ("SELFTEST_JSON {...}").
//
// Start it by holding SHIFT + ALPHA and pressing ON (from off, or straight after
// plugging the battery in / pressing reset), or by typing  selftest  in the Serial
// Monitor. Checks, in order: chip and memory, keypad scanner, battery voltage and
// charge STAT, e-paper pattern (refresh timing = BUSY works), camera snapshot,
// Wi-Fi scan (RSSI, and the battery's dip while the radio transmits), then every
// key (press all 50; AC twice when done or 90 s timeout). The calculator restarts after.
#pragma once
#include "device.h"

// The key combination that starts the self-test with ON.
bool selfTestComboHeld();
[[noreturn]] void selfTestRun(const char* trigger);
