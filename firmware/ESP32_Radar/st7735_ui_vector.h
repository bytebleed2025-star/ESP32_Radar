#pragma once

/*
 * v2.0.1-style ST7735 160x128 landscape UI for the v2.0.3 hybrid firmware.
 *
 * Animated radar + data column, drawn in code (no JPEG artwork).
 * Default v2.0.3 hybrid screen: it mirrors the web UI live.
 * The JPEG artwork build is selected with -DST7735_ARTWORK_UI=1
 * (PlatformIO env: esp32dev-art).
 *
 * Header: LACAK. Distance uses the unit "m" with the raw sensor value
 * (no cm -> m division).
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
#include <math.h>

#define ST7735_WIDTH  160
#define ST7735_HEIGHT 128
#define ST7735_ROTATION 1

namespace St7735UI {

static TFT_eSPI tft;

static int      g_dist = 0;
static int      g_angle = 90;
static int      g_dir = 1;
static int      g_maxRange = 200;
static int      g_alertCm = 50;
static uint32_t g_count = 0;

static int      g_lastDist = -1;
static uint32_t g_lastCount = 0xFFFFFFFF;
static bool     g_lastDetect = false;
static uint32_t g_nextRadar = 0;
static bool     g_redraw = true;

static const int CX = 48;
static const int CY = 73;
static const int R  = 44;

static const uint16_t C_BG     = 0x0841;
static const uint16_t C_PANEL  = 0x10A2;
static const uint16_t C_EDGE   = 0x2A69;
static const uint16_t C_GREEN  = 0x5FE4;
static const uint16_t C_TRAIL  = 0x1A45;
static const uint16_t C_TEXT   = 0xEF7D;
static const uint16_t C_DIM    = 0x7BEF;
static const uint16_t C_AMBER  = 0xFD20;

static bool detected() {
  return g_dist > 0 && g_dist <= g_alertCm;
}

static void drawBeam(int angle, uint16_t color) {
  float rad = (180.0f + angle) * PI / 180.0f;
  int x = CX + (int)(cosf(rad) * R);
  int y = CY + (int)(sinf(rad) * R);
  tft.drawLine(CX, CY, x, y, color);
}

static void drawRadar() {
  tft.fillRect(1, 20, 94, 106, C_BG);

  for (int i = 1; i <= 3; i++) {
    tft.drawCircle(CX, CY, R * i / 3, C_EDGE);
  }
  tft.drawFastHLine(CX - R, CY, 2 * R, C_EDGE);
  tft.drawFastVLine(CX, CY - R, 2 * R, C_EDGE);

  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("0", 2, 74);
  tft.drawString("90", 44, 21);
  tft.drawString("180", 86, 74);

  for (int i = 3; i >= 1; i--) {
    int a = g_angle - g_dir * i * 6;
    if (a >= 0 && a <= 180) drawBeam(a, C_TRAIL);
  }
  drawBeam(g_angle, C_GREEN);

  if (g_dist > 0) {
    int rr = map(constrain(g_dist, 0, g_maxRange), 0, g_maxRange, 6, R - 4);
    float rad = (180.0f + g_angle) * PI / 180.0f;
    int x = CX + (int)(cosf(rad) * rr);
    int y = CY + (int)(sinf(rad) * rr);
    tft.fillCircle(x, y, 2, detected() ? C_AMBER : C_GREEN);
  }

  tft.fillCircle(CX, CY, 2, C_TEXT);
}

static void drawValues(bool force) {
  bool det = detected();

  if (force || g_dist != g_lastDist || det != g_lastDetect) {
    tft.fillRect(98, 30, 60, 26, C_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(4);
    tft.drawString(String(g_dist), 99, 30);
    tft.setTextFont(1);
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString("m", 99, 56);

    tft.fillRect(98, 66, 60, 10, C_BG);
    tft.setTextFont(1);
    tft.setTextColor(det ? C_AMBER : C_GREEN, C_BG);
    tft.drawString(det ? "TERDETEKSI" : "AMAN", 99, 66);

    g_lastDist = g_dist;
    g_lastDetect = det;
  }

  if (force || g_count != g_lastCount) {
    tft.fillRect(98, 86, 60, 28, C_BG);
    tft.setTextDatum(TR_DATUM);
    if (g_count > 999) {
      tft.setTextFont(2);
      tft.drawString(String(g_count), 156, 94);
    } else {
      tft.setTextFont(4);
      tft.drawString(String(g_count), 156, 86);
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
  tft.drawString("LACAK", 4, 2);
  tft.setTextFont(1);
  tft.setTextColor(C_GREEN, C_PANEL);
  tft.drawString("v2.0.3", 130, 6);

  tft.drawFastVLine(96, 20, 106, C_EDGE);

  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("JARAK", 99, 22);
  tft.drawString("PASSES", 99, 78);
  tft.drawString(String("MAX ") + String(g_maxRange) + "m", 99, 114);

  drawRadar();
  drawValues(true);
}

inline void begin() {
  tft.init();
  tft.setRotation(ST7735_ROTATION);
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

inline void addReading(int distance, int angle) {
  if (angle != g_angle) {
    g_dir = (angle > g_angle) ? 1 : -1;
    g_angle = constrain(angle, 0, 180);
  }
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

  if ((int32_t)(now - g_nextRadar) >= 0) {
    g_nextRadar = now + 200;
    drawRadar();
  }

  bool det = detected();
  if (g_dist != g_lastDist || g_count != g_lastCount || det != g_lastDetect) {
    drawValues(false);
    g_lastDist = g_dist;
    g_lastCount = g_count;
    g_lastDetect = det;
  }
}

}  // namespace St7735UI
