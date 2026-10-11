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
  // Laid out from measured widths with the font's tight advance (the band is 16 px):
  // right cluster first, then the focus text, then the mode centred in what is left.
  s.fillRect(0, 0, kW, kTopBand, black);
  const int adv = gd::tight(2);
  const char* focusText = "";
  const char* focusShort = "";
  uint16_t focusColor = dim;
  switch (o.focus) {
    case FocusState::Focusing: focusText = focusShort = "FOCUSING"; focusColor = warn; break;
    case FocusState::Focused: focusText = focusShort = "FOCUSED"; focusColor = good; break;
    case FocusState::Fixed: focusText = "FIXED FOCUS"; focusShort = "FIXED"; focusColor = dim; break;
    case FocusState::None: focusText = focusShort = "NO AF"; focusColor = dim; break;
  }
  if (o.info.state == calc::Focus3::Moving) {
    focusText = focusShort = "HOLD STILL";
    focusColor = warn;
  }
  int right = kW - 4;
  if (o.battery >= 0) {
    char pct[24];
    snprintf(pct, sizeof pct, "%s%d%%", o.charging ? "+" : "", o.battery);
    gd::textRight(s, right + 2, 1, pct, 2, o.battery <= 20 && !o.charging ? ui::color::error : dim, adv);
    right -= gd::width(pct, 2, adv) + 4;
  }
  if (o.wifiConnected) {
    right -= 8;
    s.fillRect(right, 4, 8, 8, accent);
    right -= 6;
  }
  s.fillCircle(8, kTopBand / 2, 3, focusColor);
  // The mode centred between the focus text and the right cluster; if it doesn't fit, the
  // short focus text ("FIXED"), and if it still doesn't, the mode is left out (the focus
  // state matters more while aiming).
  const int modeW = gd::width(o.mode, 2, adv);
  const char* f = focusText;
  int left = 16 + gd::width(f, 2, adv) + 8;
  if (right - left < modeW) {
    f = focusShort;
    left = 16 + gd::width(f, 2, adv) + 8;
  }
  gd::text(s, 16, 1, f, 2, focusColor, adv);
  if (right - left >= modeW) {
    int mx = (kW - modeW) / 2 + 10;  // centred on the screen when there's room
    if (mx < left) mx = left;
    if (mx + modeW > right) mx = right - modeW;
    gd::text(s, mx, 1, o.mode, 2, accent, adv);
  }

  // Bottom band: key hints on the left, the time-out or fps on the right. The right text
  // is placed first; a hint that would run into it is left out (Effort goes first: the
  // mode text above shows it).
  s.fillRect(0, kH - kBottomBand, kW, kBottomBand, black);
  char tail[24];
  if (o.secondsLeft <= 10) snprintf(tail, sizeof tail, "off in %lus", (unsigned long)o.secondsLeft);
  else snprintf(tail, sizeof tail, "%.0f fps", o.fps);
  const int tailX = kW - 4 - gd::width(tail, 2, adv) + 2;
  gd::text(s, tailX, kH - kBottomBand + 2, tail, 2, o.secondsLeft <= 10 ? warn : dim, adv);
  struct Hint {
    const char* key;
    const char* what;
  };
  auto hintW = [&](const Hint& h) { return gd::width(h.key, 2, adv) + 2 + 3 + gd::width(h.what, 2, adv); };
  const Hint all[] = {{"=", "Scan"}, {"\xE2\x96\xB6", "Focus"}, {"\xE2\x96\xB2\xE2\x96\xBC", "Effort"}, {"AC", "Back"}};
  constexpr int kHintGap = 6;
  bool use[4] = {true, true, true, true};
  auto total = [&] {
    int w = 4;
    for (int i = 0; i < 4; ++i)
      if (use[i]) w += hintW(all[i]) + kHintGap;
    return w;
  };
  if (total() > tailX - 4) use[2] = false;  // Effort
  if (total() > tailX - 4) use[1] = false;  // Focus
  int x = 4;
  for (int i = 0; i < 4; ++i) {
    if (!use[i]) continue;
    const int kw = gd::width(all[i].key, 2, adv) + 2;
    s.fillRoundRect(x, kH - kBottomBand + 2, kw, 14, 2, dim);
    gd::text(s, x + 2, kH - kBottomBand + 2, all[i].key, 2, black, adv);
    x += kw + 3;
    x = gd::text(s, x, kH - kBottomBand + 2, all[i].what, 2, dim, adv) + kHintGap;
  }

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
