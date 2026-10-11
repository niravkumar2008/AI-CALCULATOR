#include "lcd.h"

#include <Arduino.h>
#include <Preferences.h>
#include <driver/gpio.h>

#include "log.h"
#include "pins.h"

namespace lcd {
namespace {

// ---- the panel
// 190-1732TBWPG01 family (LilyGO T-Display-S3 / Heltec HT-VMT190 panel): ST7789V3, 170 x 320
// native portrait, RAM column offset 35. Rotated to 320 x 170 landscape.
class Panel : public lgfx::LGFX_Device {
 public:
  lgfx::Panel_ST7789 panel_;
  lgfx::Bus_SPI bus_;

  Panel() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = int(LCD_SPI_HZ);  // 40 MHz through the GPIO matrix (pins_v15_lcd.h)
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = PIN_LCD_SCK;
      cfg.pin_mosi = PIN_LCD_MOSI;
      cfg.pin_miso = PIN_LCD_MISO;  // -1: SDO is not wired
      cfg.pin_dc = PIN_LCD_DC;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = PIN_LCD_CS;
      cfg.pin_rst = PIN_LCD_RST;
      cfg.pin_busy = -1;
      cfg.panel_width = LCD_WIDTH;    // 170
      cfg.panel_height = LCD_HEIGHT;  // 320
      cfg.memory_width = 240;         // the ST7789's RAM is 240 x 320; the glass shows 170 of it
      cfg.memory_height = 320;
      cfg.offset_x = LCD_COL_OFFSET;  // 35
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = LCD_INVERT != 0;        // IPS panels: inverted gamma (PORT_NOTES.md)
      cfg.rgb_order = LCD_RGB_ORDER != 0;  // swap if red and blue come out exchanged
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;  // the SPI bus is the LCD's alone (the camera is parallel, the keypad I2C)
      panel_.config(cfg);
    }
    setPanel(&panel_);
  }
};

Panel g_panel;
LGFX_Sprite g_canvas(&g_panel);
bool g_ok = false;
bool g_powered = false;
bool g_asleep = false;
uint32_t g_flushMs = 0;

// ---- backlight
constexpr int kBlChannel = 2;     // LEDC channel 2 = timer 1: the camera's XCLK owns timer 0 / channel 0
constexpr int kBlFreqHz = 5000;
constexpr int kBlBits = 8;
constexpr int kDimPercent = 10;   // the "dimmed" level
int g_brightness = 60;            // the user's setting (NVS)
int g_level = 0;                  // what the pin does now
bool g_blReady = false;
IdlePolicy g_idle;
bool g_lightOff = false;

// Perceived brightness is closer to the square of the duty than to the duty itself:
// 50 % on the knob = 25 % duty, which looks about half as bright.
uint32_t dutyFor(int percent) {
  if (percent <= 0) return 0;
  if (percent >= 100) return (1u << kBlBits) - 1;
  const uint32_t d = uint32_t(percent) * uint32_t(percent) * ((1u << kBlBits) - 1) / 10000u;
  return d < 3 ? 3 : d;  // never so low that the LEDs flicker off
}

void applyLevel() {
  if (!g_blReady) {
    ledcSetup(kBlChannel, kBlFreqHz, kBlBits);
    ledcAttachPin(PIN_LCD_BL, kBlChannel);
    g_blReady = true;
  }
  ledcWrite(kBlChannel, g_powered && !g_asleep ? dutyFor(g_level) : 0);
}

void loadNvs() {
  Preferences p;
  p.begin("lcd", true);  // read-only: a missing namespace just means the defaults
  g_brightness = p.getInt("bright", 60);
  g_idle.dimSeconds = p.getInt("dim_s", 20);
  g_idle.offSeconds = p.getInt("off_s", 60);
  g_idle.sleepSeconds = p.getInt("sleep_s", 120);
  p.end();
  if (g_brightness < 10 || g_brightness > 100) g_brightness = 60;
}

void saveNvs() {
  Preferences p;
  p.begin("lcd", false);
  p.putInt("bright", g_brightness);
  p.putInt("dim_s", g_idle.dimSeconds);
  p.putInt("off_s", g_idle.offSeconds);
  p.putInt("sleep_s", g_idle.sleepSeconds);
  p.end();
}

// Every LCD line driven low: the state a powered-down panel must see.
void pinsLow() {
  for (int p : kLcdGpios) {
    gpio_hold_dis(gpio_num_t(p));
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
}

void powerSwitch(bool on) {
  pinMode(PIN_LCD_PWR_N, OUTPUT);
  digitalWrite(PIN_LCD_PWR_N, on ? LOW : HIGH);  // Q4 is a P-FET: low = panel powered
}

}  // namespace

