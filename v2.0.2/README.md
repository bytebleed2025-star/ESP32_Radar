# v2.0.2 — Bluetooth SPP, no WiFi AP/STA

Branch: `v2.0.2` (based on v2.0.1)

This build removes WiFi AP/STA, the web server, WebSocket and OTA. The ESP32
exposes a **Bluetooth Classic SPP** serial link, and the Android APK connects
directly and hosts the radar UI in a local WebView.

## Download

| File | Use |
|---|---|
| [`../bin/v2.0.2/DetektorAir-v2.0.2-ST7735-BT-factory.bin`](../bin/v2.0.2/DetektorAir-v2.0.2-ST7735-BT-factory.bin) | blank / new board, flash at **0x0** over USB |
| [`../bin/v2.0.2/DetektorAir-v2.0.2-ST7735-BT-ota.bin`](../bin/v2.0.2/DetektorAir-v2.0.2-ST7735-BT-ota.bin) | optional one-time update from v2.0.1 web OTA |
| [`../bin/v2.0.2/DetektorAir-Bluetooth-v2.0.2.apk`](../bin/v2.0.2/DetektorAir-Bluetooth-v2.0.2.apk) | Android app |

```cmd
esptool.py --chip esp32 --port COM5 write_flash 0x0 bin\v2.0.2\DetektorAir-v2.0.2-ST7735-BT-factory.bin
```

Checksums: [`../bin/v2.0.2/SHA256SUMS.txt`](../bin/v2.0.2/SHA256SUMS.txt).

## Bluetooth

```text
Name: DetektorAir
Type: Bluetooth Classic SPP
PIN : 1234
```

1. Power the ESP32.
2. On Android: **Settings → Bluetooth → Pair new device → DetektorAir**.
3. Enter PIN **1234**.
4. Install and open the APK.
5. Select **DetektorAir** from the paired-device list.
6. Tap **HUBUNGKAN**.

The app opens the radar UI locally and receives telemetry over SPP. Press
**Back** in the radar to disconnect and return to the device list.

> There is no WiFi hotspot, no `192.168.4.1`, and no web OTA in v2.0.2.
> Firmware updates are done over USB with the factory image at `0x0`.

## App

- Package: `id.lacak.detektorair`
- Version: 2.0.2
- minSdk 24, targetSdk 34
- Debug-signed APK (allow “install unknown apps” on the phone)
- Lists already-paired Bluetooth devices; pair DetektorAir in Android settings
  first
- The local radar UI is the same 320px web UI, with the CONFIG tab hidden
- Controls available: distance, radar sweep, pass counter, reset, buzzer
  on/off, buzzer timing, range and water-alert thresholds

## Hardware

Same as v2.0.1:

| Function | ESP32 pin |
|---|---|
| HC-SR04 `TRIG` / `ECHO` | GPIO 5 / GPIO 18 (1k/2k divider) |
| ST7735 `SCLK` / `MOSI` / `MISO` | GPIO 14 / 13 / 12 |
| ST7735 `CS` / `DC` / `RST` | GPIO 27 / 26 / 25 |
| ST7735 `BLK` | GPIO 32 (active-high) |
| Buzzer `+` | GPIO 23 |
| Inductive sensor | GPIO 34 |
| Reset button | GPIO 33 |

Display config: `ST7735_GREENTAB`, HSPI, **1 MHz**, `MISO=GPIO12`.

## Protocol

The ESP32 sends newline-delimited JSON at 10 Hz:

```json
{"a":91,"d":33,"mx":200,"pc":7}
```

On connect and after configuration changes:

```json
{"cfg":1,"mx":200,"thr":50,"pc":7,"bz":1,"ba":1,"b0":1000,"b1":1500,"b2":500,"b3":750,"b4":5000}
```

Commands from the app:

```text
C0                  reset pass counter
B1 / B0             buzzer on/off
Y1 / Y0             active/passive buzzer
Zlo,lf,so,sf,hold   buzzer timings (ms)
R<cm>               range
T<cm>               water-alert threshold
```

## Build

```bash
pio run -e esp32dev
```

- PlatformIO firmware: **1,190,149 bytes (60.5%) flash**, **40,444 bytes (12.3%) RAM**
- APK: **354,714 bytes**, version 2.0.2

APK build:

```powershell
python tools\build_apk_assets.py
cd android_app
.\gradlew.bat assembleDebug
```

## Notes

- Bluetooth Classic only; iOS is not supported by this APK.
- One Bluetooth client at a time.
- No OTA after this build; use USB for future firmware updates.
- The `-ota.bin` can be uploaded once from a v2.0.1 web UI if the board still
  runs v2.0.1. After that, WiFi is gone.
