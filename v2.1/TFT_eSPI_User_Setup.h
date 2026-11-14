// TFT_eSPI setup for the v2.1 ILI9341 2.4" touch display.
//
// Arduino IDE:
//   1. Copy this file to the TFT_eSPI library folder as `User_Setup.h`
//      (replace the existing file, keep a backup).
//   2. Compile the sketch for "ESP32 Dev Module" with
//      "Minimal SPIFFS (1.9MB APP with OTA)".
//
// PlatformIO does not need this file; the same defines are in platformio.ini.

#define USER_SETUP_ID 210

#define ILI9341_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MISO 17
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   27
#define TFT_DC   26
#define TFT_RST  25
#define TFT_BL   32
#define TFT_BACKLIGHT_ON LOW

#define USE_HSPI_PORT

#define LOAD_GLCD
#define LOAD_FONT1
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7

#define SPI_FREQUENCY 40000000
#define TFT_RGB_ORDER TFT_BGR

#define TOUCH_CS 16
#define SPI_TOUCH_FREQUENCY 2500000
