// One serial log style for the whole firmware: "[module] message". The module tag
// makes the Serial Monitor easy to filter ("[wifi]", "[cam]", "[ota]", ...). Interactive
// prompts (the setup questions) and machine-read lines (SELFTEST_JSON) stay unprefixed.
#pragma once
#include <Arduino.h>

#define LOGF(tag, fmt, ...) Serial.printf("[" tag "] " fmt "\n", ##__VA_ARGS__)
