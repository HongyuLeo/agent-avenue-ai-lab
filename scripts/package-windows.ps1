param(
    [string]$BuildDirectory = "build",
    [string]$V3Checkpoint = "",
    [string]$TrainingLog = "",
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
if ($V3Checkpoint) {
    if (-not (Test-Path -LiteralPath $V3Checkpoint)) { throw "V3 checkpoint not found: $V3Checkpoint" }
    Copy-Item -LiteralPath $V3Checkpoint -Destination (Join-Path $stage "training-v3.bin")
}
if ($TrainingLog) {
    if (-not (Test-Path -LiteralPath $TrainingLog)) { throw "Training log not found: $TrainingLog" }
    Copy-Item -LiteralPath $TrainingLog -Destination (Join-Path $stage "training-evaluations.csv")
}
Copy-Item LICENSE (Join-Path $stage "LICENSE.txt")
Copy-Item docs\RELEASE_NOTES_v3.1.0.md (Join-Path $stage "README.txt")
if (Test-Path $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Host "Package created: $zip" -ForegroundColor Green
