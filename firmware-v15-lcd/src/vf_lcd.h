// The live colour viewfinder on the LCD (AI SOLVE home screen): the OV5640 streams
// 320x240 RGB565 frames; the middle 320x170 of each frame is shown 1:1 with an overlay
// (framing guide, focus state, the box around writing the core found, effort / tutor,
// key hints) drawn straight into the camera's frame buffer, then pushed to the panel.
// No copies: one frame = one DMA grab + one SPI push (~22 ms).
//
// The frame analysis (sharpness, motion, text box, time-out) is still core/viewfinder.h:
// it is fed a small grey copy of each frame and this module only draws its FrameInfo.
#pragma once
#include <cstdint>
#include <string>

#include "viewfinder.h"

namespace vflcd {

// The rows of the 320x240 camera frame that the 320x170 screen shows.
constexpr int kFrameW = 320, kFrameH = 240;
constexpr int kCropY = (kFrameH - 170) / 2;  // 35

enum class FocusState : uint8_t { None, Focusing, Focused, Fixed };

struct Overlay {
  calc::FrameInfo info;      // from Viewfinder::feed()
  FocusState focus = FocusState::None;
  uint32_t secondsLeft = 0;  // until the camera times out
  std::string mode;          // "Normal", "Careful +Tutor"...
  double fps = 0;
  int battery = -1;
  bool charging = false;
  bool wifiConnected = false;
};

// Draws the overlay into `rgb565` (320 x 240, panel byte order) and pushes rows kCropY..+170.
void showFrame(uint8_t* rgb565, const Overlay& o);
// The screen while the camera powers up (no frame yet).
void showStarting(const std::string& what);
// Grey 80x60 copy of a 320x240 RGB565 frame (every 4th pixel) for core's analysis.
void greyThumb(const uint8_t* rgb565, uint8_t* grey80x60);

}  // namespace vflcd
