// The keys: the whole Casio keypad (a 7 x 7 matrix read by a TCA8418 keypad
// scanner) + ON on its own pin.
#pragma once
#include "device.h"

void keysBegin();
// Calls `press` once per key press since the last call, plus repeats while
// a scrolling key is held (▲ ▼ ◀ ▶ DEL). Presses are
// caught by interrupts (or the scanner's own memory), so none are lost while
// the e-paper is busy refreshing.
void keysPoll(void (*press)(calc::DKey));
// Light-sleeps the chip until any key is pressed (the calculator is off).
void keysSleepUntilPress();
