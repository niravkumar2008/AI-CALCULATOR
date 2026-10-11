// The OV3660 (or OV5640 / OV2640) camera: one grayscale JPEG per call.
// Tuning (brightness, contrast, ...) can be changed live from the preview
// page and saved; saved tuning is applied at every start.
#pragma once
#include <functional>
#include <string>
#include <vector>

bool cameraBegin();                         // finds the pin layout; false = no camera found
const char* cameraName();                   // which layout/sensor answered
// One scan photo: waits for the shake of the key press to settle, takes 4
// frames, keeps the one whose writing is sharpest (see focus.h), and makes a
// full-resolution close-up of the writing when it covers only part of the
// frame (`detail`, empty otherwise). Both are kept for the preview page.
// `grabbed` (optional) runs as soon as the frames are taken, before the
// analysis: the user can stop holding still then.
bool cameraCapture(std::string& jpeg, std::string& detail, std::string& error,
                   const std::function<void()>& grabbed = nullptr);
// While a capture is waiting for stillness: millis() when it expects to have
// its frames (for a countdown). 0 when no capture is holding.
uint32_t cameraHoldUntil();
// Powers the camera down (calculator off). The next capture or preview frame
// starts it again, about 1 s with autofocus.
void cameraSleep();
bool cameraIsOn();
// Viewfinder: one small grayscale frame (160x120) of the whole 4:3 picture the scan
// will send, plus whether the autofocus reports "focused". Powers the camera up if it
// is off. False if there is no camera or no frame.
bool cameraPreviewFrame(std::vector<uint8_t>& gray, int& w, int& h, bool& focused);
// Factory self-test: powers the camera up if needed and grabs one VGA JPEG.
// sensor = "Final board (OV5640 AF), OV5640" or "not found".
bool cameraSelfTest(std::string& sensor, size_t& bytes, uint32_t& ms, bool& autofocus);
// How long the next capture will ask the user to hold still, in ms.
uint32_t cameraHoldEstimateMs();
// Sharpness of the writing in one JPEG (0 when it can't be decoded).
double cameraFocusScore(const std::string& jpeg);

// Live preview support. All are safe to call from any task.
bool cameraFrame(std::string& jpeg);        // one frame, no warm-up (for the stream)
bool cameraLastPhoto(std::string& jpeg);    // the last photo taken for a scan
bool cameraLastDetail(std::string& jpeg);   // its close-up (false if none was made)
// Sets one tuning value by name (see kTuning in camera.cpp). False = unknown
// name or out of range; nothing changes then.
bool cameraSet(const std::string& name, int value);
std::string cameraTuningJson();             // {"brightness":1,...} current values
// Tries exposure, gain and contrast on what the camera sees now and keeps the
// best for reading (about 5 s). Returns a report. Scans do this first when
// the "careful" setting is 1. Not saved until cameraSaveTuning().
std::string cameraAutoTune();
void cameraSaveTuning();                    // keep the current values after restart
void cameraResetTuning();                   // back to the defaults (and saved)
