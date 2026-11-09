# Detektor Air LACAK — Android APK

A thin **WebView wrapper** for the ESP32 radar web UI. It does not duplicate the
radar UI: the ESP32 serves the normal web app, and the APK opens it in a
full-screen Android WebView with a local connection screen.

The web UI, Config tab, AP+STA, OTA and Nextion work exactly as before.

---

## Built APK

```
DetektorAir-debug.apk
```

The final APK is also copied to the project root by the build:

`C:\Users\fitri\Documents\OPENCODE\ESP32_Radar\DetektorAir-debug.apk`

Current build: **~192 KB**, version **1.1**, debug-signed, `minSdk 24`
(Android 7+), `targetSdk 34`. The connection screen uses the same modern
dark-card theme as the radar UI.

The APK is built with:

- Android SDK Platform 34
- Build-Tools 34.0.0
- Android Gradle Plugin 8.5.2
- Gradle 8.7
- Java 21 (Temurin)
- No AndroidX — platform `Activity`/`WebView` only

The machine’s i7-2600 is too old for the latest Android Studio, so this project
is built with the **command-line SDK tools**, not the IDE.

---

## Build

Prerequisites:

- Android SDK at `%LOCALAPPDATA%\Android\Sdk` (already installed)
- `local.properties` with `sdk.dir` (created automatically by the script)
- Java 21 (`JAVA_HOME` already set)

Run:

```bat
build_apk.bat
```

or:

```bat
gradlew.bat assembleDebug
```

The first wrapper build downloads Gradle 8.7 and the Android Gradle Plugin
dependencies. The output is:

```
app/build/outputs/apk/debug/app-debug.apk
```

`build_apk.bat` copies it to `DetektorAir-debug.apk`.

> An `SDK XML version 4` warning may appear because the 2026 command-line tools
> are newer than AGP 8.5.2. The build still succeeds; this is harmless for the
> CLI build.

---

## Install on the phone

1. On the phone: **Settings → About → tap Build number 7×** to enable
   Developer options.
2. Enable **USB debugging**.
3. Connect the phone by a **data** USB cable and accept the authorization
   prompt.
4. Run:

   ```bat
   install_apk.bat
   ```

Or copy `DetektorAir-debug.apk` to the phone and install it manually (allow
“Install unknown apps” for the file manager).

Check the connection:

```bat
%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe devices
```

No device attached at build time was available to test the APK on this PC.

---

## Using the app

1. Power the ESP32 radar.
2. Join the phone to the **`DetektorAir` hotspot** (password
   `radar12345`), **or** to the same home WiFi as the ESP32.
3. Open **Detektor Air LACAK**.
4. Enter the address:
   - Hotspot: **`192.168.4.1`**
   - Home WiFi: the ESP32 **STA IP** shown in the radar’s Config tab/serial
     monitor
5. Tap **BUKA RADAR**.
6. To return to the connection screen, press **Back**. Press Back again to
   exit.

The address is saved, so the next launch opens with it prefilled.

### What the wrapper supports

- Full-screen WebView, keep-screen-on, immersive UI
- JavaScript, DOM storage, `localStorage` (web UI settings)
- HTTP **and WebSocket `ws://`** on the local network
- WebAudio (sonar ping)
- Android JS dialogs (`alert` / `confirm` / `prompt`) used by OTA/reboot
- **File chooser for web OTA** (`<input type="file">` → pick the `.bin`)
- Back-to-settings, connection error screen, saved IP
- Cleartext HTTP allowed via manifest + network security config (local device
  only)

### Limitations

- The app is only a shell. If the ESP32 web server is not reachable, it shows
  the connection screen with an error.
- Android restricts programmatic WiFi connections; join the hotspot manually
  (the **PENGATURAN WIFI** button helps).
- The APK is debug-signed. To distribute it, create your own release keystore
  and sign a release build.
- Cleartext HTTP/WS is enabled for this local-network app; do not expose the
  device to untrusted networks.
- The app was built and manifest-verified here, but not run on a physical phone
  in this environment.

---

## Troubleshooting

| Problem | Check |
|---|---|
| App cannot connect | Phone on BUKA RADAR’s WiFi? Correct IP? Try `http://192.168.4.1` in Chrome first. |
| LINK stays TERPUTUS in the web UI | WebSocket port 81 blocked / wrong network; same as browser usage. |
| Web OTA file picker doesn’t open | Use the CONFIG tab → choose `.bin`; the app implements the Android file chooser. |
| APK install fails | Android 7+, USB debugging, data cable, accept the RSA prompt, or copy the APK and install manually. |
| Gradle build fails with SDK not found | Check `local.properties` (`sdk.dir=C:/Users/.../Android/Sdk`) and `ANDROID_HOME`. |
| `SDK XML version 4` warning | Harmless with AGP 8.5.2; the build completes. |
| Old page after firmware update | Press Back → BUKA RADAR to reload; the ESP32 serves no-cache HTML. |
