#pragma once

/*
 * ST7735 local radar display (no touch).
 *
 * Wiring defaults (change in platformio.ini + here if needed):
 *   SCLK 14, MOSI 13, CS 27, DC 26, RST 25, BL 32
 *   VCC 3.3 V, GND common
 *
 * The ST7735 is display-only. Servo mode, angle, buzzer and OTA remain in
 * the web UI; this screen shows the radar and live readouts.
 */

#include <Arduino.h>

// Arduino IDE builds get the same ST7735 setup from here; PlatformIO supplies
// these through build_flags in platformio.ini.
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

#ifndef ST7735_ENABLED
#define ST7735_ENABLED 1
#endif

#define ST7735_WIDTH   128
#define ST7735_HEIGHT  160
#define ST7735_ROTATION 0

#if ST7735_HEIGHT >= 160
#define ST_RADAR_CY 110
#define ST_RADAR_R  40
#define ST_RADAR_Y  70
#define ST_RADAR_BOTTOM 151
#else
#define ST_RADAR_CY 92
#define ST_RADAR_R  26
#define ST_RADAR_Y  64
#define ST_RADAR_BOTTOM 119
#endif

#define ST_BLIP_COUNT 48
#define ST_BLIP_LIFE  8000UL

#if ST7735_ENABLED

