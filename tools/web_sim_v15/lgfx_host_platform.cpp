// The platform functions LovyanGFX's desktop build expects, for the browser build.
// A copy of tools/ui_sim_v15/lgfx_host_platform.cpp without std::thread (no pthreads in
// the WebAssembly build): we only draw into an in-memory LGFX_Sprite, so time is the
// browser clock, delays never block, and every bus is a no-op.
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <emscripten/emscripten.h>

namespace lgfx {
inline namespace v1 {

static uint8_t g_gpio[EMULATED_GPIO_MAX];
void pinMode(int_fast16_t, pin_mode_t) {}
void lgfxPinMode(int_fast16_t, pin_mode_t) {}
void gpio_hi(uint32_t pin) { g_gpio[pin & (EMULATED_GPIO_MAX - 1)] = 1; }
void gpio_lo(uint32_t pin) { g_gpio[pin & (EMULATED_GPIO_MAX - 1)] = 0; }
bool gpio_in(uint32_t pin) { return g_gpio[pin & (EMULATED_GPIO_MAX - 1)]; }

unsigned long millis(void) { return (unsigned long)emscripten_get_now(); }
unsigned long micros(void) { return (unsigned long)(emscripten_get_now() * 1000.0); }
void delay(unsigned long) {}
void delayMicroseconds(unsigned int) {}

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
