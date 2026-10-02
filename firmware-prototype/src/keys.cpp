#include "keys.h"

#include <Arduino.h>

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
constexpr uint8_t kTca = 0x34;  // TCA8418 I2C address
// Registers (TI TCA8418 datasheet)
constexpr uint8_t kCfg = 0x01, kIntStat = 0x02, kKeyLckEc = 0x03, kKeyEventA = 0x04;
constexpr uint8_t kKpGpio1 = 0x1D, kKpGpio2 = 0x1E, kKpGpio3 = 0x1F;
constexpr int kRows = 7, kCols = 7;

// Which key sits at each matrix position: row r, column c = index r * 7 + c.
// Every key except ON (its own pin), in keypad order. The board's schematic
// wires the Casio pads to match this table.
const DKey kMatrix[kRows * kCols] = {
    DKey::Shift, DKey::Alpha, DKey::Up,   DKey::Down, DKey::Left,  DKey::Right, DKey::Mode,
    DKey::Abs,   DKey::Cube,  DKey::Inv,  DKey::LogAB, DKey::Frac, DKey::Sqrt,  DKey::Sq,
    DKey::Pow,   DKey::Log,   DKey::Ln,   DKey::Neg,  DKey::Dms,   DKey::Hyp,   DKey::Sin,
    DKey::Cos,   DKey::Tan,   DKey::Rcl,  DKey::Eng,  DKey::Open,  DKey::Close, DKey::SD,
    DKey::MPlus, DKey::D7,    DKey::D8,   DKey::D9,   DKey::Del,   DKey::AC,    DKey::D4,
    DKey::D5,    DKey::D6,    DKey::Mul,  DKey::Div,  DKey::D1,    DKey::D2,    DKey::D3,
    DKey::Add,   DKey::Sub,   DKey::D0,   DKey::Dot,  DKey::Exp10, DKey::Ans,   DKey::Eq,
};
static_assert(int(DKey::Count) - 1 == kRows * kCols, "every key but ON has a matrix spot");

bool g_tcaOk = false;
Repeat g_repeat;

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

void keysBegin() {
  beginOn();
  pinMode(PIN_KEYPAD_INT, INPUT_PULLUP);  // the TCA8418's INT is open-drain
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  // Rows 0-6 and columns 0-6 scan the keypad; the scanner debounces and
  // queues up to 10 events on its own, with INT low while any wait.
  g_tcaOk = tcaWrite(kKpGpio1, 0x7F) && tcaWrite(kKpGpio2, 0x7F) && tcaWrite(kKpGpio3, 0x00) &&
            tcaWrite(kCfg, 0x01);  // KE_IEN: key events raise INT
  for (int i = 0; i < 10 && (tcaRead(kKeyLckEc) & 0x0F); ++i) tcaRead(kKeyEventA);  // drop stale events
  tcaWrite(kIntStat, 0x1F);
  if (!g_tcaOk) Serial.println("Keypad scanner (TCA8418) not found: check its I2C wiring.");
}

void keysPoll(void (*press)(DKey)) {
  pollOn(press);
  if (g_tcaOk && digitalRead(PIN_KEYPAD_INT) == LOW) {
    for (int n = tcaRead(kKeyLckEc) & 0x0F; n > 0; --n) {
      const uint8_t ev = tcaRead(kKeyEventA);
      const int code = (ev & 0x7F) - 1;  // key number 1..80 = row * 10 + column + 1
      const int row = code / 10, col = code % 10;
      if (code < 0 || row >= kRows || col >= kCols) continue;
      const DKey k = kMatrix[row * kCols + col];
      if (ev & 0x80) {  // pressed
        press(k);
        if (repeats(k)) g_repeat.start(k);
      } else if (k == g_repeat.key) {
        g_repeat.stop();
      }
    }
    tcaWrite(kIntStat, 0x01);  // K_INT handled
  }
  g_repeat.poll(press);
}

void keysSleepUntilPress() {
  gpio_wakeup_enable(gpio_num_t(PIN_KEY_ON), GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable(gpio_num_t(PIN_KEYPAD_INT), GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  Serial.flush();
  esp_light_sleep_start();  // RAM, the calculator and the clock carry on afterwards
  // The press that woke us happened while asleep, so its interrupt was
  // missed: start timing it now, and its release counts as usual.
  if (digitalRead(PIN_KEY_ON) == LOW) g_onDownMs = millis() - 50;
}

