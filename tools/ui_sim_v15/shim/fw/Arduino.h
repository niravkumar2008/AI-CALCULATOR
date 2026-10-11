// Host stand-in for the ESP32 Arduino core, just enough for the firmware files the
// screenshot harness compiles unchanged (ui.cpp, vf_lcd.cpp, selftest.cpp). Time is a
// fake clock (host_hw.cpp) so the self-test runs in an instant and repeatably.
#pragma once
#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

using std::max;
using std::min;

#define LOW 0
#define HIGH 1

uint32_t millis();
void delay(uint32_t ms);
int digitalRead(int pin);
uint32_t esp_random();

struct HostSerial {
  bool quiet = false;
  void printf(const char* fmt, ...);
  int available() { return 0; }
  int read() { return -1; }
  void flush() { std::fflush(stdout); }
};
extern HostSerial Serial;

// ESP.restart() never returns on the chip; here it throws, so a harness can call
// selfTestRun() (declared [[noreturn]]) and carry on.
struct HostRestart {};
struct HostEsp {
  uint32_t getPsramSize() { return 2u * 1024 * 1024; }
  uint32_t getFlashChipSize() { return 4u * 1024 * 1024; }
  [[noreturn]] void restart() { throw HostRestart{}; }
};
extern HostEsp ESP;
