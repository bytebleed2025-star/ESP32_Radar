@echo off
setlocal
set "ADB=%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe"

if not exist "DetektorAir-debug.apk" (
    echo APK belum ada. Jalankan build_apk.bat dulu.
    exit /b 1
)

"%ADB%" install -r "DetektorAir-debug.apk"
if errorlevel 1 (
    echo.
    echo Gagal install. Pastikan:
    echo   - USB debugging aktif
    echo   - kabel USB data
    echo   - izinkan instalasi di HP
    exit /b 1
)

"%ADB%" shell monkey -p id.lacak.detektorair -c android.intent.category.LAUNCHER 1 >nul 2>&1
echo Aplikasi terpasang dan dibuka.
