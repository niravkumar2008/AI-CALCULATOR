#pragma once
#include <cstddef>
#include <cstdint>
typedef struct { uint8_t* buf; size_t len; size_t width; size_t height; int format; } camera_fb_t;
