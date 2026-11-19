@echo off
rem Starts the Proofline Farm manager in this window. Close the window or press Ctrl+C to stop it
rem (Ctrl+C once = let the current farmhands finish, twice = stop now).
title Proofline Farm
cd /d "%~dp0"
node manager.cjs run
pause
