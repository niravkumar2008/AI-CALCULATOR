// The 2.13" e-paper: shows the calculator's 125x61 screen at 2x size (250x122).
#pragma once
#include "framebuffer.h"

void screenBegin();
// Redraws only if the picture changed. Uses fast partial refresh, with a full
// (flashing) refresh every so often to clear ghosting.
void screenShow(const calc::Framebuffer& fb, bool dots);
