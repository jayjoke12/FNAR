@echo off
setlocal
cd /d "%~dp0"

rem Prefer the caller's SDK variables; fall back to this checkout's ignored local.properties.
if not defined ANDROID_HOME if defined ANDROID_SDK_ROOT set "ANDROID_HOME=%ANDROID_SDK_ROOT%"
if not defined ANDROID_HOME if exist "android\local.properties" (
    for /f "usebackq tokens=1,* delims==" %%A in ("android\local.properties") do if "%%A"=="sdk.dir" set "ANDROID_HOME=%%B"
)
if not defined ANDROID_HOME (
    echo ERROR: Android SDK path not found. Set ANDROID_HOME or add sdk.dir to android\local.properties.
    exit /b 1
)
if not defined ANDROID_SDK_ROOT set "ANDROID_SDK_ROOT=%ANDROID_HOME%"

rem Restore the user's installed JDK and Gradle paths for shells opened before PATH was updated.
if not defined JAVA_HOME if exist "C:\Program Files\Eclipse Adoptium\jdk-17.0.21.10002-hotspot\bin\java.exe" set "JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-17.0.21.10002-hotspot"
if not defined JAVA_HOME (
    echo ERROR: JAVA_HOME is not set. Install JDK 17 and set JAVA_HOME.
    exit /b 1
)
set "GRADLE_HOME=F:\Programs\gradle-8.9"
set "PATH=%JAVA_HOME%\bin;%GRADLE_HOME%\bin;%ANDROID_HOME%\platform-tools;%PATH%"

where gradle >nul 2>nul
if errorlevel 1 (
    echo ERROR: Gradle 8.9 was not found at %GRADLE_HOME%.
    exit /b 1
)

echo Building Android ARM64 and x86_64 landscape APK...
call gradle -p android assembleRelease
if errorlevel 1 exit /b 1

if not exist "android\app\build\outputs\apk\release\app-release.apk" (
    echo ERROR: Gradle completed but the release APK was not found.
    exit /b 1
)

if not exist "build\Android" mkdir "build\Android"
if errorlevel 1 exit /b 1
copy /Y "android\app\build\outputs\apk\release\app-release.apk" "build\Android\app-release.apk" >nul
if errorlevel 1 exit /b 1

echo.
echo Android build complete:
echo build\Android\app-release.apk
