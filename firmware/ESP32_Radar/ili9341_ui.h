#pragma once

/*
 * ILI9341 2.4" (240x320) landscape UI for the v2.0.3 hybrid firmware.
 *
 * Same data as the 1.8" ST7735 mirror screen, laid out for 320x240.
 * Selected automatically when the build defines ILI9341_DRIVER
 * (PlatformIO env: esp32dev-ili9341).
 *
 * Namespace is still "St7735UI" so the sketch calls stay unchanged.
 *
 * Pins (same wiring as the ST7735 build):
 *   SCLK 14, MOSI 13, MISO 12, CS 27, DC 26, RST 25, BL 32.
 */

#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED 1
#define ILI9341_DRIVER 1
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
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
#define LOAD_FONT6 1
#define SPI_FREQUENCY 10000000
#endif

#include <TFT_eSPI.h>
#include <math.h>

#define ILI_W 320
#define ILI_H 240
#define ILI_ROTATION 1

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
static uint32_t g_frames = 0;

static const int DIV_X = 210;
static const int CX = 100;
static const int CY = 142;
static const int R  = 84;

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
  g_frames++;
  tft.fillRect(1, 29, DIV_X - 2, ILI_H - 30, C_BG);

  for (int i = 1; i <= 3; i++) {
    tft.drawCircle(CX, CY, R * i / 3, C_EDGE);
  }
  tft.drawFastHLine(CX - R, CY, 2 * R, C_EDGE);
  tft.drawFastVLine(CX, CY - R, 2 * R, C_EDGE);

  tft.setTextFont(2);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("0", 8, 146);
  tft.drawString("90", 92, 40);
  tft.drawString("180", 160, 146);

  for (int i = 3; i >= 1; i--) {
    int a = g_angle - g_dir * i * 6;
    if (a >= 0 && a <= 180) drawBeam(a, C_TRAIL);
  }
  drawBeam(g_angle, C_GREEN);

  if (g_dist > 0) {
    int rr = map(constrain(g_dist, 0, g_maxRange), 0, g_maxRange, 10, R - 6);
    float rad = (180.0f + g_angle) * PI / 180.0f;
    int x = CX + (int)(cosf(rad) * rr);
    int y = CY + (int)(sinf(rad) * rr);
    tft.fillCircle(x, y, 3, detected() ? C_AMBER : C_GREEN);
  }

  tft.fillCircle(CX, CY, 3, C_TEXT);
}

static void drawValues(bool force) {
  bool det = detected();

  if (force || g_dist != g_lastDist || det != g_lastDetect) {
    tft.fillRect(214, 52, 102, 56, C_BG);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(6);
    tft.setTextColor(C_TEXT, C_BG);
    tft.drawString(String(g_dist), 216, 52);

    tft.fillRect(214, 108, 102, 18, C_BG);
    tft.setTextFont(2);
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString("m", 218, 108);

    tft.fillRect(214, 128, 102, 20, C_BG);
    tft.setTextFont(2);
    tft.setTextColor(det ? C_AMBER : C_GREEN, C_BG);
    tft.drawString(det ? "TERDETEKSI" : "AMAN", 218, 130);

    g_lastDist = g_dist;
    g_lastDetect = det;
  }

  if (force || g_count != g_lastCount) {
    tft.fillRect(214, 168, 102, 54, C_BG);
    tft.setTextDatum(TR_DATUM);
    tft.setTextFont(g_count > 999 ? 4 : 6);
    tft.setTextColor(C_TEXT, C_BG);
    tft.drawString(String(g_count), 314, 170);
    tft.setTextDatum(TL_DATUM);
    g_lastCount = g_count;
  }
}

static void drawStatic() {
  tft.fillScreen(C_BG);

  tft.fillRect(0, 0, ILI_W, 28, C_PANEL);
  tft.drawFastHLine(0, 28, ILI_W, C_EDGE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.drawString("LACAK", 6, 2);
  tft.setTextFont(2);
  tft.setTextColor(C_GREEN, C_PANEL);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("v2.0.3-ILI", 314, 8);
  tft.setTextDatum(TL_DATUM);

  tft.drawFastVLine(DIV_X, 28, ILI_H - 28, C_EDGE);

  tft.setTextFont(2);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("KEDALAMAN", 218, 34);
  tft.drawString("PASSES", 218, 154);
  tft.setTextFont(1);
  tft.drawString(String("MAX ") + String(g_maxRange) + "m", 218, 226);

  drawRadar();
  drawValues(true);
}

inline void begin() {
  tft.init();
  tft.setRotation(ILI_ROTATION);
  tft.fillScreen(C_BG);

  // Text splash for 3 seconds (the ST7735 JPEG is too small for 320x240).
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(6);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("LACAK", ILI_W / 2, 96);
  tft.setTextFont(2);
  tft.setTextColor(C_GREEN, C_BG);
  tft.drawString("DETEKTOR AIR", ILI_W / 2, 140);
  delay(3000);

  tft.fillScreen(C_BG);
  tft.setTextDatum(TL_DATUM);
  drawStatic();
  g_lastDist = g_dist;
  g_lastCount = g_count;
  g_lastDetect = detected();
}

inline uint32_t frames() {
  return g_frames;
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
