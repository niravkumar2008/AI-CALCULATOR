#include "power.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <soc/usb_serial_jtag_reg.h>
#include <sys/time.h>

#include <cstring>

#include "pins.h"

namespace {
// ADCs vary ~3 % from chip to chip: if the % disagrees with a multimeter on the
// battery, set this to (meter mV) / (status mV).
constexpr float kBatteryCal = 1.00f;
constexpr int kBatterySamples = 8;   // averaged, highest and lowest dropped
constexpr int kDividerRatio = 2;     // 1 M / 1 M: the ADC sees half the cell voltage

// Kept through deep sleep (RTC slow memory; lost when the battery is unplugged,
// like the Casio's memory).
constexpr uint32_t kMagic = 0xA1CA1C13;
struct RtcState {
  uint32_t magic;
  uint32_t len;
  int64_t sleptAtUs;  // gettimeofday() when it went to sleep (the RTC clock keeps running)
  char data[kRtcStateBytes];
};
RTC_DATA_ATTR RtcState g_rtc;
bool g_stateTaken = false;

int64_t nowUs() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return int64_t(tv.tv_sec) * 1000000 + tv.tv_usec;
}

// A pin that must neither drive nor pull anything while the chip sleeps.
void floatPin(int pin) {
  if (pin < 0) return;
  gpio_num_t g = gpio_num_t(pin);
  gpio_hold_dis(g);
  gpio_set_direction(g, GPIO_MODE_DISABLE);
  gpio_set_pull_mode(g, GPIO_FLOATING);
  gpio_pullup_dis(g);
  gpio_pulldown_dis(g);
  if (rtc_gpio_is_valid_gpio(g)) {
    rtc_gpio_isolate(g);  // RTC pads: disconnected, no pulls, held so in deep sleep
  } else {
    gpio_hold_en(g);      // digital pads: keep "disabled, no pull" through deep sleep
  }
}

void holdLevel(int pin, int level) {
  gpio_num_t g = gpio_num_t(pin);
  gpio_hold_dis(g);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
  gpio_hold_en(g);
}
}  // namespace

void powerBegin() {
  // Every pin powerDeepSleep() held or isolated stays frozen until released here.
  auto release = [](int p) {
    gpio_hold_dis(gpio_num_t(p));
    if (rtc_gpio_is_valid_gpio(gpio_num_t(p))) rtc_gpio_deinit(gpio_num_t(p));  // back to the digital GPIO matrix
  };
  for (int p : kCameraGpios) release(p);
  for (int p : {PIN_EPD_CS, PIN_EPD_CLK, PIN_EPD_DIN, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY, PIN_I2C_SDA, PIN_I2C_SCL,
                PIN_KEY_ON, PIN_KEYPAD_INT, PIN_VBAT_SENSE, PIN_CHG_STAT, PIN_CAM_PWR_EN})
    release(p);
  gpio_deep_sleep_hold_dis();

  // External resistors set these levels: no internal pulls (an internal pull-up on
  // CHG_STAT would back-feed the charger through D6 without a cable).
  pinMode(PIN_VBUS_SENSE, INPUT);
  pinMode(PIN_CHG_STAT, INPUT);
  analogSetPinAttenuation(PIN_VBAT_SENSE, ADC_11db);  // half of 4.2 V = 2.1 V, inside the 0-3.1 V range
  // Camera regulators off until a scan needs them (R10 also pulls this low).
  pinMode(PIN_CAM_PWR_EN, OUTPUT);
  digitalWrite(PIN_CAM_PWR_EN, LOW);
}

int batteryMillivolts() {
  int s[kBatterySamples];
  for (int& v : s) v = analogReadMilliVolts(PIN_VBAT_SENSE);
  return int(calc::robustAverage(s, kBatterySamples) * kDividerRatio * kBatteryCal);
}

int batteryPercent() { return calc::lipoPercent(batteryMillivolts()); }

bool usbPresent() { return digitalRead(PIN_VBUS_SENSE) == HIGH; }
bool statPinHigh() { return digitalRead(PIN_CHG_STAT) == HIGH; }
calc::Charge chargeState() { return calc::chargeState(usbPresent(), usbPresent() && statPinHigh()); }

bool batteryOkForAi() { return usbPresent() || batteryMillivolts() >= calc::kAiMinMillivolts; }
bool batteryOkForOta(int minMv) { return usbPresent() || batteryMillivolts() >= minMv; }

void powerLimitWifi() {
  esp_wifi_set_max_tx_power(kWifiMaxTxQuarterDbm);
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);  // radio naps between the hotspot's beacons
}

