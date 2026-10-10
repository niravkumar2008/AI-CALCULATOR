#!/bin/sh
# run a pcbnew python script, hiding KiCad/SWIG noise
python3 -u "$@" 2>&1 | grep --line-buffered -v -E "assert|property.h|swig/python detected"
