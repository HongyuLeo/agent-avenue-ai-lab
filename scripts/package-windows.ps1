param(
    [string]$BuildDirectory = "build",
    [switch]$SkipBuild
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (-not $SkipBuild) { powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1 }

$stage = Join-Path $root "$BuildDirectory\package-v3"
$zip = Join-Path $root "build\AgentAvenueAI-Public-v3.1.0-Windows.zip"
$binaryDirectory = Join-Path $root "$BuildDirectory\Release"
if (-not (Test-Path (Join-Path $binaryDirectory "agent_avenue_ai_lab.exe"))) {
    $binaryDirectory = Join-Path $root $BuildDirectory
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $zip) | Out-Null
if (Test-Path $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path (Join-Path $stage "assets") | Out-Null
Copy-Item (Join-Path $binaryDirectory "agent_avenue_ai_lab.exe") (Join-Path $stage "AgentAvenueAI.exe")
Copy-Item assets\cards (Join-Path $stage "assets\cards") -Recurse
Copy-Item models\pretrained-17m.bin (Join-Path $stage "training.bin")
Copy-Item LICENSE (Join-Path $stage "LICENSE.txt")
Copy-Item docs\RELEASE_NOTES_v3.1.0.md (Join-Path $stage "README.txt")
if (Test-Path $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Host "Package created: $zip" -ForegroundColor Green
