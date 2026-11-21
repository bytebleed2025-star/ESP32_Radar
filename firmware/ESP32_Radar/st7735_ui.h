#pragma once

/*
 * v2.0.3 ST7735 160x128 UI.
 *
 * WelcomeScreen.jpg is shown for 3 seconds, then the RadarScan_00..03
 * backgrounds are selected by distance:
 *   00 = idle, 01 = far, 02 = near, 03 = alert.
 *
 * The white value boxes are part of the artwork; live values are overlaid in
 * FreeSansBold (modern sans).
 */

#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED 1
#define ST7735_DRIVER 1
#define TFT_WIDTH  128
#define TFT_HEIGHT 160
#define ST7735_REDTAB 1
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
#define LOAD_GFXFF 1
#define SPI_FREQUENCY 1000000
#define TFT_RGB_ORDER TFT_BGR
#endif

#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

#include "st7735_images.h"

#define ST7735_WIDTH  160
#define ST7735_HEIGHT 128
#define ST7735_ROTATION 1

// Continuous scan animation: 1 frame per second at 1 MHz.
#define SCAN_ANIMATE   1
#define SCAN_FRAME_MS  1000

namespace St7735UI {

static TFT_eSPI tft;

static int      g_dist = 0;
static int      g_maxRange = 200;
static int      g_alertCm = 50;
static uint32_t g_count = 0;
static int      g_frame = -1;
static int      g_pendingFrame = -1;
static uint32_t g_pendingSince = 0;
static uint32_t g_nextFrameMs = 0;

static int      g_lastDist = -1;
static uint32_t g_lastCount = 0xFFFFFFFF;
static bool     g_lastDetect = false;

static const uint16_t C_GREEN = 0x5FE4;
static const uint16_t C_AMBER = 0xFD20;

static bool detected() {
  return g_dist > 0 && g_dist <= g_alertCm;
}

static bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h,
                       uint16_t *bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

static void showJpg(const uint8_t *data, uint32_t len) {
  TJpgDec.drawJpg(0, 0, data, len);
}

static int frameForDistance() {
  if (g_dist <= 0) return 0;
  if (g_dist > g_alertCm) return 1;
  if (g_dist > g_alertCm / 2) return 2;
  return 3;
}

static void drawValues(bool force) {
  if (force || g_dist != g_lastDist) {
    char value[8];
    snprintf(value, sizeof(value), "%d", g_dist);

    tft.fillRect(126, 23, 27, 16, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(g_dist >= 100 ? &FreeSans9pt7b : &FreeSansBold9pt7b);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString(value, 140, 31);
    tft.setTextDatum(TL_DATUM);

    g_lastDist = g_dist;
  }

  if (force || g_count != g_lastCount) {
    tft.fillRect(126, 83, 27, 16, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(g_count > 999 ? &FreeSans9pt7b : &FreeSansBold9pt7b);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString(String(g_count), 140, 91);
    tft.setTextDatum(TL_DATUM);
    g_lastCount = g_count;
  }
}

static uint32_t g_frames = 0;

static void showFrame(int frame) {
  g_frames++;
  g_frame = frame;
  switch (frame) {
    case 0: showJpg(ST7735_SCAN0_JPG, ST7735_SCAN0_JPG_LEN); break;
    case 1: showJpg(ST7735_SCAN1_JPG, ST7735_SCAN1_JPG_LEN); break;
    case 2: showJpg(ST7735_SCAN2_JPG, ST7735_SCAN2_JPG_LEN); break;
    default: showJpg(ST7735_SCAN3_JPG, ST7735_SCAN3_JPG_LEN); break;
  }
  drawValues(true);
}

inline void begin() {
  tft.init();
  tft.setRotation(ST7735_ROTATION);
  tft.fillScreen(TFT_BLACK);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  // Welcome screen for 3 seconds.
  showJpg(ST7735_WELCOME_JPG, ST7735_WELCOME_JPG_LEN);
  delay(3000);

  g_frame = -1;
  g_pendingFrame = -1;
#if SCAN_ANIMATE
  showFrame(0);
  g_nextFrameMs = millis() + SCAN_FRAME_MS;
#else
  showFrame(frameForDistance());
#endif

  g_lastDist = g_dist;
  g_lastCount = g_count;
  g_lastDetect = detected();
}

inline void setConfig(int maxRange, int alertCm) {
  g_maxRange = constrain(maxRange, 40, 400);
  g_alertCm = constrain(alertCm, 10, 150);
  g_pendingFrame = -1;
  g_lastDist = -1;
  g_lastCount = 0xFFFFFFFF;
}

inline void addReading(int distance, int angle) {
  (void)angle;
  g_dist = distance > 0 ? distance : 0;
}

inline void setProximity(uint32_t count) {
  g_count = count;
}

inline uint32_t frames() {
  return g_frames;
}

inline void loop() {
#if SCAN_ANIMATE
  uint32_t now = millis();
  if ((int32_t)(now - g_nextFrameMs) >= 0) {
    g_nextFrameMs = now + SCAN_FRAME_MS;
    showFrame((g_frame + 1) & 3);
  }
#else
  int frame = frameForDistance();
  if (frame != g_frame) {
    if (frame != g_pendingFrame) {
      g_pendingFrame = frame;
      g_pendingSince = millis();
    } else if ((uint32_t)(millis() - g_pendingSince) >= 400) {
      showFrame(frame);
      g_pendingFrame = -1;
    }
  } else {
    g_pendingFrame = -1;
  }
#endif

  bool det = detected();
  if (g_dist != g_lastDist || g_count != g_lastCount || det != g_lastDetect) {
    drawValues(false);
    g_lastDist = g_dist;
    g_lastCount = g_count;
    g_lastDetect = det;
  }
}

}  // namespace St7735UI
