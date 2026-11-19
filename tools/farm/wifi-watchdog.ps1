<#
  Proofline Farm - Wi-Fi watchdog.

  Does the "turn Wi-Fi off and on" fix automatically. Task Scheduler runs it every 2 minutes
  (install with install-wifi-watchdog.ps1). Each run:
    1. checks for real internet (an HTTPS request, not just "Wi-Fi connected"),
    2. after 3 failed checks in a row (about 6 min) restarts the Wi-Fi adapter,
    3. after 3 restarts that didn't help, waits 15 min before trying again,
    4. logs to <FarmDir>\logs\wifi.log and keeps its state in <FarmDir>\wifi-state.json
       (the farm manager reads "online" from that file so it waits instead of failing tasks).

  Test without touching the adapter:
    powershell -ExecutionPolicy Bypass -File wifi-watchdog.ps1 -DryRun -SimulateOffline
#>
param(
    [string]$Adapter = 'Wi-Fi',
    [string]$FarmDir = 'C:\Users\r_kas\ProoflineFarm',
    [int]$FailsBeforeRestart = 3,
    [int]$MaxRestartsInARow = 3,
    [int]$BackoffMinutes = 15,
    [switch]$DryRun,          # log what it would do, never restart the adapter
    [switch]$SimulateOffline  # pretend the internet is down (for testing)
)

$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$logDir    = Join-Path $FarmDir 'logs'
$logFile   = Join-Path $logDir 'wifi.log'
$stateFile = Join-Path $FarmDir 'wifi-state.json'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

function Write-Log([string]$msg) {
    if ((Test-Path $logFile) -and (Get-Item $logFile).Length -gt 1MB) {
        Move-Item $logFile "$logFile.old" -Force
    }
    $tag = if ($DryRun) { ' [dry run]' } else { '' }
    Add-Content -Path $logFile -Value ("{0:yyyy-MM-dd HH:mm:ss}{1}  {2}" -f (Get-Date), $tag, $msg) -Encoding UTF8
}

# Online = any HTTPS/HTTP reply gets through. An error reply (404, 401) still proves the internet works.
# No ping: ping can succeed while DNS is broken, and then Claude still can't connect.
function Test-Online {
    if ($SimulateOffline) { return $false }
    foreach ($url in 'https://api.anthropic.com', 'http://www.msftconnecttest.com/connecttest.txt') {
        try {
            Invoke-WebRequest -Uri $url -Method Head -TimeoutSec 10 -UseBasicParsing | Out-Null
            return $true
        } catch [System.Net.WebException] {
            if ($_.Exception.Response) { return $true }
        } catch { }
    }
    return $false
}

function Read-State {
    $s = @{ fails = 0; restarts = 0; lastRestart = ''; downSince = ''; restarting = $false; online = $true; checkedAt = '' }
    if (Test-Path $stateFile) {
        try {
            $j = Get-Content $stateFile -Raw | ConvertFrom-Json
            foreach ($k in @($s.Keys)) { if ($null -ne $j.$k) { $s[$k] = $j.$k } }
        } catch { Write-Log "state file unreadable, starting fresh" }
    }
    return $s
}

function Save-State($s) {
    $s.checkedAt = (Get-Date).ToString('s')
    ($s | ConvertTo-Json) | Set-Content -Path $stateFile -Encoding UTF8
}

function Minutes-Since([string]$iso) {
    if (-not $iso) { return [double]::MaxValue }
    return ((Get-Date) - [datetime]$iso).TotalMinutes
}

$s = Read-State
$now = Get-Date

$nic = Get-NetAdapter -Name $Adapter -ErrorAction SilentlyContinue
if (-not $nic) { Write-Log "adapter '$Adapter' not found, nothing to do"; Save-State $s; exit 0 }

# Adapter switched off: if a restart of ours was cut short, switch it back on; if you did it, leave it alone.
if ($nic.AdminStatus -eq 'Down') {
    if ($s.restarting -and -not $DryRun) {
        Write-Log "adapter left off by an unfinished restart, switching it back on"
        Enable-NetAdapter -Name $Adapter -Confirm:$false
        $s.restarting = $false
    }
    Save-State $s
    exit 0
}

if (Test-Online) {
    if ($s.downSince) {
        Write-Log ("back online, was down {0:N0} min, restarts this time: {1}" -f (Minutes-Since $s.downSince), $s.restarts)
    }
    $s.fails = 0; $s.restarts = 0; $s.downSince = ''; $s.online = $true
    Save-State $s
    exit 0
}

# Offline.
$s.online = $false
if (-not $s.downSince) { $s.downSince = $now.ToString('s'); Write-Log "no internet (check 1 of $FailsBeforeRestart)" }
$s.fails = [int]$s.fails + 1

if ($s.fails -lt $FailsBeforeRestart) { Save-State $s; exit 0 }

if ([int]$s.restarts -ge $MaxRestartsInARow) {
    if ((Minutes-Since $s.lastRestart) -lt $BackoffMinutes) { Save-State $s; exit 0 }
    Write-Log "$MaxRestartsInARow restarts didn't help, $BackoffMinutes min wait is over, trying again"
    $s.restarts = 0
}

$s.restarts = [int]$s.restarts + 1
$s.lastRestart = $now.ToString('s')
Write-Log ("restarting '{0}' (restart {1} of {2}, down {3:N0} min)" -f $Adapter, $s.restarts, $MaxRestartsInARow, (Minutes-Since $s.downSince))

if (-not $DryRun) {
    $s.restarting = $true
    Save-State $s
    try {
        Restart-NetAdapter -Name $Adapter -Confirm:$false
    } catch {
        Write-Log "restart failed: $($_.Exception.Message)"
    }
    $s.restarting = $false
    Start-Sleep -Seconds 30
}

if (Test-Online) {
    Write-Log ("restart fixed it, was down {0:N0} min" -f (Minutes-Since $s.downSince))
    $s.fails = 0; $s.restarts = 0; $s.downSince = ''; $s.online = $true
} elseif ($s.restarts -ge $MaxRestartsInARow) {
    Write-Log "still no internet after $MaxRestartsInARow restarts, waiting $BackoffMinutes min"
} else {
    Write-Log "still no internet, will restart again at the next check"
}
Save-State $s
