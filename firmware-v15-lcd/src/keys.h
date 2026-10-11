// The keys: the whole Casio keypad (an 8 x 10 matrix read by a TCA8418 keypad
// scanner) + ON on its own pin.
#pragma once
#include "device.h"

// keepEvents: after a deep-sleep wake, keep the scanner's queued key events (the
// key that woke the chip); a cold start drops stale ones.
void keysBegin(bool keepEvents = false);
void keysWokeByOn();      // the chip woke from deep sleep because ON was pressed
bool keysScannerOk();     // the TCA8418 answered on I2C
bool keysHeld(calc::DKey k);  // held right now (ON: read from its pin)
// Before deep sleep: empties the scanner's queue and clears INT. True if INT is
// high afterwards (then a keypad key may wake the chip).
bool keysPrepareSleep();
// Calls `press` once per key press since the last call, plus repeats while
// a scrolling key is held (▲ ▼ ◀ ▶ DEL). Presses are
// caught by interrupts (or the scanner's own memory), so none are lost while
// the e-paper is busy refreshing.
void keysPoll(void (*press)(calc::DKey));

