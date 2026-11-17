# v2.0.3 — WiFi AP+STA + Bluetooth SPP hybrid

Branch: `v2.0.3` (based on v2.0.1)

This is the hybrid build:

- v2.0.1 WiFi **AP + STA**
- embedded web UI at `http://192.168.4.1`
- web OTA and ArduinoOTA
- **Bluetooth Classic SPP** for the Android APK
- ST7735 animated radar, buzzer, inductive pass counter, meters

The ESP32 shares one 2.4 GHz radio between WiFi and Bluetooth. Both can run at
the same time, but heavy WiFi traffic can reduce Bluetooth responsiveness and
vice versa.

## Renders

| Welcome (3 s) | Scan artwork + live values | Animation sequence |
|---|---|---|
| ![](st7735-welcome.jpg) | ![](st7735-scan-overlay.png) | ![](st7735-animation.png) |

## Download

| File | Use |
|---|---|
| [`../bin/v2.0.3/DetektorAir-v2.0.3-ST7735-WiFi-BT-factory.bin`](../bin/v2.0.3/DetektorAir-v2.0.3-ST7735-WiFi-BT-factory.bin) | blank / new board, flash at **0x0** |
| [`../bin/v2.0.3/DetektorAir-v2.0.3-ST7735-WiFi-BT-ota.bin`](../bin/v2.0.3/DetektorAir-v2.0.3-ST7735-WiFi-BT-ota.bin) | web OTA from v2.0.1/v2.0.3 |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3.apk) | Android Bluetooth app |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3b.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3b.apk) | APK with secure → insecure → RFCOMM channel 1 fallback |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3c.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3c.apk) | Same fallback + scrollable device picker |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3d.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3d.apk) | Adds Android 12+ `BLUETOOTH_SCAN` permission fix |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3e.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3e.apk) | **Hybrid APK**: Bluetooth local UI **+** WiFi web UI (`192.168.4.1`) |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3f.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3f.apk) | **Latest hybrid APK**: distance unit `m` keeps the raw value |

```cmd
esptool.py --chip esp32 --port COM5 write_flash 0x0 bin\v2.0.3\DetektorAir-v2.0.3-ST7735-WiFi-BT-factory.bin
```

Checksums: [`../bin/v2.0.3/SHA256SUMS.txt`](../bin/v2.0.3/SHA256SUMS.txt).

## Connect

### Web UI over WiFi

```text
SSID: DetektorAir
Pass: radar12345
URL : http://192.168.4.1
```

The existing CONFIG tab, network settings, OTA and buzzer controls are
available.

### Android APK over Bluetooth

```text
Name: DetektorAir
Type: Bluetooth Classic SPP
PIN : 1234
```

1. Pair **DetektorAir** (PIN 1234) in Android Bluetooth settings.
2. Install `DetektorAir-Bluetooth-v2.0.3f.apk`.
3. Select **DetektorAir** and tap **HUBUNGKAN**.
4. Press **Back** in the radar to disconnect.

### Android APK over WiFi

`2.0.3f` is a hybrid app: it can also open the ESP32 web UI directly.

1. Connect the phone to the `DetektorAir` hotspot (`radar12345`).
2. Tap **BUKA WEB UI (WIFI)**.
3. The full web UI opens, including **CONFIG** and **OTA**.
4. Press **Back** to return to the app.

Bluetooth mode loads the local UI from the APK; WiFi mode loads the real
remote UI from the ESP32, so CONFIG/OTA are available there.

Both can be used at the same time: the web UI can stay open while the phone
uses Bluetooth.

## Display artwork

- **WelcomeScreen.jpg** is shown for **3 seconds** after the display initializes.
- Then **RadarScan_00 → 01 → 02 → 03 → 00** loops continuously:
  - **1 frame per second**
  - **1 MHz SPI**
  - 4-frame cycle takes about 4 seconds
- The distance and pass count are drawn in **FreeSansBold (modern sans)**
  inside the white boxes of the artwork.
- The animation uses the same four JPEGs already stored in flash; there are no
  extra image assets.

The previous distance-based background mapping is still available by setting:

```cpp
#define SCAN_ANIMATE 0
```

| Frame | Distance-based condition |
|---|---|
| `RadarScan_00` | no echo / idle |
| `RadarScan_01` | object farther than the alert threshold |
| `RadarScan_02` | object inside the alert threshold |
| `RadarScan_03` | object very close (half the alert threshold) |

## Boot fix (WiFi + Bluetooth)

Earlier hybrid builds called `WiFi.setSleep(false)` while Bluetooth Classic
was already running. ESP-IDF aborts that combination:

```text
E wifi: Should enable WiFi modem sleep when both WiFi and Bluetooth are enabled
abort() was called at PC ...
```

The crash happened before the ST7735 was initialized, which left the panel
white. This build keeps WiFi modem sleep enabled, so the display and both
radios start normally.

## Build result

```text
Firmware: 1,916,384 bytes (~97.5% of the OTA slot)
RAM:         68,712 bytes (21.0%)
APK:        329,567 bytes, version 2.0.3f
```

> Flash usage is about **97.5%** of the 1.875 MB OTA slot. It fits, but there is
> little headroom. Avoid adding large new assets to this build.

## Distance unit

All distance readouts use the unit **`m`** and show the raw sensor value as-is.
The value is **not** divided by 100: a 33 cm reading is displayed as `33 m`.

## Protocol

Bluetooth and WebSocket use the same JSON telemetry:

```json
{"a":91,"d":33,"mx":200,"pc":7}
```

Config on connect/change:

```json
{"cfg":1,"mx":200,"thr":50,"pc":7,"bz":1,"ba":1,"b0":1000,"b1":1500,"b2":500,"b3":750,"b4":5000}
```

Commands: `C0`, `B1/B0`, `Y1/Y0`, `Zlo,lf,so,sf,hold`, `R<cm>`, `T<cm>`.

## Notes

- WiFi + Bluetooth Classic coexistence is supported by the ESP32 radio, but
  **WiFi modem sleep must stay enabled** while Bluetooth is active. Calling
  `WiFi.setSleep(false)` makes ESP-IDF abort at boot with
  `Should enable WiFi modem sleep when both WiFi and Bluetooth are enabled`.
  This build keeps modem sleep on.
- The APK is debug-signed; allow “install unknown apps”.
- OTA remains available in the web UI.
- The Android APK (`2.0.3f`) works both ways: Bluetooth mode shows the local
  UI, WiFi mode opens the ESP32 web UI with CONFIG/OTA.
