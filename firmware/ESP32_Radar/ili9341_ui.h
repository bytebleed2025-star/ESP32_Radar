#pragma once

/*
 * v2.1 ILI9341 2.4" 240x320 SPI display with XPT2046 touch, used as
 * 320x240 landscape.
 *
 * Pins: SCLK 14, MOSI 13, MISO 17, CS 27, DC 26, RST 25, BL 32,
 *       touch CS 16.
 * Backlight must be driven through a transistor/MOSFET; do not source the
 * 2.4" backlight current directly from GPIO 32.
 *
 * The HC-SR04 and ESP32 are mounted on the freely rotating unit. There is no
 * servo and no angle feedback; the TFT shows distance, pass count and history.
 */

#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED 1
#define ILI9341_DRIVER 1
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
#define USE_HSPI_PORT 1
#define LOAD_GLCD 1
#define LOAD_FONT1 1
#define LOAD_FONT2 1
#define LOAD_FONT4 1
#define LOAD_FONT6 1
#define LOAD_FONT7 1
#define SPI_FREQUENCY 40000000
#define TFT_RGB_ORDER TFT_BGR
#define TOUCH_CS 16
#define SPI_TOUCH_FREQUENCY 2500000
#endif

#include <TFT_eSPI.h>
#include <Preferences.h>

#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240
#define DISPLAY_ROTATION 1

