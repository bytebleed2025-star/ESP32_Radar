# DetektorAir Android APK (Bluetooth + WiFi)

The APK works in two modes:

- **Bluetooth** — connects to the ESP32 over **Bluetooth Classic SPP** and hosts
  the radar UI locally in a WebView.
- **WiFi** — opens the ESP32 web UI (`http://192.168.4.1`) directly, including
  the CONFIG tab, network settings and OTA.

## Pairing (Bluetooth)

1. Power the ESP32.
2. Android: **Settings → Bluetooth → Pair new device → DetektorAir**.
3. PIN: **1234**.
4. Install and open `DetektorAir-Bluetooth-v2.0.3g.apk`.
5. Select `DetektorAir` and tap **HUBUNGKAN**.

Press **Back** while the radar is shown to disconnect and return to the device
list.

## WiFi mode

1. Connect the phone to the `DetektorAir` hotspot.
2. Password: `radar12345`.
3. Tap **BUKA WEB UI (WIFI)**.
4. The full web UI opens, including CONFIG and OTA.
5. Press **Back** to return to the app.

## Build

From the repository root:

```powershell
python tools\build_apk_assets.py
cd android_app
.\gradlew.bat assembleDebug
```

Output:

```text
android_app/app/build/outputs/apk/debug/app-debug.apk
```

The release copy is stored as:

```text
bin/v2.0.3/DetektorAir-Bluetooth-v2.0.3g.apk
```

## How it works

- `MainActivity` lists paired Bluetooth devices in a scrollable dropdown.
- It opens an RFCOMM socket to the SPP UUID
  `00001101-0000-1000-8000-00805F9B34FB`, falling back to insecure RFCOMM and
  then RFCOMM channel 1.
- The ESP32 sends newline-delimited JSON; the app forwards each line into the
  WebView via `window.__btDeliver(...)`.
- The WebView's WebSocket is replaced by a Bluetooth transport shim
  (`window.AndroidBT.send(...)`), so the existing radar UI works unchanged.
- The CONFIG tab is hidden in Bluetooth mode. Use WiFi mode (or a browser) for
  firmware/network settings and OTA.

## Permissions

- Android 12+: `BLUETOOTH_CONNECT` and `BLUETOOTH_SCAN` (requested at first
  launch). `cancelDiscovery()` is guarded in case scan permission is denied.
- Android 11 and below: legacy `BLUETOOTH` / `BLUETOOTH_ADMIN`.
- The app only lists **already paired** devices. Pairing is done in Android
  Bluetooth settings.
- `INTERNET` for WiFi mode; cleartext HTTP is allowed for `192.168.4.1`.

## Limits

- Bluetooth Classic only; no iOS/BLE support.
- One Bluetooth client at a time.
- Debug-signed APK; enable “install unknown apps” on the phone.
- The APK works with the v2.0.3 hybrid firmware (WiFi+BT) and the v2.0.2
  Bluetooth-only firmware.
- In v2.0.2, firmware updates are USB-only.
