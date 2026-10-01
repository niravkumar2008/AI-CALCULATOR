// The OV3660 (or OV5640 / OV2640) camera: one grayscale JPEG per call.
#pragma once
#include <string>

bool cameraBegin();                         // finds the pin layout; false = no camera found
const char* cameraName();                   // which layout/sensor answered
bool cameraCapture(std::string& jpeg, std::string& error);