namespace DisplayUI {

static TFT_eSPI tft;
static Preferences prefs;

static const int HISTORY_LEN = 46;

static uint16_t g_cal[5] = {300, 3600, 300, 3600, 3};
static bool     g_calSaved = false;

static int      g_dist = 0;
static int      g_maxRange = 200;
static int      g_alertCm = 50;
static uint32_t g_count = 0;
static bool     g_audio = true;

static int      g_lastDist = -1;
static uint32_t g_lastCount = 0xFFFFFFFF;
static bool     g_lastDetect = false;
static bool     g_lastAudio = true;
static uint32_t g_nextMs = 0;
static uint32_t g_nextTouchMs = 0;
static bool     g_touchDown = false;

static uint8_t  g_hist[HISTORY_LEN];
static uint8_t  g_histIdx = 0;

static void (*g_onReset)() = nullptr;
static void (*g_onAudio)() = nullptr;

static const uint16_t C_BG     = 0x0841;
static const uint16_t C_PANEL  = 0x10A2;
static const uint16_t C_EDGE   = 0x2A69;
static const uint16_t C_GREEN  = 0x5FE4;
static const uint16_t C_TEXT   = 0xEF7D;
static const uint16_t C_DIM    = 0x7BEF;
static const uint16_t C_AMBER  = 0xFD20;
static const uint16_t C_BUTTON = 0x1A45;

static bool detected() {
  return g_dist > 0 && g_dist <= g_alertCm;
}

static void drawPanel(int x, int y, int w, int h) {
  tft.fillRoundRect(x, y, w, h, 6, C_PANEL);
  tft.drawRoundRect(x, y, w, h, 6, C_EDGE);
}

static void drawButton(int x, int y, int w, int h, const char *label,
                       bool active) {
  uint16_t fill = active ? 0x1F6B : C_BUTTON;
  uint16_t edge = active ? C_GREEN : C_EDGE;
  uint16_t text = active ? C_TEXT : C_DIM;
  tft.fillRoundRect(x, y, w, h, 6, fill);
  tft.drawRoundRect(x, y, w, h, 6, edge);
  tft.setTextDatum(MC_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(text, fill);
  tft.drawString(label, x + w / 2, y + h / 2);
  tft.setTextDatum(TL_DATUM);
}

static void drawButtons() {
  drawButton(8, 198, 96, 34, "RESET", true);
  drawButton(112, 198, 96, 34, g_audio ? "AUDIO ON" : "AUDIO OFF", g_audio);
  drawButton(216, 198, 96, 34, "CAL", false);
}

static void drawHistory() {
  int x0 = 170, y0 = 126, w = 138, h = 32;
  tft.fillRect(x0, y0, w, h, C_PANEL);
  int denom = constrain(g_maxRange, 40, 400);
  for (int i = 0; i < HISTORY_LEN; i++) {
    uint8_t v = g_hist[(g_histIdx + i) % HISTORY_LEN];
    int bh = (int)((long)constrain(v, 0, denom) * (h - 2) / denom);
    if (bh < 1) bh = 1;
    uint16_t col = (v > 0 && v <= g_alertCm) ? C_AMBER : C_GREEN;
    int x = x0 + i * 3;
    tft.drawFastVLine(x, y0 + h - bh, bh, col);
  }
}

static void drawValues(bool force) {
  bool det = detected();

  if (force || g_dist != g_lastDist || det != g_lastDetect) {
    tft.fillRect(10, 48, 146, 54, C_PANEL);
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(7);
    tft.setTextColor(det ? C_AMBER : C_TEXT, C_PANEL);
    tft.drawString(String(g_dist), 14, 50);
    tft.setTextFont(2);
    tft.setTextColor(C_DIM, C_PANEL);
    tft.drawString("cm", 124, 84);

    tft.fillRect(10, 116, 146, 46, C_PANEL);
    tft.setTextFont(2);
    tft.setTextColor(det ? C_AMBER : C_GREEN, C_PANEL);
    tft.drawString(det ? "AIR TERDETEKSI" : "AREA BERSIH", 12, 118);
    tft.setTextFont(1);
    tft.setTextColor(C_DIM, C_PANEL);
    tft.drawString(String("MAX ") + g_maxRange + " cm", 12, 144);

    g_lastDist = g_dist;
    g_lastDetect = det;
  }

  if (force || g_count != g_lastCount) {
    tft.fillRect(168, 48, 144, 48, C_PANEL);
    tft.setTextDatum(TR_DATUM);
    tft.setTextFont(6);
    tft.setTextColor(C_GREEN, C_PANEL);
    tft.drawString(String(g_count), 308, 48);
    tft.setTextDatum(TL_DATUM);
    g_lastCount = g_count;
  }

  if (force || g_audio != g_lastAudio) {
    drawButtons();
    g_lastAudio = g_audio;
  }
}

static void drawStatic() {
  tft.fillScreen(C_BG);

  tft.fillRect(0, 0, DISPLAY_WIDTH, 28, C_PANEL);
  tft.drawFastHLine(0, 28, DISPLAY_WIDTH, C_EDGE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.drawString("DETEKTOR AIR", 8, 1);
  tft.setTextFont(2);
  tft.setTextColor(C_GREEN, C_PANEL);
  tft.drawString("v2.1", 276, 8);

  drawPanel(8, 34, 150, 74);
  tft.setTextFont(2);
  tft.setTextColor(C_DIM, C_PANEL);
  tft.drawString("JARAK", 12, 36);

  drawPanel(8, 114, 150, 50);

  drawPanel(166, 34, 146, 74);
  tft.setTextFont(2);
  tft.setTextColor(C_DIM, C_PANEL);
  tft.drawString("PASSES", 170, 36);
  tft.setTextFont(1);
  tft.setTextColor(C_GREEN, C_PANEL);
  tft.drawString("INDUCTIVE PULSE", 170, 96);

  drawPanel(166, 114, 146, 50);
  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_PANEL);
  tft.drawString("HISTORY", 170, 116);

  drawButtons();

  drawHistory();
  drawValues(true);
}

static void pushHistory() {
  g_hist[g_histIdx] = (uint8_t)constrain(g_dist, 0, 255);
  g_histIdx = (g_histIdx + 1) % HISTORY_LEN;
}

static bool waitForTouch(uint32_t timeoutMs) {
  uint32_t start = millis();
  uint16_t x, y;
  while ((uint32_t)(millis() - start) < timeoutMs) {
    if (tft.getTouchRaw(&x, &y)) return true;
    delay(20);
  }
  return false;
}

static void calibrate() {
  tft.fillScreen(C_BG);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(4);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("TOUCH CALIBRATION", 16, 78);
  tft.setTextFont(2);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("Tap each crosshair", 16, 116);
  delay(1000);

  tft.calibrateTouch(g_cal, C_GREEN, C_BG, 15);
  prefs.putBytes("tcal", g_cal, sizeof(g_cal));
  tft.setTouch(g_cal);
  g_calSaved = true;

  drawStatic();
}

static void handleTouch(uint16_t x, uint16_t y, uint32_t now) {
  if ((uint32_t)(now - g_nextTouchMs) < 300) return;
  if (y < 198 || y > 232) return;

  if (x >= 8 && x <= 104) {
    g_nextTouchMs = now;
    if (g_onReset) g_onReset();
  } else if (x >= 112 && x <= 208) {
    g_nextTouchMs = now;
    if (g_onAudio) g_onAudio();
  } else if (x >= 216 && x <= 312) {
    g_nextTouchMs = now;
    calibrate();
  }
}

inline void begin() {
  tft.init();
  tft.setRotation(DISPLAY_ROTATION);
  tft.fillScreen(C_BG);

  prefs.begin("lacak", false);
  g_calSaved = (prefs.getBytesLength("tcal") == sizeof(g_cal));
  if (g_calSaved) prefs.getBytes("tcal", g_cal, sizeof(g_cal));
  tft.setTouch(g_cal);

  if (!g_calSaved) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextFont(4);
    tft.setTextColor(C_TEXT, C_BG);
    tft.drawString("TOUCH TO CALIBRATE", 18, 82);
    tft.setTextFont(2);
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString("or wait 6 s to skip", 18, 120);
    if (waitForTouch(6000)) {
      calibrate();
    }
  }

  drawStatic();
}

inline void setConfig(int maxRange, int alertCm) {
  g_maxRange = constrain(maxRange, 40, 400);
  g_alertCm = constrain(alertCm, 10, 150);
  g_lastDist = -1;
  g_lastCount = 0xFFFFFFFF;
}

inline void setAudioEnabled(bool enabled) {
  g_audio = enabled;
}

inline void setCallbacks(void (*onReset)(), void (*onAudio)()) {
  g_onReset = onReset;
  g_onAudio = onAudio;
}

inline void addReading(int distance) {
  g_dist = distance > 0 ? distance : 0;
}

inline void setProximity(uint32_t count) {
  g_count = count;
}

inline void loop() {
  uint32_t now = millis();

  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    if (!g_touchDown) {
      g_touchDown = true;
      handleTouch(x, y, now);
    }
  } else {
    g_touchDown = false;
  }

  if ((int32_t)(now - g_nextMs) >= 0) {
    g_nextMs = now + 250;
    pushHistory();
    drawHistory();
  }

  drawValues(false);
}

}  // namespace DisplayUI
