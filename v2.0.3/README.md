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
| [`../bin/v2.0.3/DetektorAir-v2.0.3-ST7735-WiFi-BT-factory.bin`](../bin/v2.0.3/DetektorAir-v2.0.3-ST7735-WiFi-BT-factory.bin) | **default hybrid**, v2.0.1-style mirror display, flash at **0x0** |
| [`../bin/v2.0.3/DetektorAir-v2.0.3-ST7735-WiFi-BT-ota.bin`](../bin/v2.0.3/DetektorAir-v2.0.3-ST7735-WiFi-BT-ota.bin) | default build for web OTA |
| [`../bin/v2.0.3/DetektorAir-v2.0.3-Artwork-WiFi-BT-factory.bin`](../bin/v2.0.3/DetektorAir-v2.0.3-Artwork-WiFi-BT-factory.bin) | alternate JPEG-artwork display, flash at **0x0** |
| [`../bin/v2.0.3/DetektorAir-v2.0.3-Artwork-WiFi-BT-ota.bin`](../bin/v2.0.3/DetektorAir-v2.0.3-Artwork-WiFi-BT-ota.bin) | artwork build for **web OTA** |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3.apk) | Android Bluetooth app |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3b.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3b.apk) | APK with secure → insecure → RFCOMM channel 1 fallback |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3c.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3c.apk) | Same fallback + scrollable device picker |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3d.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3d.apk) | Adds Android 12+ `BLUETOOTH_SCAN` permission fix |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3e.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3e.apk) | **Hybrid APK**: Bluetooth local UI **+** WiFi web UI (`192.168.4.1`) |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3f.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3f.apk) | Hybrid APK with raw `m` value |
| [`../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3g.apk`](../bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3g.apk) | **Latest hybrid APK**: live scan data (no false demo mode) |

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
2. Install `DetektorAir-Bluetooth-v2.0.3g.apk`.
3. Select **DetektorAir** and tap **HUBUNGKAN**.
4. Press **Back** in the radar to disconnect.

### Android APK over WiFi

`2.0.3g` is a hybrid app: it can also open the ESP32 web UI directly.

1. Connect the phone to the `DetektorAir` hotspot (`radar12345`).
2. Tap **BUKA WEB UI (WIFI)**.
3. The full web UI opens, including **CONFIG** and **OTA**.
4. Press **Back** to return to the app.

Bluetooth mode loads the local UI from the APK; WiFi mode loads the real
remote UI from the ESP32, so CONFIG/OTA are available there.

Both can be used at the same time: the web UI can stay open while the phone
uses Bluetooth.

## Display (default): v2.0.1 mirror of the web UI

The ST7735 is drawn in code and mirrors the live web UI — same sweep angle,
distance, alert state and pass count.

```text
LACAK                    v2.0.3
┌──────────────┬─────────┐
│  radar rings │ JARAK   │
│  sweep+blips │ 58      │
│              │ m       │
│              │ AMAN    │
│              │ PASSES  │
│              │      0  │
│              │ MAX 200m│
└──────────────┴─────────┘
```

- Header **LACAK**; version on the right.
- `JARAK` uses the raw sensor value with the unit **`m`** (`58`, not `0.58`).
- `AMAN` / `TERDETEKSI` follows the alert threshold.
- `PASSES` is the inductive counter.
- `MAX` is the configured range, also raw + `m`.
- The sweep updates every 200 ms and uses the same virtual angle as the web UI.

## Alternate display: JPEG artwork

The artwork build is kept as an option:

- **WelcomeScreen.jpg** is shown for **3 seconds** after the display initializes.
- Then **RadarScan_00 → 01 → 02 → 03 → 00** loops continuously:
  - **1 frame per second**
  - **1 MHz SPI**
  - 4-frame cycle takes about 4 seconds
- The distance and pass count are drawn in **FreeSansBold (modern sans)**
  inside the white boxes of the artwork.
- The JPEGs are **baseline** 160×128. `TJpg_Decoder` cannot decode progressive
  JPEGs, so the artwork is re-encoded by `tools/build_st7735_images.py`.
- The header in the scan frames reads **LACAK v2.0.3 SHD**.

### Display variants

| Build | Display | PlatformIO env |
|---|---|---|
| `DetektorAir-v2.0.3-ST7735-...` | v2.0.1-style mirror (default) | `esp32dev` |
| `DetektorAir-v2.0.3-Artwork-...` | JPEG artwork (welcome + scan frames) | `esp32dev-art` |

Both have identical WiFi AP+STA, web UI/OTA and Bluetooth SPP. Build the
artwork variant with:

```bash
pio run -e esp32dev-art
```

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

## Web UI / APK fix

Telemetry packets (`{"a":...,"d":...,"mx":...,"pc":...}`) also contain the
proximity counter `pc`. The web UI treated any packet with `pc` as a
counter-only update and returned early, so the sweep, distance and detection
never updated even though the WebSocket was connected.

- The `pc` early-return now applies only to counter-only packets.
- The APK's WebView no longer falls into demo mode (`file://`) — the injected
  Bluetooth shim sets `window.__LACAK_LIVE = true`.

## Build result

```text
Firmware: 1,864,625 bytes (94.8% of the OTA slot)
RAM:         65,104 bytes (19.9%)
APK:        329,691 bytes, version 2.0.3g
```

> Flash usage is about **97.0%** of the 1.875 MB OTA slot. It fits, but there is
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
- The Android APK (`2.0.3g`) works both ways: Bluetooth mode shows the local
  UI, WiFi mode opens the ESP32 web UI with CONFIG/OTA.
