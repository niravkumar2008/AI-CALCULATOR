// Hotspot name/password and Claude API key, kept in the chip's flash (NVS).
// Set them over USB: open PlatformIO's Serial Monitor and type "help".
#pragma once
#include <string>

struct Settings {
  std::string ssid, password, apiKey;
};

enum class SerialCmd { None, Changed, Snap, Keys, ExamOff, Scan, Preview, PreviewOff };

Settings settingsLoad();
// Reads serial input without blocking; handles the setup commands.
// statusText() is printed by "status". For Keys, `arg` holds the characters
// to press (see keyForChar in main.cpp).
SerialCmd settingsPoll(Settings& s, std::string (*statusText)(), std::string& arg);

// Exam mode survives a restart (pulling the battery doesn't end it).
void examSave(bool active, uint32_t elapsedMs);
bool examLoad(uint32_t& elapsedMs);
