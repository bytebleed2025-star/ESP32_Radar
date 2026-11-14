# Firmware images

All downloadable ESP32 images are kept under `bin/`. Pick the folder that
matches your build.

## v2.1 — ILI9341 2.4" touch (current)

File: [`v2.1/`](v2.1/)

| File | Use |
|---|---|
| [`v2.1/DetektorAir-v2.1-ILI9341-factory.bin`](v2.1/DetektorAir-v2.1-ILI9341-factory.bin) | blank / new board, flash at **0x0** |
| [`v2.1/DetektorAir-v2.1-ILI9341-ota.bin`](v2.1/DetektorAir-v2.1-ILI9341-ota.bin) | web OTA or ArduinoOTA |

```cmd
:: blank board over USB
esptool.py --chip esp32 --port COM5 write_flash 0x0 bin\v2.1\DetektorAir-v2.1-ILI9341-factory.bin
```

Web OTA: **CONFIG → UPDATE FIRMWARE – OTA** → choose
`DetektorAir-v2.1-ILI9341-ota.bin`.

Checksums: [`v2.1/SHA256SUMS.txt`](v2.1/SHA256SUMS.txt).

> Do **not** upload the `-factory.bin` through the web OTA page. It is a merged
> image for offset `0x0`, not an application image.

Features: 2.4" ILI9341 320×240 landscape, touch config buttons, freely rotating
HC-SR04 head, inductive pass counter, MAX98357A MP3/WAV, web UI and OTA.

## v2.0 — ST7735, fixed-direction

File: [`v2.0/`](v2.0/)

| File | Use |
|---|---|
| [`v2.0/DetektorAir-v2.0-ST7735-factory.bin`](v2.0/DetektorAir-v2.0-ST7735-factory.bin) | blank / new board, flash at **0x0** |
| [`v2.0/DetektorAir-v2.0-ST7735-ota.bin`](v2.0/DetektorAir-v2.0-ST7735-ota.bin) | web OTA or ArduinoOTA |

```cmd
:: blank board over USB
esptool.py --chip esp32 --port COM5 write_flash 0x0 bin\v2.0\DetektorAir-v2.0-ST7735-factory.bin
```

Web OTA: **CONFIG → UPDATE FIRMWARE – OTA** → choose
`DetektorAir-v2.0-ST7735-ota.bin`.

> Do **not** upload the `-factory.bin` through the web OTA page. It is a merged
> image for offset `0x0`, not an application image.

### ST7735S init variants

Only use these if the display is blank, shifted, or has wrong colours with the
default image. They contain the same firmware; only the ST7735 init sequence
differs.

| File | Init profile |
|---|---|
| [`v2.0/variants/DetektorAir-v2.0-ST7735S-redtab-ota.bin`](v2.0/variants/DetektorAir-v2.0-ST7735S-redtab-ota.bin) | `ST7735_REDTAB` |
| [`v2.0/variants/DetektorAir-v2.0-ST7735S-greentab-ota.bin`](v2.0/variants/DetektorAir-v2.0-ST7735S-greentab-ota.bin) | `ST7735_GREENTAB` |

Upload them through the web OTA page exactly like the normal `-ota.bin`.

Checksums: [`v2.0/SHA256SUMS.txt`](v2.0/SHA256SUMS.txt) and
[`v2.0/variants/SHA256SUMS.txt`](v2.0/variants/SHA256SUMS.txt).

## Legacy images

File: [`legacy/`](legacy/)

| File | Build |
|---|---|
| `legacy/DetektorAir-Nextion-factory.bin` | Nextion display, servo sweep |
| `legacy/DetektorAir-ota.bin` | Nextion / servo OTA firmware |
| `legacy/DetektorAir-ST7735-factory.bin` | older ST7735 servo build |
| `legacy/DetektorAir-ST7735-ota.bin` | older ST7735 servo OTA |

These do **not** contain the v2.0 fixed-direction logic, MAX98357A alerts, or the
inductive proximity counter.
