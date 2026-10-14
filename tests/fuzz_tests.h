// The fuzz campaign as code (tests/fuzz_tests.cpp). tests.cpp runs it small through
// runFuzzTests(); tools/fuzz/fuzz_long.cpp runs it big (tools/fuzz/README.md).
#pragma once
#include <cstdint>
#include <vector>

namespace fuzz {

struct Options {
  // Device fuzzer: for each seed, `deviceSessions` fresh Devices of `deviceSteps` actions.
  std::vector<uint64_t> seeds = {88172645463325252ull, 0x2545F4914F6CDD1Dull, 0xD1B54A32D192ED03ull, 1234567ull};
  int deviceSessions = 50;
  int deviceSteps = 1000;
  int persistIterations = 50;  // x 40 mutants (20 saveApps(), 20 saveState())
  int pngImages = 50;          // random images, plus the edge sizes and the Type it texts
  int moduleSteps = 50000;     // random keys, each to Geography and to the Periodic Table
  int modeSteps = 30000;       // random keys to STAT, TABLE and the CALC / SOLVE runners on their own
  int engineLists = 20000;     // random token lists for the calculator engine
  bool verbose = false;
};

// Returns the number of failed checks (each distinct failure is printed once, with the
// actions before it).
int run(const Options& o);

}  // namespace fuzz
