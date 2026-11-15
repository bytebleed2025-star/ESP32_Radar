# Drivers

## CP210x USB-to-UART (Windows)

`CP210x-driver.zip` is the Silicon Labs CP210x VCP driver archive used for the
ESP32 dev board's USB-to-serial bridge (VID `0x10C4`, PID `0xEA60`).

Install it if Windows Device Manager shows the board as an unknown device or
does not expose a `COMx` port.

Official source:
<https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers>

After installing:

1. Reconnect the board.
2. Confirm `Silicon Labs CP210x USB to UART Bridge (COMx)` in Device Manager.
3. Use that COM port with esptool / esptool-js.
