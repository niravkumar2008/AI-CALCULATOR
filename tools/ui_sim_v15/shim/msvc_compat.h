// Forced-include for the MSVC host build (cl /FI): LovyanGFX and the firmware are
// written for GCC. Nothing here changes what gets drawn.
#pragma once
#include <stdint.h>
#include <stddef.h>
#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(x)
#endif
