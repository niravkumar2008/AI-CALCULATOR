<#
  Makes the Proofline Farm manager always on: a Scheduled Task starts it hidden every time you log in to Windows,
  and restarts it if it crashes. No administrator rights needed (it runs as you).

  Install:   powershell -ExecutionPolicy Bypass -File install-farm-autostart.ps1
  Remove:    powershell -ExecutionPolicy Bypass -File install-farm-autostart.ps1 -Uninstall
  Pause the farm without removing it: create an empty file named PAUSE in C:\Users\<you>\ProoflineFarm
#>
param([switch]$Uninstall)

$ErrorActionPreference = 'Stop'
$taskName = 'Proofline Farm'

if ($Uninstall) {
    Stop-ScheduledTask -TaskName $taskName -ErrorAction SilentlyContinue
    Unregister-ScheduledTask -TaskName $taskName -Confirm:$false -ErrorAction SilentlyContinue
    Write-Host "Removed '$taskName'. The farm no longer starts at login."
    exit 0
}

$node = (Get-Command node -ErrorAction Stop).Source
$manager = Join-Path $PSScriptRoot 'manager.cjs'
$user = "$env:USERDOMAIN\$env:USERNAME"

$action   = New-ScheduledTaskAction -Execute $node -Argument "`"$manager`" run" -WorkingDirectory $PSScriptRoot
$trigger  = New-ScheduledTaskTrigger -AtLogOn -User $user
$who      = New-ScheduledTaskPrincipal -UserId $user -LogonType Interactive -RunLevel Limited
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable `
                -MultipleInstances IgnoreNew -ExecutionTimeLimit ([TimeSpan]::Zero) `
                -RestartCount 5 -RestartInterval (New-TimeSpan -Minutes 1) -Hidden

Register-ScheduledTask -TaskName $taskName -Action $action -Trigger $trigger -Principal $who -Settings $settings `
    -Description 'Proofline Farm manager: works on tasks in ProoflineFarm\tasks within the usage rules. Pause with a PAUSE file.' -Force | Out-Null

Write-Host "Installed '$taskName': starts hidden at every login." -ForegroundColor Green