namespace St7735UI {

static TFT_eSPI tft;

struct Blip {
  uint8_t angle;
  uint16_t dist;
  uint32_t t;
};
static Blip g_blips[ST_BLIP_COUNT];
static uint8_t g_blipHead = 0;

static int      g_angle = 0;
static int      g_dist = 0;
static int      g_near = 0;
static int      g_maxRange = 200;
static int      g_alertCm = 50;
static bool     g_paused = false;
static bool     g_hand = false;
static int      g_target = 90;
static int      g_step = 2;
static bool     g_buzzer = true;
static bool     g_dirty = true;
static uint32_t g_nextDrawMs = 0;
static uint32_t g_nextTextMs = 0;

// ---------------------------------------------------------------------------
//  Colours and small drawing helpers
// ---------------------------------------------------------------------------
static const uint16_t C_BG      = 0x0841;  // #08130D
static const uint16_t C_PANEL   = 0x18E3;  // #10271A
static const uint16_t C_GREEN   = 0x5FE4;  // #58FFA1
static const uint16_t C_DIM     = 0x3A8C;  // #1E7A4C
static const uint16_t C_TEXT    = 0xEFFF;  // #E6FFF0
static const uint16_t C_AMBER   = 0xFE0B;  // #FFC857

static int nearestCm(uint32_t now) {
  int best = 0;
  for (uint8_t i = 0; i < ST_BLIP_COUNT; i++) {
    const Blip &b = g_blips[i];
    if (b.dist == 0) continue;
    if ((uint32_t)(now - b.t) > ST_BLIP_LIFE) continue;
    if (best == 0 || b.dist < best) best = b.dist;
  }
  return best;
}

// ---------------------------------------------------------------------------
//  Radar
// ---------------------------------------------------------------------------
static void drawRadar() {
  uint32_t now = millis();

  tft.fillRect(0, ST_RADAR_Y, ST7735_WIDTH, ST_RADAR_BOTTOM - ST_RADAR_Y, TFT_BLACK);

  int cx = ST7735_WIDTH / 2;
  int cy = ST_RADAR_CY;
  int r  = ST_RADAR_R;

  for (int i = 1; i <= 4; i++) {
    tft.drawCircle(cx, cy, r * i / 4, (i == 4) ? C_DIM : 0x10A2);
  }

  tft.drawLine(cx - r, cy, cx + r, cy, C_DIM);
  tft.drawLine(cx, cy - r, cx, cy + r, C_DIM);

  for (int a = 30; a < 180; a += 30) {
    float rad = (180.0f + a) * PI / 180.0f;
    int x1 = cx + (int)(cosf(rad) * r);
    int y1 = cy + (int)(sinf(rad) * r);
    int x2 = cx + (int)(cosf(rad) * (r - 4));
    int y2 = cy + (int)(sinf(rad) * (r - 4));
    tft.drawLine(x1, y1, x2, y2, C_DIM);
  }

  for (uint8_t i = 0; i < ST_BLIP_COUNT; i++) {
    const Blip &b = g_blips[i];
    if (b.dist == 0) continue;
    if ((uint32_t)(now - b.t) > ST_BLIP_LIFE) continue;

    float rr = (float)b.dist / (float)max(1, g_maxRange) * r;
    if (rr > r) rr = r;
    float rad = (180.0f + b.angle) * PI / 180.0f;
    int x = cx + (int)(cosf(rad) * rr);
    int y = cy + (int)(sinf(rad) * rr);
    uint16_t col = (b.dist <= g_alertCm) ? C_AMBER : C_GREEN;
    tft.fillCircle(x, y, (b.dist <= g_alertCm) ? 2 : 1, col);
  }

  float srad = (180.0f + g_angle) * PI / 180.0f;
  int sx = cx + (int)(cosf(srad) * r);
  int sy = cy + (int)(sinf(srad) * r);
  tft.drawLine(cx, cy, sx, sy, C_GREEN);
  tft.fillCircle(cx, cy, 2, C_GREEN);
}

// ---------------------------------------------------------------------------
//  Text
// ---------------------------------------------------------------------------
static void drawHeader() {
  tft.fillRect(0, 0, ST7735_WIDTH, 20, C_BG);
  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(2);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("DETEKTOR AIR", 4, 2);

  tft.setTextFont(1);
  tft.setTextColor(g_hand ? C_AMBER : C_GREEN, C_BG);
  tft.drawString(g_hand ? "HAND" : "AUTO", 100, 5);
}

static void drawData() {
  tft.fillRect(0, 20, ST7735_WIDTH, 48, C_BG);

  tft.setTextDatum(TL_DATUM);
  tft.setTextFont(1);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("DIST", 4, 22);
  tft.drawString("ANGLE", 46, 22);
  tft.drawString("NEAR", 88, 22);

  tft.setTextFont(2);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString(String(g_dist), 4, 31);
  tft.drawString(String(g_angle), 46, 31);
  tft.drawString(String(g_near), 88, 31);

  bool water = (g_near > 0 && g_near <= g_alertCm);
  tft.setTextFont(1);
  tft.setTextColor(water ? C_AMBER : C_GREEN, C_BG);
  tft.drawString(water ? "AIR TERDETEKSI" : "AREA BERSIH", 4, 50);
}

static void drawFooter() {
  tft.fillRect(0, ST7735_HEIGHT - 9, ST7735_WIDTH, 9, C_BG);
  tft.setTextFont(1);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString(String("R ") + g_maxRange + " S " + g_step +
                 (g_paused ? " STOP" : ""), 4, ST7735_HEIGHT - 9);
}

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------
inline void begin() {
  tft.init();
  tft.setRotation(ST7735_ROTATION);
  tft.fillScreen(C_BG);
  tft.setTextWrap(false);
  drawHeader();
  drawData();
  drawFooter();
  g_dirty = true;
}

inline void addReading(int angle, int distance) {
  g_angle = constrain(angle, 0, 180);
  if (distance > 0) {
    g_dist = distance;
    Blip &b = g_blips[g_blipHead];
    b.angle = (uint8_t)g_angle;
    b.dist = (uint16_t)constrain(distance, 0, 2000);
    b.t = millis();
    g_blipHead = (g_blipHead + 1) % ST_BLIP_COUNT;
  }
  g_dirty = true;
}

inline void setConfig(int maxRange, bool paused, int alertCm,
                      bool hand, int target, int step, bool buzzer) {
  g_maxRange = constrain(maxRange, 40, 400);
  g_paused = paused;
  g_alertCm = constrain(alertCm, 10, 150);
  g_hand = hand;
  g_target = constrain(target, 0, 180);
  g_step = constrain(step, 1, 5);
  g_buzzer = buzzer;
  g_dirty = true;
}

inline void setNetwork(const char *, const char *, const char *, int,
                       unsigned long, const char *) {
  // The small ST7735 page is radar-only; network details remain in the web UI.
}

inline void loop() {
  uint32_t now = millis();
  int near = nearestCm(now);
  if (near != g_near) {
    g_near = near;
    g_dirty = true;
  }

  if ((int32_t)(now - g_nextTextMs) >= 0) {
    g_nextTextMs = now + 250;
    if (g_dirty) {
      drawHeader();
      drawData();
      drawFooter();
      g_dirty = false;
    }
  }

  if ((int32_t)(now - g_nextDrawMs) >= 0) {
    g_nextDrawMs = now + 120;   // ~8 FPS PPI
    drawRadar();
  }
}

}  // namespace St7735UI

#else  // ST7735_ENABLED

namespace St7735UI {
inline void begin() {}
inline void addReading(int, int) {}
inline void setConfig(int, bool, int, bool, int, int, bool) {}
inline void setNetwork(const char *, const char *, const char *, int,
                       unsigned long, const char *) {}
inline void loop() {}
}

#endif
