@echo off
setlocal
cd /d "%~dp0"

echo ==========================================
echo      FNAF - Build PC and Android
echo ==========================================
call "%~dp0build_pc.bat"
if errorlevel 1 (
    echo.
    echo PC build failed. Android build was not started.
    exit /b 1
)

call "%~dp0build_android.bat"
if errorlevel 1 (
    echo.
    echo Android build failed. PC build is available.
    exit /b 1
)

echo.
echo Both builds completed successfully.
if exist "%~dp0build\pc_windows\Release\fnaf.exe" (
    echo PC:      build\pc_windows\Release\fnaf.exe
) else (
    echo PC:      build\pc_windows\fnaf.exe
)
echo Android: build\Android\app-release.apk
exit /b 0
