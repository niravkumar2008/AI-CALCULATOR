@echo off
rem Signs the `claude` command in for the farm (opens your browser). Double-click it from File Explorer,
rem not from inside the Claude app, so the sign-in is saved for normal terminals.
title Sign in Claude for the farm
claude auth login
echo.
claude auth status
echo.
echo If it says "loggedIn": true above, you're done. You can close this window.
pause
