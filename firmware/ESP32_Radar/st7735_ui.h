#pragma once

/*
 * v2.0.1 ST7735 160x128 landscape UI (modern tile layout).
 *
 * SCLK 14, MOSI 13, MISO 12, CS 27, DC 26, RST 25, BL 32.
 * ST7735S panel, GREENTAB init, 1 MHz HSPI (verified on breadboard wiring).
 * The HC-SR04 is fixed-direction; there is no servo.
 */

#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED 1
#define ST7735_DRIVER 1
#define TFT_WIDTH  128
#define TFT_HEIGHT 160
#define ST7735_GREENTAB 1
#define TFT_MISO 12
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
#define SPI_FREQUENCY 1000000
#define TFT_RGB_ORDER TFT_BGR
#endif

#include <TFT_eSPI.h>

#include "st7735_logo.h"

#define ST7735_WIDTH  160
#define ST7735_HEIGHT 128
#define ST7735_ROTATION 1

namespace St7735UI {

static TFT_eSPI tft;

static int      g_dist = 0;
static int      g_maxRange = 200;
static int      g_alertCm = 50;
static uint32_t g_count = 0;
static bool     g_redraw = true;

static int      g_lastDist = -1;
static uint32_t g_lastCount = 0xFFFFFFFF;
static bool     g_lastDetect = false;
static uint32_t g_nextMs = 0;

static const int HISTORY_LEN = 46;
static uint8_t g_hist[HISTORY_LEN];
static uint8_t g_histIdx = 0;

static const uint16_t C_BG     = 0x0841;
static const uint16_t C_PANEL  = 0x10A2;
static const uint16_t C_EDGE   = 0x2A69;
static const uint16_t C_GREEN  = 0x5FE4;
static const uint16_t C_TEXT   = 0xEF7D;
static const uint16_t C_DIM    = 0x7BEF;
static const uint16_t C_AMBER  = 0xFD20;

static bool detected() {
  return g_dist > 0 && g_dist <= g_alertCm;
}

static void drawTile(int x, int y, int w, int h) {
  tft.fillRoundRect(x, y, w, h, 5, C_PANEL);
  tft.drawRoundRect(x, y, w, h, 5, C_EDGE);
  tft.drawFastHLine(x + 6, y + 1, w - 12, C_GREEN);
}

static void drawHistory() {
  int x0 = 8, y0 = 94, h = 12;
  tft.fillRect(x0, y0, 144, h, C_PANEL);
  int denom = constrain(g_maxRange, 40, 400);

  for (int i = 0; i < HISTORY_LEN; i++) {
    uint8_t v = g_hist[(g_histIdx + i) % HISTORY_LEN];
    int bh = (int)((long)constrain(v, 0, denom) * h / denom);
    if (bh < 1) bh = 1;
    uint16_t col = (v > 0 && v <= g_alertCm) ? C_AMBER : C_GREEN;
    tft.drawFastVLine(x0 + i * 3, y0 + h - bh, bh, col);
  }
}

static void drawValues(bool force) {
  bool det = detected();

  if (force || g_dist != g_lastDist || det != g_lastDetect) {
    tft.fillRect(9, 33, 68, 32, C_PANEL);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(4);
    tft.setTextColor(det ? C_AMBER : C_TEXT, C_PANEL);
    tft.drawString(String(g_dist), 9, 33);
    tft.setTextFont(1);
    tft.setTextColor(C_DIM, C_PANEL);
    tft.drawString("cm", 60, 57);

    tft.fillRect(9, 66, 68, 10, C_PANEL);
    tft.setTextFont(1);
    tft.setTextColor(det ? C_AMBER : C_GREEN, C_PANEL);
    tft.drawString(det ? "TERDETEKSI" : "AREA BERSIH", 9, 68);

    g_lastDist = g_dist;
    g_lastDetect = det;
  }

  if (force || g_count != g_lastCount) {
    tft.fillRect(89, 33, 64, 30, C_PANEL);
    tft.setTextDatum(TR_DATUM);
    if (g_count > 999) {
      tft.setTextFont(2);
      tft.drawString(String(g_count), 151, 43);
    } else {
      tft.setTextFont(4);
      tft.drawString(String(g_count), 151, 33);
    }
    tft.setTextDatum(TL_DATUM);
    g_lastCount = g_count;
  }
}

static void drawStatic() {
  tft.fillScreen(C_BG);

  tft.fillRect(0, 0, ST7735_WIDTH, 20, C_PANEL);
  tft.drawFastHLine(0, 20, ST7735_WIDTH, C_EDGE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.drawString("DETEKTOR AIR", 6, 2);
  tft.setTextFont(1);
  tft.setTextColor(C_GREEN, C_PANEL);
  tft.drawString("v2.0.1", 124, 6);

  drawTile(4, 24, 76, 54);
  drawTile(84, 24, 72, 54);

  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_PANEL);
  tft.drawString("JARAK", 9, 27);
  tft.drawString("PASSES", 89, 27);
  tft.setTextColor(C_GREEN, C_PANEL);
  tft.drawString("INDUKTIF", 89, 68);

  tft.fillRoundRect(4, 82, 152, 28, 5, C_PANEL);
  tft.drawRoundRect(4, 82, 152, 28, 5, C_EDGE);
  tft.drawFastHLine(10, 83, 140, C_GREEN);
  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_PANEL);
  tft.drawString("RIWAYAT", 8, 84);

  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_PANEL);
  tft.drawString(String("MAX ") + g_maxRange + " cm", 6, 114);
  tft.setTextColor(C_GREEN, C_PANEL);
  tft.drawString("SONAR", 128, 114);

  drawHistory();
  drawValues(true);
}

inline void begin() {
  tft.init();
  tft.setRotation(ST7735_ROTATION);
  tft.fillScreen(C_BG);

  tft.pushImage(0, 0, ST7735_WIDTH, ST7735_HEIGHT, ST7735_BOOT_LOGO);
  delay(1500);
  tft.fillScreen(C_BG);

  drawStatic();
  g_lastDist = g_dist;
  g_lastCount = g_count;
  g_lastDetect = detected();
}

inline void setConfig(int maxRange, int alertCm) {
  g_maxRange = constrain(maxRange, 40, 400);
  g_alertCm = constrain(alertCm, 10, 150);
  g_redraw = true;
  g_lastDist = -1;
  g_lastCount = 0xFFFFFFFF;
}

inline void addReading(int distance) {
  g_dist = distance > 0 ? distance : 0;
}

inline void setProximity(uint32_t count) {
  g_count = count;
}

inline void loop() {
  uint32_t now = millis();

  if (g_redraw) {
    g_redraw = false;
    drawStatic();
  }

  bool det = detected();
  if (g_dist != g_lastDist || g_count != g_lastCount || det != g_lastDetect) {
    drawValues(false);
    g_lastDist = g_dist;
    g_lastCount = g_count;
    g_lastDetect = det;
  }

  if ((int32_t)(now - g_nextMs) >= 0) {
    g_nextMs = now + 250;
    g_hist[g_histIdx] = (uint8_t)constrain(g_dist, 0, 255);
    g_histIdx = (g_histIdx + 1) % HISTORY_LEN;
    drawHistory();
  }
}

}  // namespace St7735UI
