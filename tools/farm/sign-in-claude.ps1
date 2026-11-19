# Signs the `claude` command in for the farm. Safe to run from the Claude app's terminal panel:
# it first clears the app's own session settings so the sign-in is saved for normal (farm) use.
Get-ChildItem env: | Where-Object { $_.Name -match '^(CLAUDE_CODE_|CLAUDECODE$|CLAUDE_AGENT_SDK|CLAUDE_PID$|CLAUDE_EFFORT$|CLAUDE_PREVIEW|ANTHROPIC_BASE_URL$)' } |
    ForEach-Object { Remove-Item "env:$($_.Name)" }
Write-Host "Signing in Claude for the farm. A browser opens; sign in, then come back here if it asks for a code." -ForegroundColor Cyan
claude auth login
Write-Host ""
claude auth status
Write-Host ""
Write-Host 'If it says "loggedIn": true above, you are done and the farm starts by itself.' -ForegroundColor Green