bool begin() {
  static bool loaded = false;
  if (!loaded) {
    loadNvs();
    loaded = true;
  }
  if (g_powered && g_ok) return true;
  // 1. Backlight PWM off (R22 keeps it off before this runs), every LCD line low.
  g_level = 0;
  g_powered = false;
  g_asleep = false;
  applyLevel();  // duty 0
  ledcDetachPin(PIN_LCD_BL);
  g_blReady = false;
  pinsLow();
  // 2. Power the panel, let its supply settle, then the driver's own reset pulse + init.
  powerSwitch(true);
  g_powered = true;
  delay(10);
  g_panel.init();  // pulses RST, SWRESET, SLPOUT, COLMOD 16-bit, MADCTL, INVON, DISPON
  g_panel.setRotation(LCD_ROTATION);
  g_panel.setColorDepth(16);
  g_panel.fillScreen(0);  // black before the light comes on
  if (!g_ok) {
    g_canvas.setPsram(true);  // 320 x 170 x 2 = 109 KB: PSRAM, the internal RAM is for Wi-Fi + camera
    g_canvas.setColorDepth(16);
    g_ok = g_canvas.createSprite(kW, kH) != nullptr;
    if (!g_ok) {
      LOGF("lcd", "no memory for the 320x170 canvas");
    } else {
      g_canvas.fillSprite(0);
    }
  }
  g_lightOff = false;
  LOGF("lcd", "on: %dx%d at %lu MHz, brightness %d %%", kW, kH, (unsigned long)(LCD_SPI_HZ / 1000000),
       g_brightness);
  return g_ok;
}

bool ok() { return g_ok; }
bool powered() { return g_powered; }
bool asleep() { return g_asleep; }

void powerOff() {
  if (!g_powered) return;
  g_level = 0;
  applyLevel();
  ledcDetachPin(PIN_LCD_BL);
  g_blReady = false;
  g_panel.sleep();  // SLPIN + DISPOFF
  delay(5);
  pinsLow();        // nothing drives into the panel once its supply is gone
  powerSwitch(false);
  g_powered = false;
  g_asleep = false;
  g_lightOff = false;
}

void sleepPanel() {
  if (!g_powered || g_asleep) return;
  g_level = 0;
  applyLevel();
  g_panel.sleep();
  g_asleep = true;
}

void wake() {
  if (!g_powered) {
    begin();
    return;
  }
  if (!g_asleep) return;
  g_panel.wakeup();  // SLPOUT: the panel needs ~120 ms before it shows a stable picture
  delay(120);
  g_asleep = false;
}

LGFX_Sprite& canvas() { return g_canvas; }

void flush() {
  if (!g_ok) return;
  wake();
  const uint32_t t0 = millis();
  g_canvas.pushSprite(0, 0);
  g_flushMs = millis() - t0;
}

uint32_t lastFlushMs() { return g_flushMs; }

void pushRaw(int y, const uint16_t* pixels, int h) {
  if (!g_powered) return;
  wake();
  // The data is already in the panel's byte order (swapped), like the canvas.
  g_panel.setSwapBytes(false);
  g_panel.pushImage(0, y, kW, h, pixels);
}

void fill(uint16_t color565) {
  if (!g_powered) return;
  wake();
  g_panel.fillScreen(color565);
}

void setBrightness(int percent, bool save) {
  if (percent < 10) percent = 10;
  if (percent > 100) percent = 100;
  g_brightness = percent;
  if (save) saveNvs();
  setLevel(percent);
}

int brightness() { return g_brightness; }

void setLevel(int percent) {
  if (percent > g_brightness) percent = g_brightness;
  if (percent < 0) percent = 0;
  g_level = percent;
  if (g_powered) applyLevel();
}

int level() { return g_level; }

const IdlePolicy& idlePolicy() { return g_idle; }

void setIdlePolicy(const IdlePolicy& p, bool save) {
  g_idle = p;
  if (save) saveNvs();
}

bool serviceIdle(uint32_t idleMs, bool keepOn) {
  if (!g_powered) return false;
  int want = g_brightness;
  bool off = false, sleepNow = false;
  if (!keepOn) {
    if (g_idle.sleepSeconds > 0 && idleMs >= uint32_t(g_idle.sleepSeconds) * 1000u) sleepNow = off = true;
    else if (g_idle.offSeconds > 0 && idleMs >= uint32_t(g_idle.offSeconds) * 1000u) off = true;
    else if (g_idle.dimSeconds > 0 && idleMs >= uint32_t(g_idle.dimSeconds) * 1000u) want = kDimPercent;
  }
  if (off) want = 0;
  if (sleepNow) sleepPanel();
  else if (g_asleep) wake();
  if (want != g_level) setLevel(want);
  g_lightOff = off;
  return off;
}

bool lightOff() { return g_lightOff; }

}  // namespace lcd
