#include "vf_lcd.h"

#include <Arduino.h>

#include "glyphdraw.h"
#include "lcd.h"
#include "ui.h"

namespace gd = glyphdraw;
using ui::color::accent;
using ui::color::black;
using ui::color::dim;
using ui::color::good;
using ui::color::warn;
using ui::color::white;

namespace vflcd {
namespace {

constexpr int kW = lcd::kW, kH = lcd::kH;
constexpr int kTopBand = 16, kBottomBand = 18;  // solid bands for the text (readable over any picture)
constexpr int kThumbW = 80, kThumbH = 60;       // the grey copy fed to core's Viewfinder

// A sprite that points INTO the camera's frame buffer (no pixel copy), so LovyanGFX's
// primitives draw the overlay straight onto the picture.
LGFX_Sprite g_overlay;
bool g_overlayReady = false;

void bind(uint8_t* rgb565) {
  if (!g_overlayReady) {
    g_overlay.setColorDepth(16);
    g_overlayReady = true;
  }
  g_overlay.setBuffer(rgb565 + size_t(kCropY) * kFrameW * 2, kW, kH, 16);
}

void bracket(LGFX_Sprite& s, int x, int y, int dx, int dy, int len, uint16_t c) {
  s.fillRect(dx > 0 ? x : x - len + 1, y, len, 2, c);
  s.fillRect(x, dy > 0 ? y : y - len + 1, 2, len, c);
}

}  // namespace

void greyThumb(const uint8_t* rgb565, uint8_t* grey) {
  for (int y = 0; y < kThumbH; ++y) {
    const uint8_t* row = rgb565 + size_t(y * 4) * kFrameW * 2;
    for (int x = 0; x < kThumbW; ++x) {
      const uint8_t* p = row + size_t(x * 4) * 2;
#if PREVIEW_SWAP_BYTES
      const uint16_t v = uint16_t(p[0] | (p[1] << 8));
#else
      const uint16_t v = uint16_t((p[0] << 8) | p[1]);  // panel order: high byte first
#endif
      const int r = (v >> 11) & 0x1F, g = (v >> 5) & 0x3F, b = v & 0x1F;
      grey[y * kThumbW + x] = uint8_t((r * 8 * 77 + g * 4 * 151 + b * 8 * 28) >> 8);
    }
  }
}

void showFrame(uint8_t* rgb565, const Overlay& o) {
#if PREVIEW_SWAP_BYTES
  // The camera's byte order differs from the panel's: swap in place (~2 ms for the crop).
  uint8_t* p = rgb565 + size_t(kCropY) * kFrameW * 2;
  for (size_t i = 0; i < size_t(kW) * kH; ++i, p += 2) {
    const uint8_t t = p[0];
    p[0] = p[1];
    p[1] = t;
  }
#endif
  bind(rgb565);
  LGFX_Sprite& s = g_overlay;

  // Framing guide: the whole width is sent; the picture continues 35 px above and below
  // the screen (dashed edge marks say so), so the corner brackets sit at the screen edge.
  const uint16_t guide = o.focus == FocusState::Focused || o.focus == FocusState::Fixed ? good : white;
  bracket(s, 4, kTopBand + 3, 1, 1, 18, guide);
  bracket(s, kW - 5, kTopBand + 3, -1, 1, 18, guide);
  bracket(s, 4, kH - kBottomBand - 4, 1, -1, 18, guide);
  bracket(s, kW - 5, kH - kBottomBand - 4, -1, -1, 18, guide);
  for (int x = 40; x < kW - 40; x += 24) {  // the dashed marks: "the photo goes on past here"
    s.fillRect(x, kTopBand, 10, 1, dim);
    s.fillRect(x, kH - kBottomBand - 1, 10, 1, dim);
  }
  // Centre cross
  s.fillRect(kW / 2 - 8, kH / 2, 17, 1, guide);
  s.fillRect(kW / 2, kH / 2 - 8, 1, 17, guide);
  // The writing the analysis found, in thumbnail pixels (x4) minus the crop.
  if (o.info.textFound && o.info.tw > 0) {
    int x = o.info.tx * 4, y = o.info.ty * 4 - kCropY, w = o.info.tw * 4, h = o.info.th * 4;
    if (y < kTopBand) { h -= kTopBand - y; y = kTopBand; }
    if (y + h > kH - kBottomBand) h = kH - kBottomBand - y;
    if (w > 4 && h > 4) s.drawRect(x, y, w, h, warn);
  }

  // Top band: focus state on the left, mode in the middle, battery / Wi-Fi on the right.
  s.fillRect(0, 0, kW, kTopBand, black);
  const char* focusText = "";
  uint16_t focusColor = dim;
  switch (o.focus) {
    case FocusState::Focusing: focusText = "FOCUSING"; focusColor = warn; break;
    case FocusState::Focused: focusText = "FOCUSED"; focusColor = good; break;
    case FocusState::Fixed: focusText = "FIXED FOCUS"; focusColor = dim; break;
    case FocusState::None: focusText = "NO AF"; focusColor = dim; break;
  }
  if (o.info.state == calc::Focus3::Moving) {
    focusText = "HOLD STILL";
    focusColor = warn;
  }
  s.fillCircle(8, kTopBand / 2, 3, focusColor);
  gd::text(s, 16, 1, focusText, 2, focusColor);
  gd::textCentered(s, kW / 2 + 10, 1, o.mode, 2, accent);
  char right[24];
  snprintf(right, sizeof right, "%s%d%%", o.charging ? "+" : "", o.battery < 0 ? 0 : o.battery);
  if (o.battery >= 0) gd::textRight(s, kW - 4, 1, right, 2, o.battery <= 20 && !o.charging ? ui::color::error : dim);
  if (o.wifiConnected) s.fillRect(kW - 50, 4, 8, 8, accent);

  // Bottom band: key hints, time left, fps.
  s.fillRect(0, kH - kBottomBand, kW, kBottomBand, black);
  int x = 4;
  auto hint = [&](const char* key, const char* what) {
    const int kw = gd::width(key, 2) + 4;
    s.fillRoundRect(x, kH - kBottomBand + 2, kw, 14, 2, dim);
    gd::text(s, x + 2, kH - kBottomBand + 2, key, 2, black);
    x += kw + 3;
    x = gd::text(s, x, kH - kBottomBand + 2, what, 2, dim) + 8;
  };
  hint("=", "Scan");
  hint("\xE2\x96\xB6", "Focus");
  hint("\xE2\x96\xB2\xE2\x96\xBC", "Effort");
  hint("AC", "Back");
  char tail[24];
  if (o.secondsLeft <= 10) snprintf(tail, sizeof tail, "off in %lus", (unsigned long)o.secondsLeft);
  else snprintf(tail, sizeof tail, "%.0f fps", o.fps);
  gd::textRight(s, kW - 4, kH - kBottomBand + 2, tail, 2, o.secondsLeft <= 10 ? warn : dim);

  lcd::pushRaw(0, reinterpret_cast<const uint16_t*>(rgb565 + size_t(kCropY) * kFrameW * 2), kH);
}

void showStarting(const std::string& what) {
  LGFX_Sprite& s = lcd::canvas();
  s.fillSprite(black);
  s.fillRect(0, 0, kW, kTopBand, black);
  gd::textCentered(s, kW / 2, kH / 2 - 20, "CAMERA", 3, dim);
  gd::textCentered(s, kW / 2, kH / 2 + 8, what, 2, accent);
  gd::textCentered(s, kW / 2, kH - 16, "AC: back", 2, dim);
  lcd::flush();
}

}  // namespace vflcd
