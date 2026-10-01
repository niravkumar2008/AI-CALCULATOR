// Word wrapping for the 25-column screen.
#pragma once
#include <string>
#include <vector>

namespace calc {

// Wraps UTF-8 text to at most `width` characters per line. Breaks at spaces
// where possible, hard-breaks words longer than a line, and honours '\n'.
// Never returns an empty vector (empty input gives one empty line).
std::vector<std::string> wrapText(const std::string& utf8, int width);

// Number of characters (code points) in a UTF-8 string.
int textLength(const std::string& utf8);

}  // namespace calc
