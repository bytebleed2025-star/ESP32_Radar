# DetektorAir Android APK (Bluetooth)

The APK connects to the ESP32 over **Bluetooth Classic SPP** and hosts the radar
UI locally in a WebView. No WiFi, hotspot, HTTP or WebSocket is used.

## Pairing

1. Power the ESP32.
2. Android: **Settings → Bluetooth → Pair new device → DetektorAir**.
3. PIN: **1234**.
4. Install and open `DetektorAir-Bluetooth-v2.0.2.apk`.
5. Select `DetektorAir` and tap **HUBUNGKAN**.

Press **Back** while the radar is shown to disconnect and return to the device
list.

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
bin/v2.0.2/DetektorAir-Bluetooth-v2.0.2.apk
```

## How it works

- `MainActivity` lists paired Bluetooth devices.
- It opens an RFCOMM socket to the SPP UUID
  `00001101-0000-1000-8000-00805F9B34FB`.
- The ESP32 sends newline-delimited JSON; the app forwards each line into the
  WebView via `window.__btDeliver(...)`.
- The WebView's WebSocket is replaced by a Bluetooth transport shim
  (`window.AndroidBT.send(...)`), so the existing radar UI works unchanged.
- The CONFIG tab is hidden in the APK; firmware/network settings are not
  available over Bluetooth.

## Permissions

- Android 12+: `BLUETOOTH_CONNECT` (requested at first launch).
- Android 11 and below: legacy `BLUETOOTH` / `BLUETOOTH_ADMIN`.
- The app only lists **already paired** devices. Pairing is done in Android
  Bluetooth settings.

## Limits

- Bluetooth Classic only; no iOS/BLE support.
- One Bluetooth client at a time.
- Debug-signed APK; enable “install unknown apps” on the phone.
- No OTA; firmware updates are USB-only in v2.0.2.
