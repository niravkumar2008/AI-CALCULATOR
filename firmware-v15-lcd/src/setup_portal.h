// Set the calculator up from a phone, no PC needed (review S3): hotspot name and
// password, the AI proxy address and this calculator's device token.
//
// The calculator makes its own small Wi-Fi network ("AI-Calc-xxxx" with a short
// random password, both shown on the e-paper). A phone that joins it gets the
// setup form as the network's sign-in page (captive portal: every web address
// and every DNS name lead to the form). Saved values go to NVS, the same place
// the Serial Monitor commands write, so both ways can be mixed.
//
// Started from the keypad (= on SETUP > 6:Wi-Fi & key) or with the "setup" serial
// command; ends with AC, after a save, or after 10 minutes. The form needs no
// secret in the link: the network's own password is the secret, and nothing on
// the page shows the camera or the stored token.
#pragma once
#include <string>

#include "settings.h"

struct SetupInfo {
  std::string apName, apPass;
  std::string url;  // "http://192.168.4.1/"
};

// Scans for nearby networks (a few seconds, offered as a list on the form), then
// starts the network, the page and the captive DNS. False = `error` says why.
bool setupStart(const Settings& current, SetupInfo& info, std::string& error);
void setupStop();
bool setupActive();
void setupService();            // call from loop(): DNS, time limit
uint32_t setupSecondsLeft();    // for the e-paper
// The page saved new settings (already validated and written to NVS): take them once.
bool setupTakeSaved(Settings& out);
// Seconds since a save happened (0 = none): main stops the portal shortly after one.
uint32_t setupSavedAgoMs();
