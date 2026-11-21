# ILI9341 2.4" (240×320) build

Same hybrid firmware as `v2.0.3` — WiFi AP+STA, web UI/OTA and Bluetooth
SPP — with the mirror radar screen laid out for a **2.4" ILI9341 SPI** panel.

## Download

| File | Use |
|---|---|
| [`DetektorAir-v2.0.3-ILI9341-WiFi-BT-factory.bin`](DetektorAir-v2.0.3-ILI9341-WiFi-BT-factory.bin) | flash at **0x0** |
| [`DetektorAir-v2.0.3-ILI9341-WiFi-BT-ota.bin`](DetektorAir-v2.0.3-ILI9341-WiFi-BT-ota.bin) | web OTA only |

Checksums: [`SHA256SUMS.txt`](SHA256SUMS.txt)

```cmd
esptool.py --chip esp32 --port COM5 erase_flash
esptool.py --chip esp32 --port COM5 write_flash 0x0 bin\ili9341\DetektorAir-v2.0.3-ILI9341-WiFi-BT-factory.bin
```

## Wiring (SPI ILI9341 module)

| ILI9341 pin | ESP32 |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| CS | GPIO27 |
| RESET | GPIO25 |
| DC / RS | GPIO26 |
| MOSI / SDI | GPIO13 |
| SCK / CLK | GPIO14 |
| MISO / SDO | GPIO12 (optional) |
| LED / BL | GPIO32 (or 3.3 V) |

The HC-SR04, buzzer, SN04 and ST7735 pins are unchanged, so the rest of the
harness stays as it is.

## Build

```bash
pio run -e esp32dev-ili9341
```

- 10 MHz SPI (a full 320×240 frame at 1 MHz would take >1 s).
- Font 6 is used for the large value/count, font 2 for labels.
- If the colours are swapped, add `-DTFT_RGB_ORDER=TFT_BGR` to the env.
- If the panel is inverted, add `-DTFT_INVERSION_ON`.

## Important: Arduino UNO "TFT LCD SHIELD" is different

The red **2.4" TFT LCD SHIELD** (UNO footprint, microSD, touch) uses an
**8-bit parallel** bus, not SPI. It also has 5 V level-shifting buffers.

Driving that shield from an ESP32 is possible with `TFT_PARALLEL_8_BIT`, but it
needs:

- 8 data pins + `WR`, `RD`, `CS`, `DC`, `RST`, `BL` = **14 GPIOs**
- a full pin reassignment of the HC-SR04, buzzer and SN04
- 5 V supply for the shield; 3.3 V ESP32 outputs into 5 V HC245 buffers are
  marginal (VIH ≈ 3.15 V)
- touch and microSD are not used by this firmware

For a simple build, use the **SPI** ILI9341 module wired as above.
