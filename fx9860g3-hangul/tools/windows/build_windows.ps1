# One-shot build on Windows via WSL2 (Ubuntu). Run from PowerShell:
#   powershell -ExecutionPolicy Bypass -File tools\windows\build_windows.ps1
# Result: dist\Hangul.g1a (in this project folder). Untested on a real PC -- see README.
$ErrorActionPreference = "Stop"

if (-not (Get-Command wsl -ErrorAction SilentlyContinue)) {
    Write-Host "WSL not found. Run in an ADMIN PowerShell:  wsl --install -d Ubuntu   then reboot and run this script again."
    exit 1
}
$distros = ((wsl -l -q) -join "`n") -replace "`0", ""
if ($distros -notmatch "Ubuntu") {
    Write-Host "No Ubuntu distro in WSL. Installed distros: $($distros.Trim())"
    Write-Host "Run in an ADMIN PowerShell:  wsl --install -d Ubuntu   then reboot, create the Ubuntu user, and run this script again."
    exit 1
}

$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$wslRoot = (wsl -d Ubuntu wslpath -a "$root").Trim()
Write-Host "Project : $root"
Write-Host "WSL path: $wslRoot"
Write-Host "First run: asks for your Ubuntu (sudo) password and builds the toolchain (30-60+ min). Do not close this window."

wsl -d Ubuntu bash "$wslRoot/tools/windows/build_in_wsl.sh" "$wslRoot"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed (exit code $LASTEXITCODE). Copy the messages above and send them back."
    exit $LASTEXITCODE
}
Write-Host "DONE: $root\dist\Hangul.g1a"
