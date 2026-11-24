@echo off
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake was not found. Install CMake and add it to PATH.
    exit /b 1
)

echo [1/2] Configuring Windows PC build...
rem Use a fresh build directory to avoid a corrupted/locked Ninja database in build\pc.
cmake -S . -B build\pc_windows -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

echo [2/2] Building Windows PC version...
cmake --build build\pc_windows --config Release --parallel
if errorlevel 1 exit /b 1

if exist build\pc_windows\Release\fnaf.exe (
    echo.
    echo PC build complete: build\pc_windows\Release\fnaf.exe
    exit /b 0
) else if exist build\pc_windows\fnaf.exe (
    echo.
    echo PC build complete: build\pc_windows\fnaf.exe
    exit /b 0
) else (
    echo Build finished, but fnaf.exe was not found in the expected output folders.
    exit /b 1
)
