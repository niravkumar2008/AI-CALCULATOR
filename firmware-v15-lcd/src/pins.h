// Every pin of the v15-LCD board lives in ONE file: hardware/kicad_v15_lcd/pins_v15_lcd.h
// (checked against the v15 schematic's netlist). This header only includes it, so the
// firmware can never drift from the board. Don't add pins here: change that file instead.
//
// What differs from the v14 board (hardware/pins_final.h): the e-paper GPIOs became the
// LCD's (IO5 RST, IO6 CS, IO8 SCK, IO41 D/C, IO42 MOSI, IO33 LCD_PWR_N, IO4 backlight PWM)
// and KEYPAD_INT moved from IO4 to IO3. The camera, keypad I2C, power and USB pins are the same.
#pragma once
#include <stdint.h>

#ifndef BOARD_V15_LCD
#error "This firmware is for the v15-LCD board only: build it with -DBOARD_V15_LCD (platformio.ini)"
#endif

#include "../../hardware/kicad_v15_lcd/pins_v15_lcd.h"
