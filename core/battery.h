// Battery maths for the LiPo cell (Adafruit #1317, 150 mAh; pure functions, unit-tested on a PC).
#pragma once
#include <cstdint>

namespace calc {

// State of charge (0-100) from the cell voltage in mV, along a typical LiPo
// discharge curve under a light load (the curve is flat between 3.7 and 3.9 V,
// so a straight line from 3.3 to 4.2 V would read 30 % too high mid-way).
int lipoPercent(int millivolts);

enum class Charge : uint8_t { NoCable, Charging, Full };

// MCP73831 STAT through D6 with R20 pulled up to VBUS_SENSE: low = charging,
// high = done. Without a cable R20 has no supply and STAT reads low, so STAT
// only means something while VBUS is present.
Charge chargeState(bool vbusPresent, bool statHigh);

// Below this the AI (Wi-Fi bursts of ~350-500 mA) is refused on battery: the cell
// sags 0.15-0.3 V and the 3.3 V regulator drops out (review S2/S4). Tuned for the
// original 100 mAh cell; conservative for the 150 mAh #1317 fitted now.
constexpr int kAiMinMillivolts = 3600;
// The lock-out ends only above this (hysteresis: a sagging cell must not flicker
// between "AI off" and "AI on" every few seconds).
constexpr int kAiResumeMillivolts = 3700;

// Averages ADC readings and drops the highest and lowest (Wi-Fi bursts and
// e-paper refreshes pull the cell down for a moment). n <= 16.
int robustAverage(const int* samples, int n);

}  // namespace calc
