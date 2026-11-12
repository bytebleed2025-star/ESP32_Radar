#pragma once

/*
 * v2.0 ST7735 160x128 local display (landscape, no touch).
 *
 * SCLK 14, MOSI 13, CS 27, DC 26, RST 25, BL 32
 * The HC-SR04 is fixed-direction; there is no servo.
 */

#include <Arduino.h>

#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED 1
#define ST7735_DRIVER 1
#define TFT_WIDTH  128
#define TFT_HEIGHT 160
#define ST7735_INITB 1
#define TFT_MISO -1
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   27
#define TFT_DC   26
#define TFT_RST  25
#define TFT_BL   32
#define TFT_BACKLIGHT_ON HIGH
#define USE_HSPI_PORT 1
#define LOAD_GLCD 1
#define LOAD_FONT1 1
#define LOAD_FONT2 1
#define LOAD_FONT4 1
#define SPI_FREQUENCY 27000000
#define TFT_RGB_ORDER TFT_BGR
#endif

#include <TFT_eSPI.h>

#include "st7735_logo.h"
#include "st7735_drop_off.h"
#include "st7735_drop_on.h"

#define ST7735_WIDTH  160
#define ST7735_HEIGHT 128
#define ST7735_ROTATION 1

#define ST_ICON_X 70
#define ST_ICON_Y 32
#define ST_ICON_W 80
#define ST_ICON_H 82

namespace St7735UI {

static TFT_eSPI tft;

static int      g_dist = 0;
static int      g_maxRange = 200;
static int      g_alertCm = 50;
static uint32_t g_count = 0;
static bool     g_filled = true;   // force the first icon draw
static int      g_lastShownDist = -1;
static uint32_t g_lastShownCount = 0xFFFFFFFF;
static bool     g_lastShownDetect = false;
static uint32_t g_nextMs = 0;

static const uint16_t C_BG    = 0x0841;
static const uint16_t C_GREEN = 0x5FE4;
static const uint16_t C_TEXT  = 0xEFFF;
static const uint16_t C_DIM   = 0x3A8C;
static const uint16_t C_AMBER = 0xFE0B;

static bool objectSeen() {
  return g_dist > 0;
}

static bool waterAlert() {
  return g_dist > 0 && g_dist <= g_alertCm;
}

static void drawIcon() {
  bool on = objectSeen();
  if (on == g_filled) return;
  g_filled = on;
  tft.pushImage(ST_ICON_X, ST_ICON_Y, ST_ICON_W, ST_ICON_H,
                on ? ST7735_DROP_ON : ST7735_DROP_OFF);
}

static void drawText() {
  bool on = waterAlert();

  tft.fillRect(0, 0, ST7735_WIDTH, 20, C_BG);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("DETEKTOR AIR", 4, 2);

  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("P", 132, 2);
  tft.setTextColor(C_GREEN, C_BG);
  tft.drawString(String(g_count), 142, 2);

  tft.fillRect(0, 20, 62, 52, C_BG);
  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("DIST", 4, 21);
  tft.setTextFont(4);
  tft.setTextColor(on ? C_AMBER : C_TEXT, C_BG);
  tft.drawString(String(g_dist), 4, 29);
  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("cm", 4, 58);

  tft.fillRect(0, 116, ST7735_WIDTH, 12, C_BG);
  tft.setTextFont(1);
  tft.setTextColor(on ? C_AMBER : C_GREEN, C_BG);
  tft.drawString(on ? "AIR TERDETEKSI" : "AREA BERSIH", 4, 117);

  tft.setTextColor(C_DIM, C_BG);
  tft.drawString(String("MAX ") + g_maxRange, 116, 117);
}

inline void begin() {
  tft.init();
  tft.setRotation(ST7735_ROTATION);
  tft.fillScreen(C_BG);

  tft.pushImage(0, 0, ST7735_WIDTH, ST7735_HEIGHT, ST7735_BOOT_LOGO);
  delay(1500);
  tft.fillScreen(C_BG);

  drawIcon();
  drawText();
  g_lastShownDist = g_dist;
  g_lastShownCount = g_count;
  g_lastShownDetect = objectSeen();
}

inline void setConfig(int maxRange, int alertCm) {
  g_maxRange = constrain(maxRange, 40, 400);
  g_alertCm = constrain(alertCm, 10, 150);
  g_lastShownDist = -1;
  g_lastShownCount = 0xFFFFFFFF;
}

inline void addReading(int distance) {
  g_dist = distance > 0 ? distance : 0;
}

inline void setProximity(uint32_t count) {
  g_count = count;
}

inline void loop() {
  uint32_t now = millis();
  bool on = objectSeen();
  if (g_dist != g_lastShownDist || g_count != g_lastShownCount ||
      on != g_lastShownDetect) {
    if (on != g_lastShownDetect) drawIcon();
    drawText();
    g_lastShownDist = g_dist;
    g_lastShownCount = g_count;
    g_lastShownDetect = on;
  }
  if ((int32_t)(now - g_nextMs) >= 0) {
    g_nextMs = now + 250;
    drawIcon();
  }
}

}  // namespace St7735UI
