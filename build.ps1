# Build script for Windows PowerShell (replaces "make").
#
# Usage (run from the project root, e.g. E:\AlgorithmicTrading):
#   powershell -ExecutionPolicy Bypass -File .\build.ps1 test    # build + run all tests
#   powershell -ExecutionPolicy Bypass -File .\build.ps1 run     # build + run the demo
#   powershell -ExecutionPolicy Bypass -File .\build.ps1 all     # build the demo only
#   powershell -ExecutionPolicy Bypass -File .\build.ps1 clean   # delete the build folder
#
# If scripts are already allowed on your PC you can simply type:  .\build.ps1 test

param([string]$Target = "test")

$flags = @("-std=c++17", "-Wall", "-Wextra", "-O2")
$build = "build"

# ---- check the compiler exists ----
if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: g++ was not found. Install a C++ compiler (MinGW-w64 / MSYS2 / WinLibs)"
    Write-Host "and make sure its 'bin' folder is on your PATH. Check with:  g++ --version"
    exit 1
}

# ---- clean ----
if ($Target -eq "clean") {
    if (Test-Path $build) { Remove-Item -Recurse -Force $build }
    Write-Host "Cleaned."
    exit 0
}

New-Item -ItemType Directory -Force -Path $build | Out-Null

# ---- compile every src/*.cpp into build/*.o ----
$libObjs = @()
foreach ($f in Get-ChildItem -Path "src" -Filter "*.cpp") {
    $obj = Join-Path $build ($f.BaseName + ".o")
    Write-Host "Compiling src\$($f.Name)"
    & g++ @flags -c $f.FullName -o $obj
    if ($LASTEXITCODE -ne 0) { Write-Host "COMPILE FAILED: $($f.Name)"; exit 1 }
    $libObjs += $obj
}

# ---- build the demo program ----
function Build-App {
    $exe = Join-Path $build "trading_app.exe"
    Write-Host "Linking $exe"
    & g++ @flags "main.cpp" @libObjs -o $exe
    if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED: main.cpp"; exit 1 }
    return
}

if ($Target -eq "all" -or $Target -eq "run") {
    Build-App
    if ($Target -eq "run") {
        & (Join-Path $build "trading_app.exe")
    }
    exit 0
}

# ---- build and run every tests/test_*.cpp ----
if ($Target -eq "test") {
    $anyFailed = $false
    foreach ($t in Get-ChildItem -Path "tests" -Filter "test_*.cpp") {
        $exe = Join-Path $build ($t.BaseName + ".exe")
        Write-Host ""
        Write-Host "== $($t.BaseName)"
        & g++ @flags $t.FullName @libObjs -o $exe
        if ($LASTEXITCODE -ne 0) {
            Write-Host "BUILD FAILED: $($t.Name)"
            $anyFailed = $true
            continue
        }
        & $exe
        if ($LASTEXITCODE -ne 0) { $anyFailed = $true }
    }
    Write-Host ""
    if ($anyFailed) { Write-Host "SOME TEST SUITES FAILED"; exit 1 }
    else { Write-Host "ALL TEST SUITES PASSED"; exit 0 }
}

Write-Host "Unknown target '$Target'. Use: test | run | all | clean"
exit 1
