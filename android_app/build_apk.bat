@echo off
setlocal
set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
set "ANDROID_SDK_ROOT=%ANDROID_HOME%"
if "%JAVA_HOME%"=="" set "JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-21.0.12.8-hotspot"

if not exist "local.properties" (
    echo sdk.dir=%ANDROID_HOME:\=/%>local.properties
)

echo Building Detektor Air APK...
call gradlew.bat assembleDebug
if errorlevel 1 goto :fail

copy /Y "app\build\outputs\apk\debug\app-debug.apk" "DetektorAir-debug.apk" >nul
echo.
echo APK siap:
echo   %CD%\DetektorAir-debug.apk
echo.
echo Install ke HP:
echo   install_apk.bat
goto :eof

:fail
echo.
echo Build gagal.
exit /b 1
