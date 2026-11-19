# Puts a "Proofline Farm" icon on the desktop and in the Start menu. Double-click it to open the farm app.
#   powershell -ExecutionPolicy Bypass -File make-shortcut.ps1
$ErrorActionPreference = 'Stop'
$app = $PSScriptRoot
$electron = Join-Path $app 'node_modules\electron\dist\electron.exe'
if (-not (Test-Path $electron)) {
    Write-Host 'Installing the app parts first (one time, about 1 minute)...'
    Push-Location $app; npm install --no-audit --no-fund; Pop-Location
}
$ico = Join-Path $app 'proofline-farm.ico'
node -e "require('fs').writeFileSync(process.argv[1], require(process.argv[2]).ico())" $ico (Join-Path $app 'icon.cjs')

$shell = New-Object -ComObject WScript.Shell
$places = @([Environment]::GetFolderPath('Desktop'), (Join-Path ([Environment]::GetFolderPath('Programs')) ''))
foreach ($dir in $places) {
    $lnk = $shell.CreateShortcut((Join-Path $dir 'Proofline Farm.lnk'))
    $lnk.TargetPath = $electron
    $lnk.Arguments = "`"$app`""
    $lnk.WorkingDirectory = $app
    $lnk.IconLocation = "$ico,0"
    $lnk.Description = 'Proofline Farm: see and steer your Claude farmhands'
    $lnk.Save()
}
Write-Host 'Made "Proofline Farm" on your desktop and in the Start menu.' -ForegroundColor Green
