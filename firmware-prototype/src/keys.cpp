#include "keys.h"

#include <Arduino.h>

#include "log.h"
#include "pins.h"

using calc::DKey;

namespace {

// Holding a scrolling key repeats it: first after kRepeatDelayMs, then every
// kRepeatEveryMs. The e-paper's ~0.3 s refresh is slower than that, so the
// presses due are sent together and the screen skips to where it should be.
constexpr uint32_t kRepeatDelayMs = 400;
constexpr uint32_t kRepeatEveryMs = 120;

struct Repeat {
  DKey key = DKey::Count;  // Count = nothing held
  uint32_t next = 0;
  void start(DKey k) {
    key = k;
    next = millis() + kRepeatDelayMs;
  }
  void stop() { key = DKey::Count; }
  void poll(void (*press)(DKey)) {
    const uint32_t now = millis();
    for (int n = 0; key != DKey::Count && int32_t(now - next) >= 0 && n < 4; ++n) {
      next += kRepeatEveryMs;
      press(key);
    }
    if (key != DKey::Count && int32_t(now - next) >= 0) next = now + kRepeatEveryMs;  // fell behind: don't catch up later
  }
};

// ON: counts on release after being held 50 ms, so a glitch never powers
// the calculator on or off.
volatile uint32_t g_onDownMs = 0;
volatile bool g_onPressed = false;
void IRAM_ATTR onOnChange() {
  const uint32_t now = millis();
  if (digitalRead(PIN_KEY_ON) == LOW) {
    g_onDownMs = now;
  } else if (g_onDownMs != 0) {
    if (now - g_onDownMs >= 50) g_onPressed = true;
    g_onDownMs = 0;
  }
}

void beginOn() {
  pinMode(PIN_KEY_ON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_KEY_ON), onOnChange, CHANGE);
}

void pollOn(void (*press)(DKey)) {
  if (!g_onPressed) return;
  g_onPressed = false;
  press(DKey::On);
}

}  // namespace

// ---------------------------------------------------------------- prototype
#include <Wire.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

namespace {
constexpr uint8_t kTca = TCA8418_I2C_ADDR;  // 0x34
// Registers (TI TCA8418 datasheet)
constexpr uint8_t kCfg = 0x01, kIntStat = 0x02, kKeyLckEc = 0x03, kKeyEventA = 0x04;
constexpr uint8_t kKpGpio1 = 0x1D, kKpGpio2 = 0x1E, kKpGpio3 = 0x1F;
constexpr int kRows = 8, kCols = 10;
constexpr DKey X = DKey::Count;  // no key at this crossing

// Which key sits at each matrix crossing [ROWr][COLc], copied from the
// board's schematic (hardware/kicad/keypad.kicad_sch, SW1-SW49). Every key
// but ON (its own pin).
const DKey kMatrix[kRows][kCols] = {
    {DKey::Shift, DKey::Alpha, X, X, DKey::Mode, X, DKey::Up, DKey::Down, DKey::Left, DKey::Right},
    {DKey::Frac, DKey::Sqrt, DKey::Sq, DKey::Pow, DKey::Log, DKey::Ln, DKey::Calc, DKey::Integral, DKey::Inv, DKey::LogAB},
    {DKey::Neg, DKey::Dms, DKey::Hyp, DKey::Sin, DKey::Cos, DKey::Tan, X, X, X, X},
    {DKey::Rcl, DKey::Eng, DKey::Open, DKey::Close, DKey::SD, DKey::MPlus, X, X, X, X},
    {DKey::D7, DKey::D8, DKey::D9, DKey::Del, DKey::AC, X, X, X, X, X},
    {DKey::D4, DKey::D5, DKey::D6, DKey::Mul, DKey::Div, X, X, X, X, X},
    {DKey::D1, DKey::D2, DKey::D3, DKey::Add, DKey::Sub, X, X, X, X, X},
    {DKey::D0, DKey::Dot, DKey::Exp10, DKey::Ans, DKey::Eq, X, X, X, X, X},
};

bool g_tcaOk = false;
Repeat g_repeat;
bool g_down[int(DKey::Count)] = {};  // keys held right now (from press / release events)

bool tcaWrite(uint8_t reg, uint8_t v) {
  Wire.beginTransmission(kTca);
  Wire.write(reg);
  Wire.write(v);
  return Wire.endTransmission() == 0;
}

uint8_t tcaRead(uint8_t reg) {
  Wire.beginTransmission(kTca);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(kTca, uint8_t(1)) != 1) return 0;
  return Wire.read();
}

