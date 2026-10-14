// Pictures in and out with WIC (Windows Imaging Component, part of Windows): the fake
// camera's picture from any JPG / PNG / BMP, and screenshots saved as PNG.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Loads a picture, scales it to cover w x h (keeping its shape, the middle cut out, as a
// camera frames it) and returns RGB888, w * h * 3 bytes. False if Windows can't read it.
bool wicLoadCover(const std::wstring& path, int w, int h, std::vector<uint8_t>& rgb);
// Writes 32-bit BGRX pixels (top-down, w * h) as a PNG file.
bool wicSavePng(const std::wstring& path, const uint32_t* bgrx, int w, int h);
