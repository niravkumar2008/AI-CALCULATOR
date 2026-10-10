// Host build of LovyanGFX for the UI screenshot harness (tools/ui_sim_v15).
// NOT the real SDL: LovyanGFX picks its desktop ("SDL") platform when it finds this
// header. We only need its in-memory LGFX_Sprite, never a window, so this file declares
// just the type names Panel_sdl.hpp mentions. Panel_sdl.inl is never compiled; the few
// platform functions (millis, delay, gpio, spi/i2c) are in ../../lgfx_host_platform.cpp.
#pragma once
#define SDL_h_
#include <stdint.h>
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_mutex SDL_mutex;
typedef enum { KMOD_NONE = 0 } SDL_Keymod;
typedef enum { SDLK_UNKNOWN = 0 } SDL_KeyCode;
