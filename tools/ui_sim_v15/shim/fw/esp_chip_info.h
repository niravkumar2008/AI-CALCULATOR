#pragma once
#include <cstdint>
struct esp_chip_info_t { int model = 9; uint32_t features = 0; uint16_t revision = 2; uint8_t cores = 2; };
inline void esp_chip_info(esp_chip_info_t* c) { *c = esp_chip_info_t(); }