Wake powerWakeCause() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_UNDEFINED: return Wake::ColdBoot;
    case ESP_SLEEP_WAKEUP_EXT1: {
      const uint64_t pins = esp_sleep_get_ext1_wakeup_status();
      if (pins & (1ULL << PIN_KEY_ON)) return Wake::OnKey;
      if (pins & (1ULL << PIN_KEYPAD_INT)) return Wake::Keypad;
      return Wake::Other;
    }
    default: return Wake::Other;
  }
}

bool powerWokeFromSleep() {
  const Wake w = powerWakeCause();
  return w == Wake::OnKey || w == Wake::Keypad;
}

bool powerTakeState(std::string& state, uint32_t& asleepMs) {
  if (g_stateTaken) return false;
  g_stateTaken = true;
  if (!powerWokeFromSleep() || g_rtc.magic != kMagic || g_rtc.len > kRtcStateBytes) return false;
  state.assign(g_rtc.data, g_rtc.len);
  const int64_t slept = nowUs() - g_rtc.sleptAtUs;
  asleepMs = slept > 0 ? uint32_t(std::min<int64_t>(slept / 1000, 0xFFFFFFFF)) : 0;
  g_rtc.magic = 0;  // used once
  return true;
}

void powerCameraPinsSafe() {
  // Power off first, so nothing is ever driven into an unpowered sensor; PWDN is
  // never driven high (330 uA through R9 into a dead sensor), R9 keeps it low.
  pinMode(PIN_CAM_PWR_EN, OUTPUT);
  digitalWrite(PIN_CAM_PWR_EN, LOW);
  for (int p : kCameraGpios) {  // includes SIOD/SIOC = MTDO/MTCK: no JTAG pulls left on
    gpio_num_t g = gpio_num_t(p);
    gpio_set_direction(g, GPIO_MODE_DISABLE);
    gpio_pullup_dis(g);
    gpio_pulldown_dis(g);
  }
}

[[noreturn]] void powerDeepSleep(const std::string& state, bool keypadClear) {
  // 1. The calculator's state, for the next wake.
  g_rtc.len = uint32_t(std::min(state.size(), kRtcStateBytes));
  memcpy(g_rtc.data, state.data(), g_rtc.len);
  g_rtc.sleptAtUs = nowUs();
  g_rtc.magic = kMagic;

  // 2. Radios and camera off (the caller already stopped Wi-Fi and esp_camera).
  esp_wifi_stop();
  powerCameraPinsSafe();
  for (int p : kCameraGpios) floatPin(p);
  floatPin(PIN_CAM_PWR_EN);  // R10 100 k holds the regulators off (VDD_SPI powers down)

  // 3. E-paper: the panel is already in deep sleep (GxEPD2 hibernate). Hold its
  // inputs at idle levels so they don't float into the controller.
  holdLevel(PIN_EPD_CS, HIGH);
  holdLevel(PIN_EPD_RST, HIGH);
  holdLevel(PIN_EPD_DC, LOW);
  holdLevel(PIN_EPD_CLK, LOW);
  holdLevel(PIN_EPD_DIN, LOW);
  floatPin(PIN_EPD_BUSY);

  // 4. I2C idles high on its own 4.7 k pull-ups; the battery divider is analog.
  floatPin(PIN_I2C_SDA);
  floatPin(PIN_I2C_SCL);
  floatPin(PIN_VBAT_SENSE);
  floatPin(PIN_CHG_STAT);

  // 5. USB: without a cable, switch the USB pad and its D+ pull-up off.
  if (!usbPresent()) {
    CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_DP_PULLUP);
    CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE);
  }

  // 6. Wake on ON, and on any keypad key if the TCA8418's INT is really released
  // (a stuck-low INT would wake the chip at once and also draw 330 uA in R15).
  uint64_t mask = 1ULL << PIN_KEY_ON;
  if (keypadClear) mask |= 1ULL << PIN_KEYPAD_INT;
  for (int p : {PIN_KEY_ON, PIN_KEYPAD_INT}) {
    rtc_gpio_init(gpio_num_t(p));
    rtc_gpio_set_direction(gpio_num_t(p), RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_dis(gpio_num_t(p));    // 100 k (R17) and 10 k (R15) on the board
    rtc_gpio_pulldown_dis(gpio_num_t(p));
  }
  esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);
  gpio_deep_sleep_hold_en();
  esp_deep_sleep_start();
}

const char* resetReasonText() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_EXT: return "reset pin";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "crash";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep-sleep wake";
    case ESP_RST_BROWNOUT: return "brownout";
    default: return "other";
  }
}
