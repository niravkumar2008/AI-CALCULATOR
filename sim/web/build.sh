#!/bin/sh
# Builds calc.js from the shared core, compiled to plain JavaScript (-sWASM=0)
# so the page also runs where WebAssembly is blocked. Needs Emscripten:
#   https://emscripten.org/docs/getting_started/downloads.html
set -e
cd "$(dirname "$0")"
em++ -std=c++17 -O2 -fexceptions web_main.cpp ../../core/*.cpp -o calc.js \
  -sMODULARIZE=1 -sEXPORT_NAME=createCalc -sSINGLE_FILE=1 -sENVIRONMENT=web \
  -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,HEAPU8 \
  -sFILESYSTEM=0 -fexceptions -sWASM=0
[ -f make_page.py ] && python3 make_page.py
echo "built $(pwd)/calc.js and simulator.html"
