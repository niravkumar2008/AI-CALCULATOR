// Battery, charging, Wi-Fi power cap and deep sleep. Cell: Adafruit #1317 150 mAh LiPo
// (charged at 50 mA by the MCP73831; the limits below were tuned for the earlier 100 mAh
// cell and are conservative for this one).
//
// Off = deep sleep (about 8 uA for the chip; ~25-35 uA for the whole board with the
// RT9080 regulator; the v15 LCD is unpowered, Q4 off). Only ON (IO7) or a keypad key (TCA8418 INT, IO3 on v15) wakes it: the
// chip restarts and main.cpp puts the calculator back from RTC memory, so memories,
// history and settings survive. A USB cable can't wake it (VBUS_SENSE is not an RTC
// pin) but the charger works without the chip.
#pragma once
#include <stdint.h>

#include <string>

#include "battery.h"

void powerBegin();

// ---- battery and charging
int batteryMillivolts();       // averaged, through the 1 M / 1 M divider
int batteryPercent();          // LiPo curve (core/battery.h)
bool usbPresent();             // VBUS_SENSE high
calc::Charge chargeState();    // STAT is only read while VBUS is present (R20 pulls up to VBUS_SENSE)
bool statPinHigh();            // raw CHG_STAT level (self-test)
// AI requests make 350-500 mA Wi-Fi bursts: refused on a nearly empty cell (review S2/S4).
bool batteryOkForAi();
// A firmware download is one long burst: cable in, or the cell above minMv (ota.h).
bool batteryOkForOta(int minMv);

// ---- Wi-Fi: call right after WiFi.mode(WIFI_STA). Caps TX power at 11 dBm (review S4: a
// small cell sags 0.15-0.3 V at full power) and keeps modem sleep on.
void powerLimitWifi();
constexpr int8_t kWifiMaxTxQuarterDbm = 44;  // 44 x 0.25 dBm = 11 dBm

// ---- deep sleep
enum class Wake : uint8_t { ColdBoot, OnKey, Keypad, Other };
Wake powerWakeCause();         // why this boot happened (ColdBoot = power-up, reset, flash)
bool powerWokeFromSleep();     // OnKey or Keypad
// The calculator's state kept in RTC memory while asleep (see Device::saveState).
// powerTakeState gives it back once after a wake, with how long the chip slept.
constexpr size_t kRtcStateBytes = 2048;
bool powerTakeState(std::string& state, uint32_t& asleepMs);

// Puts the board in its lowest-current state and deep-sleeps; does not return
// (the next press restarts the firmware). keypadClear: the TCA8418 INT line went
// high after draining its FIFO, so it may be used as a wake source.
[[noreturn]] void powerDeepSleep(const std::string& state, bool keypadClear);

// Shuts the camera's GPIOs down for sleep: disabled, no pull-up / pull-down (review S6).
void powerCameraPinsSafe();

const char* resetReasonText();
