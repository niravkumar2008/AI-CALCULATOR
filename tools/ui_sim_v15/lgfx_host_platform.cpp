// The platform functions LovyanGFX's desktop build expects (normally in
// platforms/sdl/common.inl, which needs the real SDL). The harness only draws into
// in-memory sprites, so time is std::chrono and every bus is a no-op.
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <chrono>
#include <thread>

namespace lgfx {
inline namespace v1 {

static uint8_t g_gpio[EMULATED_GPIO_MAX];
void pinMode(int_fast16_t, pin_mode_t) {}
void lgfxPinMode(int_fast16_t, pin_mode_t) {}
void gpio_hi(uint32_t pin) { g_gpio[pin & (EMULATED_GPIO_MAX - 1)] = 1; }
void gpio_lo(uint32_t pin) { g_gpio[pin & (EMULATED_GPIO_MAX - 1)] = 0; }
bool gpio_in(uint32_t pin) { return g_gpio[pin & (EMULATED_GPIO_MAX - 1)]; }

static const auto g_t0 = std::chrono::steady_clock::now();
unsigned long millis(void) {
  return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_t0).count();
}
unsigned long micros(void) {
  return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - g_t0).count();
}
void delay(unsigned long ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
void delayMicroseconds(unsigned int us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }

namespace spi {
cpp::result<void, error_t> init(int, int, int, int) { return cpp::fail(error_t::unknown_err); }
void release(int) {}
void beginTransaction(int, uint32_t, int) {}
void endTransaction(int) {}
void writeBytes(int, const uint8_t*, size_t) {}
void readBytes(int, uint8_t*, size_t) {}
}  // namespace spi

namespace i2c {
cpp::result<void, error_t> init(int, int, int) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> release(int) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> restart(int, int, uint32_t, bool) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> beginTransaction(int, int, uint32_t, bool) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> endTransaction(int) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> writeBytes(int, const uint8_t*, size_t) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> readBytes(int, uint8_t*, size_t, bool) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> transactionWrite(int, int, const uint8_t*, uint8_t, uint32_t) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> transactionRead(int, int, uint8_t*, uint8_t, uint32_t) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> transactionWriteRead(int, int, const uint8_t*, uint8_t, uint8_t*, size_t, uint32_t) {
  return cpp::fail(error_t::unknown_err);
}
cpp::result<uint8_t, error_t> readRegister8(int, int, uint8_t, uint32_t) { return cpp::fail(error_t::unknown_err); }
cpp::result<void, error_t> writeRegister8(int, int, uint8_t, uint8_t, uint8_t, uint32_t) { return cpp::fail(error_t::unknown_err); }
}  // namespace i2c

}  // namespace v1
}  // namespace lgfx
