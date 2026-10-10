// The live camera viewfinder on the e-paper (AI SOLVE), like a phone's camera
// screen: a small grayscale frame -> contrast stretch -> 4x4 Bayer dither at the
// panel's full 250x122 resolution, with corner brackets, a centre crosshair, a
// focus / hold-still indicator and a box around text it finds.
//
// Pure image maths with no hardware, so it runs (and is golden-tested) on a PC.
// The firmware feeds it frames from the camera; the simulator from a picture.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace calc {

// The e-paper panel at its real resolution, one bit per pixel (true = black).
class Panel {
 public:
  static constexpr int kWidth = 250;
  static constexpr int kHeight = 122;

  void clear();
  void set(int x, int y, bool on);
  bool get(int x, int y) const;
  void invert(int x, int y);
  void fillRect(int x, int y, int w, int h, bool on);
  void drawText(int x, int y, const std::string& utf8);  // 5x7 font, 1x
  int diff(const Panel& other) const;                     // pixels that differ
  std::string toAscii() const;                            // for golden tests
  // Rows of 32 bytes (256 bits, MSB = leftmost pixel, 1 = black): the layout
  // Adafruit-GFX drawBitmap() takes, so the firmware sends it in one call.
  const uint8_t* bits() const { return &px_[0][0]; }
  static constexpr int kBytesPerRow = 32;

 private:
  static constexpr int kStride = kBytesPerRow;  // 3.9 KB for the panel
  uint8_t px_[kHeight][kStride] = {};
};

enum class Focus3 : uint8_t { Starting, Moving, Focusing, Sharp };

struct FrameInfo {
  double sharpness = 0;  // variance of the Laplacian in the centre, contrast-normalised
  double motion = 0;     // mean change from the previous frame (gray levels, 0-255)
  Focus3 state = Focus3::Starting;
  bool textFound = false;
  int tx = 0, ty = 0, tw = 0, th = 0;  // text box in frame pixels
  int lo = 0, hi = 255;                // contrast stretch range
};

class Viewfinder {
 public:
  // Where the photo goes on the panel: the whole 4:3 frame (exactly what is sent),
  // 162x122 at the left; the right 84 px hold the status text.
  static constexpr int kImgW = 162;
  static constexpr int kImgH = 122;
  static constexpr int kSideX = 166;
  static constexpr uint32_t kTimeoutMs = 30000;  // no key for 30 s: camera off
  // Tuning (real hardware may need other values: see FIRMWARE_STAGE13.md)
  static constexpr double kMotionMax = 10.0;     // mean gray change per frame above this = moving
  static constexpr double kSharpMin = 60.0;      // normalised Laplacian variance below this = soft
  int fullEvery = 10;                            // partial refreshes between full refreshes
  int ghostBudget = 3 * kImgW * kImgH;           // flipped pixels before a full refresh is due

  void start(uint32_t nowMs);
  void stop() { running_ = false; }
  bool running() const { return running_; }
  void touch(uint32_t nowMs) { lastKeyMs_ = nowMs; }  // a key press restarts the timeout
  bool timedOut(uint32_t nowMs) const { return running_ && nowMs - lastKeyMs_ >= kTimeoutMs; }
  uint32_t secondsLeft(uint32_t nowMs) const;

  // One camera frame, 8-bit gray, any size (4:3 expected). afFocused: the camera's
  // own autofocus says it is in focus (true for fixed-focus modules).
  const FrameInfo& feed(const uint8_t* gray, int w, int h, bool afFocused, uint32_t nowMs);
  const FrameInfo& info() const { return info_; }

  // Draws the last frame + overlay + side panel. `extra` is one line under the
  // controls (e.g. "Tutor on"). Call after feed().
  void render(Panel& out, uint32_t nowMs, const std::string& extra = "") const;
  // The panel shown while the camera starts (no frame yet).
  void renderStarting(Panel& out) const;

  // After render(): whether this frame should be a full (flashing) refresh to clear
  // e-paper ghosting: every `fullEvery` frames, or when many pixels have flipped.
  bool takeFullRefresh(const Panel& now);

  int frames() const { return frames_; }
  double fps(uint32_t nowMs) const;

 private:
  bool running_ = false;
  uint32_t startMs_ = 0, lastKeyMs_ = 0;
  int frames_ = 0;
  int sinceFull_ = 0;
  long flipped_ = 0;
  Panel shown_;
  bool haveShown_ = false;
  FrameInfo info_;
  std::vector<uint8_t> image_;  // kImgW x kImgH stretched gray
  std::vector<uint8_t> prev_;   // previous frame, 32x24, for motion
  int frameW_ = 0, frameH_ = 0; // size of the last frame fed
};

// Variance of the 4-neighbour Laplacian over the centre half of the frame,
// divided by the frame's contrast squared (x 1000): high = sharp edges.
double focusMeasure(const uint8_t* gray, int w, int h, int lo, int hi);

}  // namespace calc
