# Fuzz tests

A fuzz campaign (2026-10-11) drove `core/` with random input and found eleven bugs
(B-1 ... B-11, fixed; see `firmware-v15-lcd/PORT_NOTES.md`). Its harnesses now live in the
repo as one piece of code, `tests/fuzz_tests.cpp`, run two ways.

## 1. Every test run (bounded, a few seconds)

`tests\msvc\build_and_run.bat` (and the CMake `tests` target) runs `runFuzzTests()` with
fixed seeds, so every run does exactly the same thing:

| part    | what | size |
|---------|------|------|
| device  | random keys, time jumps, AI replies / failures / partial answers (good, broken, huge), `startScan()`, exam mode, persistence round trips, platform setters, on a `Device` (70 % of sessions with Type it, 30 % without). Every invariant checked after every step: exam mode never shows an app or sends a request, no "AI" mode in an app view, list selections in range, a NoteView always has its note, toasts fit and vanish, results never show nan/inf or overflow the screen, saved replies within their caps, typed PNGs only after SEND / resend and only where supported. | 4 seeds x 50 sessions x 1000 steps = 200 000 steps |
| persist | `saveApps()` / `saveState()` of populated devices, mutated (bit flips, truncation, garbage, edge bytes; most re-sealed with a valid checksum to reach the parser): refused without changing anything, or accepted and then browsed, rendered and calculated with | 50 x 40 = 2 000 mutants |
| png     | `encodePngGray()` (edge sizes up to 65535 px and random ones) and `renderTextPng()` decoded by an independent decoder in the test (chunk CRCs, zlib header, stored blocks, Adler-32, filter bytes) and compared pixel for pixel | ~70 images |
| modules | Geography and Periodic Table on their own: random keys with SHIFT / ALPHA, every quiz filter | 50 000 keys |
| engine  | random token lists: an error, or a finite value whose exact fraction agrees with it | 20 000 lists |

It prints one line (`fuzz: ... 0 failed`); each distinct failure is printed once with the
last 60 actions before it. Change the sizes in `tests/fuzz_tests.h` (`fuzz::Options`), not
in the code.

## 2. The long run (occasional, minutes)

```
tools\fuzz\run_long.bat                   debug build: /RTC1 /MDd /D_ITERATOR_DEBUG_LEVEL=2
tools\fuzz\run_long.bat release           /O2, ~10x faster
tools\fuzz\run_long.bat release --seeds 2 --first 100 --sessions 50 --verbose
```

`tools/fuzz/fuzz_long.cpp` runs the same code per seed (8 seeds by default: 2.4 M device
steps, 19 200 persistence mutants, 480 PNGs, 1.2 M module keys, 320 000 token lists) and
prints a line per seed. Options: `--seeds N --first S --sessions N --steps N --persist N
--png N --modules N --engine N --verbose` (counts are per seed; `--verbose` keeps the last
400 actions for a failure). The debug build catches out-of-range iterators and uninitialised
reads the release build would not; it takes roughly 20-40 minutes. Build output goes to
`tools\fuzz\build\` (ignored by git). Exit code 0 = clean.

A failure prints its seed line: rerun with `--first <seed> --seeds 1 --verbose` to replay
it exactly, then turn it into a deterministic test in `tests/tests.cpp`
(`testFuzzFindings()` holds the ones found so far; `tests/fuzz/crash_state_llong_min.bin`
is the saved state that crashed B-1).
