// The full-size fuzz campaign: the code of tests/fuzz_tests.cpp, run with millions of
// steps, one seed at a time (each prints its own line). run_long.bat builds and runs it.
//   fuzz_long.exe [--seeds N] [--first S] [--sessions N] [--steps N] [--persist N]
//                 [--png N] [--modules N] [--modes N] [--engine N] [--verbose]
// --seeds N runs seeds S, S+1, ... (S = --first, default 1); the other counts are per seed.
// Exit code: 0 clean, 1 failures (each printed once with the actions before it).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../../tests/fuzz_tests.h"

int main(int argc, char** argv) {
  std::setvbuf(stdout, nullptr, _IONBF, 0);
  int seeds = 8;
  uint64_t first = 1;
  fuzz::Options o;
  o.deviceSessions = 300;   // x 1000 steps x 8 seeds = 2.4 M device steps
  o.deviceSteps = 1000;
  o.persistIterations = 60;  // x 40 mutants x 8 seeds = 19 200 mutants
  o.pngImages = 60;
  o.moduleSteps = 150000;
  o.modeSteps = 100000;
  o.engineLists = 40000;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    const bool more = i + 1 < argc;
    auto num = [&]() { return more ? std::strtoll(argv[++i], nullptr, 10) : 0; };
    if (a == "--seeds") seeds = static_cast<int>(num());
    else if (a == "--first") first = static_cast<uint64_t>(num());
    else if (a == "--sessions") o.deviceSessions = static_cast<int>(num());
    else if (a == "--steps") o.deviceSteps = static_cast<int>(num());
    else if (a == "--persist") o.persistIterations = static_cast<int>(num());
    else if (a == "--png") o.pngImages = static_cast<int>(num());
    else if (a == "--modules") o.moduleSteps = static_cast<int>(num());
    else if (a == "--modes") o.modeSteps = static_cast<int>(num());
    else if (a == "--engine") o.engineLists = static_cast<int>(num());
    else if (a == "--verbose") o.verbose = true;
    else {
      std::fprintf(stderr, "unknown option %s (see the top of tools/fuzz/fuzz_long.cpp)\n", a.c_str());
      return 2;
    }
  }
  std::printf("fuzz_long: %d seeds from %llu; per seed %d sessions x %d steps, %d x 40 mutants, %d PNGs, "
              "%d module keys, %d STAT / TABLE / CALC / SOLVE keys, %d token lists\n",
              seeds, static_cast<unsigned long long>(first), o.deviceSessions, o.deviceSteps, o.persistIterations,
              o.pngImages, o.moduleSteps, o.modeSteps, o.engineLists);
  const auto t0 = std::chrono::steady_clock::now();
  int fails = 0;
  for (int s = 0; s < seeds; ++s) {
    // Spread the seed's bits (seeds 1, 2, 3 ... would start the xorshift almost alike).
    uint64_t z = (first + static_cast<uint64_t>(s)) * 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    o.seeds = {(z ^ (z >> 31)) | 1};
    std::printf("seed %llu: ", static_cast<unsigned long long>(first + static_cast<uint64_t>(s)));
    fails += fuzz::run(o);
    const double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("  (%.0f s so far)\n", sec);
  }
  std::printf("fuzz_long: %s (%d failed checks)\n", fails ? "FAILURES" : "clean", fails);
  return fails ? 1 : 0;
}
