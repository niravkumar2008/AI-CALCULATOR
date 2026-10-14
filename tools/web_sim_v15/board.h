// The simulated v15 board for the public website's "Try it" page: what
// firmware-v15-lcd/src/main.cpp does every loop(), redone around the REAL calculator (core/
// calc::Device) and the REAL screen code (firmware-v15-lcd/src/ui*.cpp, vf_lcd.cpp,
// fbtext.cpp drawing into a LovyanGFX sprite).
//
// Adapted from tools/sim_v15/board.h (the Windows simulator) for a public demo:
//  - no test controls: battery fixed at 80 %, no USB, the "hotspot" always reachable;
//  - the AI is an offline demo: photos rotate three worked problems, Type it works out
//    arithmetic and linear equations in x with the calculator's own maths;
//  - nothing is ever sent anywhere; History and notes are kept by the page (onSaveApps),
//    nothing else is saved;
//  - setup by phone is not offered (a toast says so).
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "device.h"
#include "framebuffer.h"
#include "viewfinder.h"

namespace ui {
struct Status;
}

class Board {
 public:
  Board();
  ~Board();

  int batteryPct = 80;
  std::string linkCode = "DEMO-123";
  // History and notes changed (2 s after the last change, like the board's flash write):
  // the page keeps them (localStorage). May be empty.
  std::function<void(const std::string&)> onSaveApps;

  // Power-up (setup() in main.cpp). `apps`: History / notes the page kept ("" = none).
  void boot(const std::string& apps);
  // One pass of loop().
  void step();

  // A key on the keypad (main.cpp press()).
  void press(calc::DKey k);
  // A character typed on the PC keyboard (as tools/sim_v15). False = no key for it.
  bool typeChar(char c);
  // Type it / notes keyboard: moves the cursor to SEND (SAVE) without pressing it.
  void cursorToSend();
  // The fake camera's picture: 320 x 240 RGB888.
  void setPhoto(const std::vector<uint8_t>& rgb);

  calc::Device& dev() { return *dev_; }
  const calc::Device& dev() const { return *dev_; }

 private:
  struct Request {
    int id = 0;
    bool typed = false;
    std::string text;
    uint32_t t0 = 0;
    int stage = 0;  // 0 photo, 1 waiting for Wi-Fi, 2 asking the AI
    uint32_t stageAt = 0;
    bool partialSent = false;
    std::string json, answer;
    double confidence = 0;
  };

  void queueRequest(int id);
  void serviceRequests(uint32_t now);
  void chooseReply(Request& r);
  bool serviceViewfinder(uint32_t now);
  void stopViewfinder();
  void serviceWifi(uint32_t now);
  void serviceBattery();
  void serviceApps(uint32_t now);
  void serviceScreenSettings(uint32_t now);
  void serviceLink(uint32_t now);
  bool onAiReady() const;
  ui::Status uiStatus() const;
  void kbPressCell(const std::string& label);

  std::unique_ptr<calc::Device> dev_;
  calc::Framebuffer fb_;

  uint32_t lastKeyAt_ = 0;
  bool lightWasOff_ = false;
  bool shiftShown_ = false;
  bool wifiOn_ = false, wifiConnected_ = false;
  uint32_t wifiJoinAt_ = 0, wifiNeededUntil_ = 0;
  std::unique_ptr<Request> req_;
  uint32_t holdUntil_ = 0;
  std::string lastTyped_;
  int demoNext_ = 0;
  calc::Viewfinder vf_;
  bool vfPaused_ = false, wasAiReady_ = false;
  uint32_t nextFrameAt_ = 0, focusFrom_ = 0, refocusAt_ = 0;
  uint32_t fpsAt_ = 0;
  int fpsFrames_ = 0;
  double fps_ = 0;
  std::vector<uint8_t> photo_;  // 320 x 240 RGB888
  uint32_t appsRevSeen_ = 0, appsChangedAt_ = 0;
  int appsSelSeen_ = -1;
  bool appsDirty_ = false;
  int brightSynced_ = -1, sleepSynced_ = -1;
  bool linkWaiting_ = false, wasOnLink_ = false;
  uint32_t linkAt_ = 0;
};
