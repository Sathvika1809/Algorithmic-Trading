$ErrorActionPreference = "Stop"

if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    throw "Python was not found on PATH. Install Python and reopen PowerShell."
}
if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    throw "g++ was not found on PATH. Check your MinGW-w64 installation."
}

Write-Host "Installing/checking Python dependency..."
python -m pip install -r requirements.txt
if ($LASTEXITCODE -ne 0) { throw "Could not install Python dependencies." }

Write-Host "Downloading five years of NSE daily data..."
python .\scripts\download_nse_data.py --years 5
if ($LASTEXITCODE -ne 0) { throw "Data download failed. Check the symbol-specific error messages." }

Write-Host "Compiling C++17 backtest..."
$sourceFiles = Get-ChildItem .\src -Filter *.cpp | ForEach-Object { $_.FullName }
& g++ -std=c++17 -Wall -Wextra -O2 .\backtest_main.cpp @sourceFiles -o .\backtest.exe
if ($LASTEXITCODE -ne 0) { throw "C++ compilation failed." }

Write-Host "Running backtest..."
.\backtest.exe
if ($LASTEXITCODE -ne 0) { throw "Backtest failed." }
