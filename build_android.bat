@echo off
setlocal
cd /d "%~dp0"

if "%ANDROID_HOME%"=="" if "%ANDROID_SDK_ROOT%"=="" (
    echo ERROR: Set ANDROID_HOME or ANDROID_SDK_ROOT to your Android SDK folder.
    exit /b 1
)

where gradle >nul 2>nul
if errorlevel 1 (
    echo ERROR: Gradle was not found. Install Gradle 8.9 or use Android Studio.
    exit /b 1
)

echo Building Android ARM64 landscape APK...
gradle -p android assembleRelease
if errorlevel 1 exit /b 1

echo.
echo Android build complete:
echo android\app\build\outputs\apk\release\app-release.apk
