# Firmware images

All downloadable ESP32 images are kept under `bin/`. Pick the folder that
matches your build.

## v2.0.1 — ST7735 + buzzer (current ST7735 build)

File: [`v2.0.1/`](v2.0.1/)

| File | Use |
|---|---|
| [`v2.0.1/DetektorAir-v2.0.1-ST7735-factory.bin`](v2.0.1/DetektorAir-v2.0.1-ST7735-factory.bin) | blank / new board, flash at **0x0** |
| [`v2.0.1/DetektorAir-v2.0.1-ST7735-ota.bin`](v2.0.1/DetektorAir-v2.0.1-ST7735-ota.bin) | web OTA or ArduinoOTA |

```cmd
:: blank board over USB
esptool.py --chip esp32 --port COM5 write_flash 0x0 bin\v2.0.1\DetektorAir-v2.0.1-ST7735-factory.bin
```

Web OTA: **CONFIG → UPDATE FIRMWARE – OTA** → choose
`DetektorAir-v2.0.1-ST7735-ota.bin`.

Checksums: [`v2.0.1/SHA256SUMS.txt`](v2.0.1/SHA256SUMS.txt).

Features: ST7735 1.8" fixed-direction sonar, inductive pass counter, GPIO23
buzzer with long idle / short alert patterns adjustable in the web UI, web OTA.
No MAX98357A and no MP3/WAV.

### v2.0.1 display init variants

If the ST7735 stays **white** with the default image, try these OTA files. They
are identical to v2.0.1 except for the ST7735 init sequence and a lower
**15 MHz** SPI clock (safer on breadboard wiring).

| File | Init |
|---|---|
| [`v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-initb-15mhz-ota.bin`](v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-initb-15mhz-ota.bin) | `ST7735_INITB` |
| [`v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-redtab-15mhz-ota.bin`](v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-redtab-15mhz-ota.bin) | `ST7735_REDTAB` |
| [`v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-greentab-15mhz-ota.bin`](v2.0.1/variants/DetektorAir-v2.0.1-ST7735S-greentab-15mhz-ota.bin) | `ST7735_GREENTAB` |

Upload them through the web OTA page. Try INITB first, then REDTAB, then
GREENTAB.

> Do **not** upload the `-factory.bin` through the web OTA page. It is a merged
> image for offset `0x0`, not an application image.

## v2.0 — ST7735, fixed-direction (previous)

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
