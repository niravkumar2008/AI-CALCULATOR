// The structured reply Claude returns for one photo.
#pragma once
#include <string>
#include <vector>

namespace calc {

struct SolveResult {
  bool readable = true;             // false: no problem found in the photo
  std::string readAs;               // the question as Claude understood it
  double confidence = 1.0;          // 0..1, how sure Claude is of its reading
  std::vector<std::string> unclear; // unclear symbols / cut-off regions
  std::string expression;           // the calculation in calculator notation ("" if none)
  std::string choice;               // multiple choice: the correct option's label ("C"), else ""
  std::string answer;               // final answer, with units
  std::vector<std::string> steps;   // solution steps, in order
};

// Parses Claude's JSON reply (see README "Reply format"). Missing optional
// fields get defaults; a readable reply without an answer is rejected.
bool parseSolveResult(const std::string& json, SolveResult& out, std::string& err);

}  // namespace calc
