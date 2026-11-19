<#
  Installs the Proofline Wi-Fi watchdog as a Windows Scheduled Task (run once, as administrator).

  - Copies wifi-watchdog.ps1 to <FarmDir>\bin, so the task keeps working even if this worktree is deleted.
  - Creates the task "Proofline Wi-Fi Watchdog": every 2 minutes and at start-up, runs as SYSTEM
    (highest privileges, no password needed, works when you're logged out), no window.

  Remove it again:  install-wifi-watchdog.ps1 -Uninstall
#>
param(
    [string]$FarmDir = 'C:\Users\r_kas\ProoflineFarm',
    [string]$Adapter = 'Wi-Fi',
    [switch]$Uninstall
)

$ErrorActionPreference = 'Stop'
$taskName = 'Proofline Wi-Fi Watchdog'

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "Please run this as administrator: right-click PowerShell > Run as administrator." -ForegroundColor Yellow
    exit 1
}

if ($Uninstall) {
    Unregister-ScheduledTask -TaskName $taskName -Confirm:$false -ErrorAction SilentlyContinue
    Write-Host "Removed '$taskName'. Logs are still in $FarmDir\logs."
    exit 0
}

if (-not (Get-NetAdapter -Name $Adapter -ErrorAction SilentlyContinue)) {
    Write-Host "No network adapter called '$Adapter'. Adapters on this PC:" -ForegroundColor Yellow
    Get-NetAdapter | Format-Table Name, InterfaceDescription, Status -AutoSize
    Write-Host "Run again with -Adapter '<name>'."
    exit 1
}

$binDir = Join-Path $FarmDir 'bin'
New-Item -ItemType Directory -Force -Path $binDir, (Join-Path $FarmDir 'logs') | Out-Null
$script = Join-Path $binDir 'wifi-watchdog.ps1'
Copy-Item (Join-Path $PSScriptRoot 'wifi-watchdog.ps1') $script -Force

$taskArgs = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$script`" -Adapter `"$Adapter`" -FarmDir `"$FarmDir`""
$action   = New-ScheduledTaskAction -Execute 'powershell.exe' -Argument $taskArgs
$every2   = New-ScheduledTaskTrigger -Once -At (Get-Date).AddMinutes(1) -RepetitionInterval (New-TimeSpan -Minutes 2)
$atBoot   = New-ScheduledTaskTrigger -AtStartup
$who      = New-ScheduledTaskPrincipal -UserId 'SYSTEM' -LogonType ServiceAccount -RunLevel Highest
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable `
                -MultipleInstances IgnoreNew -ExecutionTimeLimit (New-TimeSpan -Minutes 5)

Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $every2, $atBoot -Principal $who -Settings $settings `
    -Description 'Proofline Farm: checks the internet every 2 min and restarts the Wi-Fi adapter when it is stuck.' -Force | Out-Null

Write-Host "Installed '$taskName' (adapter '$Adapter')." -ForegroundColor Green
Write-Host "Log:   $FarmDir\logs\wifi.log"
Write-Host "State: $FarmDir\wifi-state.json"
