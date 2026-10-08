// The 2.13" e-paper: shows the calculator's 125x61 screen at 2x size (250x122).
#pragma once
#include <stdint.h>

#include "framebuffer.h"
#include "viewfinder.h"

// wokeFromSleep: the picture on the glass is still the one from before the deep
// sleep, so nothing is redrawn until it changes.
void screenBegin(bool wokeFromSleep = false);
// Redraws only if the picture changed. Uses fast partial refresh, with a full
// (flashing) refresh every so often to clear ghosting.
void screenShow(const calc::Framebuffer& fb, bool dots);
// Factory self-test: a full refresh of a checkerboard + border, with rows 4-6 of
// `fb` underneath. Returns how long the refresh took (BUSY high time): about
// 1.5-3 s on a working panel, a few ms if BUSY never went high, ~10+ s if stuck.
uint32_t screenTestPattern(const calc::Framebuffer& fb);

// Camera viewfinder: shows the full-resolution 250x122 picture. full = a flashing
// full refresh (clears ghosting); otherwise a fast partial refresh (~0.3 s).
void screenShowPanel(const calc::Panel& p, bool full);
// Viewfinder over: the panel sleeps and the next screenShow() redraws in full.
void screenEndPanel();
