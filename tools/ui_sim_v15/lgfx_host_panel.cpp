// The one panel implementation LGFXBase links against (Panel_Device, the base of every
// display). LovyanGFX's own lgfx_v1_panel.cpp also builds every panel driver, one of
// which MSVC rejects (a VLA); none of them is used to draw a sprite.
#define LGFX_USE_V1
#define LGFX_V1_IMPLEMENTATION
#include <LovyanGFX.hpp>
#include <lgfx/v1/panel/Panel_Device.inl>
#undef LGFX_V1_IMPLEMENTATION