bool repeats(DKey k) {
  return k == DKey::Up || k == DKey::Down || k == DKey::Left || k == DKey::Right || k == DKey::Del;
}
}  // namespace

void keysBegin(bool keepEvents) {
  beginOn();
  pinMode(PIN_KEYPAD_INT, INPUT);  // open drain with R15 10 k on the board: no internal pull needed
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  // Rows 0-7 and columns 0-9 scan the keypad; the scanner debounces and
  // queues up to 10 events on its own, with INT low while any wait. Its RESET
  // pin isn't wired to the ESP32, so every register is written each start.
  g_tcaOk = tcaWrite(kKpGpio1, 0xFF) && tcaWrite(kKpGpio2, 0xFF) && tcaWrite(kKpGpio3, 0x03) &&
            tcaWrite(kCfg, 0x01);  // KE_IEN: key events raise INT
  // After a deep-sleep wake the key that woke the chip is still in the FIFO: keep it.
  if (!keepEvents) {
    for (int i = 0; i < 10 && (tcaRead(kKeyLckEc) & 0x0F); ++i) tcaRead(kKeyEventA);  // drop stale events
    tcaWrite(kIntStat, 0x1F);
  }
  if (!g_tcaOk) LOGF("keys", "keypad scanner (TCA8418) not found: check its I2C wiring");
}

void keysWokeByOn() {
  // The ON press that woke the chip started before the firmware ran: time it from
  // now, or count it at once if it was already released during the boot.
  if (digitalRead(PIN_KEY_ON) == LOW) g_onDownMs = millis() - 50;
  else g_onPressed = true;
}

bool keysScannerOk() { return g_tcaOk; }

bool keysHeld(DKey k) {
  if (k == DKey::On) return digitalRead(PIN_KEY_ON) == LOW;
  return k < DKey::Count && g_down[int(k)];
}

bool keysPrepareSleep() {
  g_repeat.stop();
  if (!g_tcaOk) return false;
  // Drain the FIFO and clear every interrupt flag: a waiting event keeps INT low,
  // which would wake the chip at once and draw 330 uA through R15 (review S6).
  for (int tries = 0; tries < 3; ++tries) {
    for (int i = 0; i < 12 && (tcaRead(kKeyLckEc) & 0x0F); ++i) tcaRead(kKeyEventA);
    tcaWrite(kIntStat, 0x1F);
    delayMicroseconds(200);
    if (digitalRead(PIN_KEYPAD_INT) == HIGH) return true;
    delay(20);  // a key still bouncing: try again
  }
  LOGF("keys", "keypad INT stays low: only ON will wake the calculator");
  return false;
}

void keysPoll(void (*press)(DKey)) {
  pollOn(press);
  if (g_tcaOk && digitalRead(PIN_KEYPAD_INT) == LOW) {
    for (int n = tcaRead(kKeyLckEc) & 0x0F; n > 0; --n) {
      const uint8_t ev = tcaRead(kKeyEventA);
      const int code = (ev & 0x7F) - 1;  // key number 1..80 = row * 10 + column + 1
      const int row = code / 10, col = code % 10;
      if (code < 0 || row >= kRows || col >= kCols) continue;
      const DKey k = kMatrix[row][col];
      if (k == X) continue;
      g_down[int(k)] = (ev & 0x80) != 0;
      if (ev & 0x80) {  // pressed
        press(k);
        if (repeats(k)) g_repeat.start(k);
      } else if (k == g_repeat.key) {
        g_repeat.stop();
      }
    }
    tcaWrite(kIntStat, 0x01);  // K_INT handled
    if (g_tcaOk && digitalRead(PIN_KEYPAD_INT) == LOW && !(tcaRead(kKeyLckEc) & 0x0F))
      tcaWrite(kIntStat, 0x1F);  // other flags (overflow, GPI) also hold INT low
  }
  g_repeat.poll(press);
}
