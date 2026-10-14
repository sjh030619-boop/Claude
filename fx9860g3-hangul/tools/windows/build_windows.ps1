# One-shot build on Windows via WSL2 (Ubuntu). Run from PowerShell:
#   powershell -ExecutionPolicy Bypass -File tools\windows\build_windows.ps1
# Result: dist\Hangul.g1a (in this project folder). Untested on a real PC -- see README.
$ErrorActionPreference = "Stop"

if (-not (Get-Command wsl -ErrorAction SilentlyContinue)) {
    Write-Host "WSL이 없습니다. 관리자 PowerShell에서 'wsl --install -d Ubuntu' 실행 후 재부팅하고 다시 실행하세요."
    exit 1
}
$distros = ((wsl -l -q) -join "`n") -replace "`0", ""
if ($distros -notmatch "Ubuntu") {
    Write-Host "Ubuntu 배포판이 없습니다. 관리자 PowerShell에서 'wsl --install -d Ubuntu' 실행 후 재부팅하고 다시 실행하세요."
    exit 1
}

$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$wslRoot = (wsl -d Ubuntu wslpath -a "$root").Trim()
Write-Host "프로젝트: $root"
Write-Host "WSL 경로: $wslRoot"
Write-Host "처음 실행이면 비밀번호(sudo)를 물어보고, 툴체인 빌드에 30~60분 이상 걸립니다. 창을 닫지 마세요."

wsl -d Ubuntu bash "$wslRoot/tools/windows/build_in_wsl.sh" "$wslRoot"
if ($LASTEXITCODE -ne 0) {
    Write-Host "빌드 실패 (코드 $LASTEXITCODE). 위 메시지를 그대로 복사해서 알려 주세요."
    exit $LASTEXITCODE
}
Write-Host "완료: $root\dist\Hangul.g1a"
