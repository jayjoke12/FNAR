@echo off
setlocal
cd /d "%~dp0"

echo ==========================================
echo      FNAF - Build PC and Android
echo ==========================================
call build_pc.bat
if errorlevel 1 (
    echo.
    echo PC build failed. Android build was not started.
    exit /b 1
)

call build_android.bat
if errorlevel 1 (
    echo.
    echo Android build failed. PC build is available.
    exit /b 1
)

echo.
echo Both builds completed successfully.
