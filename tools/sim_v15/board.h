// The simulated v15 board: what firmware-v15-lcd/src/main.cpp does every loop(), redone
// for the PC around the REAL calculator (core/ calc::Device) and the REAL screen code
// (firmware-v15-lcd/src/ui*.cpp, vf_lcd.cpp, fbtext.cpp drawing into a LovyanGFX sprite).
//
// main.cpp itself can't be compiled on a PC (FreeRTOS tasks, the camera driver, Wi-Fi,
// LittleFS, deep sleep), so its services are mirrored here step by step, with the same
// names and the same rules: press(), the viewfinder, Wi-Fi only while the AI needs it,
// the battery lock-out, exam mode, History / Notes saved 2 s after a change, Settings >
// Brightness / Sleep timer driving the backlight, Settings > Link calculator, setup by
// phone, the backlight's idle policy and "deep sleep" when off.
//
// What is fake: the AI (canned but realistic replies after ~1.5 s, or a chosen failure),
// the camera (frames cut from a picture file), Wi-Fi (a "hotspot reachable" switch), the
// battery (a slider) and flash / RTC memory (files in %APPDATA%\ProoflineSim).
// No windows.h and no LovyanGFX in this header.
#pragma once
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "device.h"
#include "framebuffer.h"
#include "viewfinder.h"

namespace ui {
struct Status;
}

enum class ReplyMode : uint8_t { Normal, Unclear, Unreadable, Timeout, ServerError, Busy };

class Board {
 public:
  Board();
  ~Board();

  // ---- the test controls (the window sets these, then calls the matching *Changed())
  int batteryPct = 78;
  bool usb = false;
  bool hotspot = true;  // the phone's hotspot is on and in range
  bool linked = false;
  std::string linkCode = "K7Q2MX";
  ReplyMode reply = ReplyMode::Normal;
  bool shaky = false;   // camera: a moving hand (the viewfinder's motion check)
  std::filesystem::path dataDir;  // %APPDATA%\ProoflineSim: the board's flash and RTC memory

  // Power-up (setup() in main.cpp): a new calculator, then what flash / RTC memory kept.
  void boot();
  // One pass of loop().
  void step();

  // A key on the keypad (main.cpp press()).
  void press(calc::DKey k);
  // A character typed on the PC keyboard: calc::keysForChar(), except letters and space
  // on the Type it / note keyboard, which move the cursor to that cell and press = (what
  // a user does with the arrows). False = no key for it.
  bool typeChar(char c);
  // Type it / notes keyboard: moves the cursor to SEND (SAVE) without pressing it.
  void cursorToSend();

  void startExam();        // SHIFT 7 ON while off (switches off first if on)
  void teacherUnlock();    // "exam off" over USB
  void powerButton();      // ON when off, SHIFT AC when on
  void coldBoot();         // restart: state and apps come back from the files
  void factoryReset();     // deletes the files, then a cold boot
  void batteryChanged();
  void linkChanged();
  void advance(uint32_t ms);  // the clock jumps (auto-off, exam 12 h, toasts)
  // The fake camera's picture: 320 x 240 RGB888. isDefault: the sample photo (3x + 7 = 22).
  void setPhoto(const std::vector<uint8_t>& rgb, bool isDefault);
  void saveNow();          // state + apps + settings to the files

  calc::Device& dev() { return *dev_; }
  const calc::Device& dev() const { return *dev_; }

  // What the board has been doing, like main.cpp's Serial log ("[wifi] connected").
  const std::deque<std::string>& log() const { return log_; }
  uint32_t logRevision() const { return logRev_; }
  void note(const std::string& line) { logf("%s", line.c_str()); }
  // One line for the panel: Wi-Fi, light, exam clock, sleep.
  std::string statusText() const;
  bool wifiOn() const { return wifiOn_; }
  bool wifiConnected() const { return wifiConnected_; }

 private:
  struct Request {
    int id = 0;
    bool typed = false;
    std::string text;      // Type it: what was typed
    uint32_t t0 = 0;       // queued
    int stage = 0;         // 0 photo, 1 waiting for Wi-Fi, 2 asking the AI
    uint32_t stageAt = 0;
    bool partialSent = false;
    std::string json, answer;  // the canned reply, chosen when it is "sent"
    double confidence = 0;
  };

  void logf(const char* fmt, ...);
  void queueRequest(int id);
  void serviceRequests(uint32_t now);
  void chooseReply(Request& r);
  bool serviceViewfinder(uint32_t now);
  void stopViewfinder();
  void serviceWifi(uint32_t now);
  void serviceBattery();
  void serviceExam();
  void serviceApps(uint32_t now);
  void serviceScreenSettings(uint32_t now);
  void serviceLink(uint32_t now);
  void servicePower(uint32_t now);
  void serviceState(uint32_t now);
  void drawSetupScreen();
  bool onAiReady() const;
  ui::Status uiStatus() const;  // what the status bar shows (main.cpp uiStatus())
  bool charging() const { return usb && batteryPct < 100; }
  void kbPressCell(const std::string& label);

  // files
  void loadFiles();
  void writeFile(const char* name, const std::string& data);
  bool readFile(const char* name, std::string& data) const;

  std::unique_ptr<calc::Device> dev_;
  calc::Framebuffer fb_;
  std::deque<std::string> log_;
  uint32_t logRev_ = 0;
  bool quietKeys_ = false;  // keyboard-cell helpers press many arrows: not logged one by one

  // main.cpp's globals
  uint32_t lastKeyAt_ = 0;
  bool lightWasOff_ = false;
  bool shiftShown_ = false;
  bool aiBatteryOk_ = true;
  bool wifiOn_ = false, wifiConnected_ = false;
  uint32_t wifiJoinAt_ = 0, wifiNeededUntil_ = 0;
  bool wasExam_ = false;
  bool wasOff_ = false, asleep_ = false;
  // requests
  std::unique_ptr<Request> req_;
  uint32_t holdUntil_ = 0;
  std::string lastTyped_;
  int demoNext_ = 0;
  // viewfinder
  calc::Viewfinder vf_;
  bool vfPaused_ = false, wasAiReady_ = false;
  uint32_t camReadyAt_ = 0, nextFrameAt_ = 0, focusFrom_ = 0, refocusAt_ = 0;
  uint32_t fpsAt_ = 0;
  int fpsFrames_ = 0;
  double fps_ = 0;
  std::vector<uint8_t> photo_;  // 320 x 240 RGB888
  bool photoDefault_ = true;
  // setup by phone
  bool setupActive_ = false;
  uint32_t setupUntil_ = 0;
  // apps / settings / state persistence
  uint32_t appsRevSeen_ = 0, appsChangedAt_ = 0;
  int appsSelSeen_ = -1;
  bool appsDirty_ = false;
  int brightSynced_ = -1, sleepSynced_ = -1;
  bool screenDirty_ = false;
  uint32_t screenChangedAt_ = 0;
  uint32_t stateCheckedAt_ = 0;
  std::string stateSaved_;
  // link
  bool linkWaiting_ = false, wasOnLink_ = false;
  uint32_t linkAt_ = 0;
};
