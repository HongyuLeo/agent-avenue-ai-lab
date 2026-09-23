$ErrorActionPreference = "Stop"
Set-Location (Split-Path -Parent $PSScriptRoot)

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake was not found. Install Visual Studio 2022 with 'Desktop development with C++' and CMake tools."
}

cmake -S . -B build -A x64
cmake --build build --config Release --parallel

$exe = Join-Path (Get-Location) "build\Release\agent_avenue_ai_lab.exe"
if (-not (Test-Path $exe)) { throw "Build completed but the EXE was not found at $exe" }
Write-Host "Build succeeded: $exe" -ForegroundColor Green
