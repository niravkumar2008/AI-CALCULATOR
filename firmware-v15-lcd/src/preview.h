// Live camera preview: the calculator makes its own small Wi-Fi network
// ("AI-Calc-xxxx", random password) that a phone or laptop joins, then opens
// the printed link to see what the camera sees, change the tuning and save it.
// Its own network means it works whatever the hotspot allows between devices,
// and needs no internet. The hotspot is dropped while it runs, and used
// briefly for each Send. Off unless started with the "preview" serial
// command; stops after 15 minutes, in exam mode, or with "preview off".
#pragma once
#include <string>

// Starts the preview. True: `message` says how to join and the link (with a
// one-time secret). False: `message` is the error.
bool previewStart(std::string& message);
void previewStop();
bool previewRunning();  // page is up right now
bool previewActive();   // up, or stepped away for a Send and coming back
void previewService();  // call from loop(): enforces the time limit

// Send (scan with Claude from the page). The app polls previewTakeSend(),
// takes the photo, calls previewSuspend() to free the radio for the hotspot,
// solves, then previewSetResult() and previewResume() (same network name,
// password and link, so the laptop can rejoin).
bool previewTakeSend();
void previewSuspend();
bool previewResume();
// replyJson: Claude's reply (the solve schema); or error: a sentence for the user.
void previewSetResult(const std::string& replyJson, const std::string& error);
// Defined by the app: the preview network is gone, rejoin the hotspot.
void previewEnded();
