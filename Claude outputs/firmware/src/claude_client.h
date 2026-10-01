// Sends one photo to Claude over HTTPS and streams the reply back.
// Runs on its own task; results come back through callbacks.
#pragma once
#include <functional>
#include <string>

#include "app.h"

struct SolveCallbacks {
  std::function<bool()> cancelled;  // true once the user pressed AC
  std::function<void(double confidence, const std::string& answer)> partial;
  std::function<void(const std::string& replyJson)> reply;
  std::function<void(calc::Failure f, const std::string& detail)> fail;
};

void claudeSolve(const std::string& apiKey, const std::string& jpeg, const SolveCallbacks& cb);
