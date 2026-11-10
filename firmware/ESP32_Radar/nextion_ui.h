#pragma once

/*
 * ============================================================================
 *  Nextion UI for NX3224T028 (2.8" 320x240)  â€”  optional companion display
 * ============================================================================
 *
 *  The web UI keeps working. This module drives the Nextion over UART2 and
 *  also receives touch events (buttons configured with "Send Component ID").
 *
 *  Wiring
 *  ------
 *    Nextion 5V  -> 5V
 *    Nextion GND -> GND (common with ESP32)
 *    Nextion TX  -> level shifter/divider -> GPIO16  (ESP32 RX2)
 *    Nextion RX  -> GPIO17                            (ESP32 TX2)
 *    Set the Nextion baud rate to 115200 (see nextion/README.md).
 *
 *  If your board is an ESP32 WROVER with PSRAM, GPIO16/17 may be reserved.
 *  Change NX_RX_PIN / NX_TX_PIN below to free pins.
 *
 *  The HMI pages and component names are documented in nextion/README.md.
 *  Set NEXTION_ENABLED to 0 in the sketch if you do not use the display.
 * ============================================================================
 */

#include <Arduino.h>
#include <HardwareSerial.h>

#ifndef NEXTION_ENABLED
#define NEXTION_ENABLED 1
#endif

#ifndef NX_RX_PIN
#define NX_RX_PIN 16
#endif
#ifndef NX_TX_PIN
#define NX_TX_PIN 17
#endif
#ifndef NX_BAUD
#define NX_BAUD 115200
#endif

// Alert threshold used on the Nextion screen (the web UI has its own slider).
#ifndef NX_ALERT_CM
#define NX_ALERT_CM 50
#endif

// Nextion page indices (keep this order in the HMI design).
#define NX_PAGE_RADAR   0
#define NX_PAGE_CONTROL 1
#define NX_PAGE_WELCOME 2
#define NX_PAGE_INFO    3

// Touch component IDs for custom component-ID HMIs. The prebuilt HMI
// uses the NX:... text commands instead.
#define NX_ID_INFO   1   // page 0: INFO button
#define NX_ID_BACK   1   // page 3: BACK button
#define NX_ID_R100   2   // page 3
#define NX_ID_R200   3   // page 3
#define NX_ID_R300   4   // page 3
#define NX_ID_PAUSE  5   // page 3

// Radar drawing geometry (must match the empty black area on page 0).
#define NX_RADAR_CX 160
#define NX_RADAR_CY 164
#define NX_RADAR_R  74
#define NX_RADAR_Y  84
#define NX_RADAR_H  156

// RGB565 colours
#define NX_C_BLACK 0
#define NX_C_GREEN 24564   // #58FFA1
#define NX_C_DIM   7113    // #1E7A4C
#define NX_C_AMBER 65098   // #FFC857
#define NX_C_PILL   4387   // #10271A
#define NX_C_ACTIVE 4516   // #123626

typedef void (*NextionRangeCallback)(int cm);
typedef void (*NextionVoidCallback)();
typedef void (*NextionCommandCallback)(const char *cmd);

#if NEXTION_ENABLED

namespace NextionUI {

static HardwareSerial NxSerial(2);

// Blip history for the local PPI. 128 entries â‰ˆ 7â€“9 s of sweep data.
static const uint8_t NX_BLIP_COUNT = 128;
struct Blip {
  uint8_t angle;
  uint16_t dist;
  uint32_t t;
};
static Blip g_blips[NX_BLIP_COUNT];
static uint8_t g_blipHead = 0;

static bool     g_ready      = false;
static uint8_t  g_page       = NX_PAGE_RADAR;
static int      g_lastAngle  = 0;
static int      g_lastDist   = 0;
static int      g_maxRange   = 200;
static int      g_alertCm    = NX_ALERT_CM;
static bool     g_paused     = false;
static bool     g_hand       = false;
static int      g_target     = 90;
static int      g_step       = 2;
static bool     g_buzzer     = true;

static uint32_t g_nextDrawMs = 0;
static uint32_t g_nextTextMs = 0;
static uint32_t g_bootDeadlineMs = 0;   // fallback if the welcome page never reports

static int      g_lastShownAngle = -1;
static int      g_lastShownDist  = -1;
static int      g_lastShownNear  = -1;
static bool     g_lastShownWater = false;
static bool     g_alertDirty     = true;   // force initial alert refresh

static uint8_t  g_rxBuf[8];
static uint8_t  g_rxLen = 0;
static char     g_lineBuf[32];
static uint8_t  g_lineLen = 0;
static uint32_t g_rxTotal = 0;
static int      g_lastTouchComp = -1;
static uint32_t g_lastTouchMs = 0;

static String g_mode = "AP";
static String g_apip;
static String g_staip;
static String g_fw;
static int    g_rssi = 0;
static uint32_t g_up = 0;

static NextionRangeCallback  g_onRange = nullptr;
static NextionVoidCallback   g_onPause = nullptr;
static NextionCommandCallback g_onCommand = nullptr;

// ---------------------------------------------------------------------------

static void cmd(const String &s) {
  NxSerial.print(s);
  NxSerial.write(0xFF);
  NxSerial.write(0xFF);
  NxSerial.write(0xFF);
}

static void cmd(const char *s) {
  cmd(String(s));
}

static String upString(uint32_t seconds) {
  uint32_t h = seconds / 3600;
  uint32_t m = (seconds % 3600) / 60;
  uint32_t s = seconds % 60;
  char b[16];
  snprintf(b, sizeof(b), "%02lu:%02lu:%02lu",
           (unsigned long)h, (unsigned long)m, (unsigned long)s);
  return String(b);
}

static int nearestCm(uint32_t now) {
  int best = 0;
  for (uint8_t i = 0; i < NX_BLIP_COUNT; i++) {
    const Blip &b = g_blips[i];
    if (b.dist == 0) continue;
    if ((uint32_t)(now - b.t) > 6000UL) continue;
    if (best == 0 || b.dist < best) best = b.dist;
  }
  return best;
}

// ---------------------------------------------------------------------------
//  Drawing
// ---------------------------------------------------------------------------

static void drawRadar() {
  uint32_t now = millis();

  // Clear the radar rectangle. No HMI components may sit inside this area.
  cmd("fill 0," + String(NX_RADAR_Y) + ",320," + String(NX_RADAR_H) + "," + String(NX_C_BLACK));

  // Rings
  for (int i = 1; i <= 4; i++) {
    int r = NX_RADAR_R * i / 4;
    cmd("cir " + String(NX_RADAR_CX) + "," + String(NX_RADAR_CY) + "," +
        String(r) + "," + String(NX_C_DIM));
  }

  // Crosshair
  cmd("line " + String(NX_RADAR_CX - NX_RADAR_R) + "," + String(NX_RADAR_CY) + "," +
      String(NX_RADAR_CX + NX_RADAR_R) + "," + String(NX_RADAR_CY) + "," + String(NX_C_DIM));
  cmd("line " + String(NX_RADAR_CX) + "," + String(NX_RADAR_CY - NX_RADAR_R) + "," +
      String(NX_RADAR_CX) + "," + String(NX_RADAR_CY + NX_RADAR_R) + "," + String(NX_C_DIM));

  // Active-sector ticks every 30Â° (0..180 logical = upper half)
  for (int a = 0; a <= 180; a += 30) {
    float rad = (180.0f + a) * PI / 180.0f;
    int x1 = NX_RADAR_CX + (int)(cosf(rad) * NX_RADAR_R);
    int y1 = NX_RADAR_CY + (int)(sinf(rad) * NX_RADAR_R);
    int x2 = NX_RADAR_CX + (int)(cosf(rad) * (NX_RADAR_R - 6));
    int y2 = NX_RADAR_CY + (int)(sinf(rad) * (NX_RADAR_R - 6));
    cmd("line " + String(x1) + "," + String(y1) + "," +
        String(x2) + "," + String(y2) + "," + String(NX_C_DIM));
  }

  // Recent contacts (newest 12), then the sweep line on top.
  // Keep each frame under ~1 KB for the Nextion's serial buffer.
  int shown = 0;
  for (int i = 1; i <= NX_BLIP_COUNT && shown < 12; i++) {
    int idx = (g_blipHead + NX_BLIP_COUNT - i) % NX_BLIP_COUNT;
    const Blip &b = g_blips[idx];
    if (b.dist == 0) continue;
    uint32_t age = now - b.t;
    if (age > 8000UL) continue;

    float r = (float)b.dist / (float)max(1, g_maxRange) * NX_RADAR_R;
    if (r > NX_RADAR_R) r = NX_RADAR_R;
    float rad = (180.0f + b.angle) * PI / 180.0f;
    int x = NX_RADAR_CX + (int)(cosf(rad) * r);
    int y = NX_RADAR_CY + (int)(sinf(rad) * r);

    int colour;
    if (b.dist <= g_alertCm) colour = NX_C_AMBER;
    else if (age < 2500UL)     colour = NX_C_GREEN;
    else                       colour = NX_C_DIM;

    cmd("cirs " + String(x) + "," + String(y) + ",2," + String(colour));
    shown++;
  }

  float rad = (180.0f + g_lastAngle) * PI / 180.0f;
  int ex = NX_RADAR_CX + (int)(cosf(rad) * NX_RADAR_R);
  int ey = NX_RADAR_CY + (int)(sinf(rad) * NX_RADAR_R);
  cmd("line " + String(NX_RADAR_CX) + "," + String(NX_RADAR_CY) + "," +
      String(ex) + "," + String(ey) + "," + String(NX_C_GREEN));
  cmd("cirs " + String(NX_RADAR_CX) + "," + String(NX_RADAR_CY) + ",2," + String(NX_C_GREEN));
}

// ---------------------------------------------------------------------------
//  Text pages
// ---------------------------------------------------------------------------

static void updateRadarText() {
  uint32_t now = millis();
  int nearCm = nearestCm(now);
  bool water = (nearCm > 0 && nearCm <= g_alertCm);

  if (g_lastShownDist != g_lastDist) {
    g_lastShownDist = g_lastDist;
    cmd("nDist.val=" + String(g_lastDist));
  }
  if (g_lastShownAngle != g_lastAngle) {
    g_lastShownAngle = g_lastAngle;
    cmd("nAngle.val=" + String(g_lastAngle));
  }
  if (g_lastShownNear != nearCm) {
    g_lastShownNear = nearCm;
    cmd("nNear.val=" + String(nearCm));
  }
  if (g_alertDirty || g_lastShownWater != water) {
    g_alertDirty = false;
    g_lastShownWater = water;
    cmd("tAlert.pco=" + String(water ? NX_C_AMBER : NX_C_GREEN));
    cmd(String("tAlert.txt=\"") + (water ? "AIR TERDETEKSI" : "AREA BERSIH") + "\"");
  }
}

static void updateInfoText() {
  cmd("tMode.txt=\"" + g_mode + "\"");
  cmd("tApIp.txt=\"" + g_apip + "\"");
  cmd("tStaIp.txt=\"" + (g_staip.length() ? g_staip : String("-")) + "\"");
  cmd("tRssi.txt=\"" + (g_staip.length() ? (String(g_rssi) + " dBm") : String("-")) + "\"");
  cmd("tUp.txt=\"" + upString(g_up) + "\"");
  cmd("tFw.txt=\"" + g_fw + "\"");
  cmd("tRange.txt=\"" + String(g_maxRange) + " cm\"");
  cmd("tPauseBtn.txt=\"" + String(g_paused ? "LANJUT" : "JEDA") + "\"");
}

// ---------------------------------------------------------------------------
//  Control page text
// ---------------------------------------------------------------------------

static void updateControlText() {
  cmd("tMode.txt=\"" + String(g_hand ? "HAND" : "AUTO") + "\"");
  cmd("tAngle.txt=\"" + String(g_hand ? g_target : g_lastAngle) + "°\"");
  cmd("tStepVal.txt=\"" + String(g_step) + "°\"");
  cmd("tBuzz.txt=\"BUZZER: " + String(g_buzzer ? "ON" : "OFF") + "\"");
  cmd("tThr.txt=\"AMBANG " + String(g_alertCm) + " cm\"");
  cmd("tPause2.txt=\"" + String(g_paused ? "LANJUT" : "JEDA") + "\"");
  cmd("tAuto.bco=" + String(g_hand ? NX_C_PILL : NX_C_ACTIVE));
  cmd("tHand.bco=" + String(g_hand ? NX_C_ACTIVE : NX_C_PILL));
  cmd("tAuto.pco=" + String(g_hand ? NX_C_DIM : NX_C_GREEN));
  cmd("tHand.pco=" + String(g_hand ? NX_C_GREEN : NX_C_DIM));
  cmd("tCtlSpd1.pco=" + String(g_step == 1 ? NX_C_GREEN : NX_C_DIM));
  cmd("tCtlSpd2.pco=" + String(g_step == 2 ? NX_C_GREEN : NX_C_DIM));
  cmd("tCtlSpd5.pco=" + String(g_step == 5 ? NX_C_GREEN : NX_C_DIM));
  cmd("tBuzz.pco=" + String(g_buzzer ? NX_C_GREEN : NX_C_DIM));
  cmd("tPause2.pco=" + String(g_paused ? NX_C_AMBER : NX_C_GREEN));
}

// ---------------------------------------------------------------------------
//  Touch
// ---------------------------------------------------------------------------

static void handleTouch(uint8_t page, uint8_t comp, uint8_t event) {
  uint32_t now = millis();
  if (now - g_lastTouchMs < 300UL && comp == g_lastTouchComp) return;
  g_lastTouchMs = now;
  g_lastTouchComp = comp;

  Serial.printf("[NX] touch page=%u comp=%u event=%u\n",
                (unsigned)page, (unsigned)comp, (unsigned)event);

  if (page != g_page) {
    g_page = page;
    g_nextTextMs = 0;
    g_nextDrawMs = 0;
  }

  if (page == NX_PAGE_RADAR) {
    if (comp == NX_ID_INFO) {
      g_page = NX_PAGE_INFO;
      cmd("page 3");
      g_nextTextMs = 0;
    }
    return;
  }

  if (page == NX_PAGE_INFO) {
    switch (comp) {
      case NX_ID_BACK:
        g_page = NX_PAGE_RADAR;
        cmd("page 0");
        g_lastShownAngle = -1;   // force a refresh of page 0
        g_lastShownDist  = -1;
        g_lastShownNear  = -1;
        g_alertDirty     = true;
        g_nextTextMs = 0;
        g_nextDrawMs = 0;
        break;
      case NX_ID_R100:
        if (g_onRange) g_onRange(100);
        break;
      case NX_ID_R200:
        if (g_onRange) g_onRange(200);
        break;
      case NX_ID_R300:
        if (g_onRange) g_onRange(300);
        break;
      case NX_ID_PAUSE:
        if (g_onPause) g_onPause();
        break;
      default:
        break;
    }
  }
}

static void handleTextCommand(const char *line) {
  Serial.printf("[NX] cmd %s\n", line);

  if (strcmp(line, "NX:READY") == 0) {
    // The splash has finished. Wait for radar.load's NX:RADAR before drawing.
    g_page = NX_PAGE_WELCOME;
    g_ready = false;
    g_nextDrawMs = 0;
    g_nextTextMs = 0;
    g_bootDeadlineMs = millis() + 3000UL;
    Serial.println("[NX] Welcome finished, waiting for radar page");
  } else if (strcmp(line, "NX:RADAR") == 0) {
    // radar.load has run: page 0 is now actually on screen.
    g_ready = true;
    g_page = NX_PAGE_RADAR;
    g_lastShownAngle = -1;
    g_lastShownDist  = -1;
    g_lastShownNear  = -1;
    g_alertDirty     = true;
    g_nextDrawMs = millis() + 400;
    g_nextTextMs = millis() + 400;
    Serial.println("[NX] Radar page loaded, drawing enabled");
  } else if (strcmp(line, "NX:CTRL") == 0) {
    g_page = NX_PAGE_CONTROL;
    g_nextTextMs = 0;
    g_nextDrawMs = 0;
  } else if (strcmp(line, "NX:CTRLBACK") == 0) {
    g_page = NX_PAGE_INFO;
    g_nextTextMs = 0;
    g_nextDrawMs = 0;
  } else if (strcmp(line, "NX:INFO") == 0) {
    g_page = NX_PAGE_INFO;
    g_nextTextMs = 0;
    g_nextDrawMs = 0;
  } else if (strcmp(line, "NX:BACK") == 0) {
    g_page = NX_PAGE_RADAR;
    g_lastShownAngle = -1;
    g_lastShownDist  = -1;
    g_lastShownNear  = -1;
    g_alertDirty     = true;
    g_nextTextMs = 0;
    g_nextDrawMs = 0;
  } else if (strcmp(line, "NX:R100") == 0) {
    if (g_onRange) g_onRange(100);
  } else if (strcmp(line, "NX:R200") == 0) {
    if (g_onRange) g_onRange(200);
  } else if (strcmp(line, "NX:R300") == 0) {
    if (g_onRange) g_onRange(300);
  } else if (strcmp(line, "NX:PAUSE") == 0) {
    if (g_onPause) g_onPause();
  } else if (strncmp(line, "NX:AUTO", 7) == 0 ||
             strncmp(line, "NX:HAND", 7) == 0 ||
             strncmp(line, "NX:HOLD", 7) == 0 ||
             strncmp(line, "NX:ANG", 6) == 0 ||
             strncmp(line, "NX:SPD", 6) == 0 ||
             strncmp(line, "NX:BUZZ", 7) == 0 ||
             strncmp(line, "NX:THR", 6) == 0) {
    if (g_onCommand) g_onCommand(line);
  }
}

static void pollSerial() {
  while (NxSerial.available()) {
    uint8_t b = (uint8_t)NxSerial.read();
    g_rxTotal++;

    if (g_rxLen == 0) {
      if (b == 0x65 || b == 0x66) {
        g_rxBuf[g_rxLen++] = b;
        g_lineLen = 0;
        continue;
      }
      if (b == '\r' || b == '\n') {
        if (g_lineLen > 0) {
          g_lineBuf[g_lineLen] = '\0';
          handleTextCommand(g_lineBuf);
          g_lineLen = 0;
        }
        continue;
      }
      if (b >= 32 && b < 127) {
        if (g_lineLen < sizeof(g_lineBuf) - 1) {
          g_lineBuf[g_lineLen++] = (char)b;
        }
        continue;
      }
      // Command return bytes / noise: do not let them join the next line.
      g_lineLen = 0;
      continue;
    }

    if (g_rxBuf[0] == 0x65) {
      g_rxBuf[g_rxLen++] = b;
      if (g_rxLen == 7) {
        if (g_rxBuf[4] == 0xFF && g_rxBuf[5] == 0xFF && g_rxBuf[6] == 0xFF) {
          handleTouch(g_rxBuf[1], g_rxBuf[2], g_rxBuf[3]);
        }
        g_rxLen = 0;
      }
    } else if (g_rxBuf[0] == 0x66) {
      g_rxBuf[g_rxLen++] = b;
      if (g_rxLen == 5) {
        if (g_rxBuf[2] == 0xFF && g_rxBuf[3] == 0xFF && g_rxBuf[4] == 0xFF) {
          if (g_rxBuf[1] != g_page) {
            g_page = g_rxBuf[1];
            g_nextTextMs = 0;
            g_nextDrawMs = 0;
          }
        }
        g_rxLen = 0;
      }
    } else {
      g_rxLen = 0;
    }
  }
}

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------

inline void setCallbacks(NextionRangeCallback onRange, NextionVoidCallback onPause) {
  g_onRange = onRange;
  g_onPause = onPause;
}

inline void setCommandCallback(NextionCommandCallback onCommand) {
  g_onCommand = onCommand;
}

inline uint32_t rxBytes() {
  return g_rxTotal;
}

inline void begin() {
  NxSerial.setRxBufferSize(512);
  NxSerial.setTxBufferSize(2048);
  NxSerial.begin(NX_BAUD, SERIAL_8N1, NX_RX_PIN, NX_TX_PIN);

  // The Nextion boots slower than the ESP32. The welcome page plays for
  // 3.5 s and then prints NX:READY; until then, do not draw on it.
  delay(1200);

  cmd("bkcmd=0");     // no command-return chatter
  cmd("sendxy=0");    // use component IDs, not raw coordinates
  g_page = NX_PAGE_WELCOME;
  g_ready = false;
  g_bootDeadlineMs = millis() + 8000UL;
  g_nextDrawMs = 0;
  g_nextTextMs = 0;

  Serial.printf("[NX] Nextion UART started: %lu baud, RX=%d TX=%d (waiting for welcome)\n",
                (unsigned long)NX_BAUD, NX_RX_PIN, NX_TX_PIN);
}

inline void addReading(int angle, int distance) {
  if (!g_ready) return;
  g_lastAngle = constrain(angle, 0, 180);

  if (distance > 0) {
    g_lastDist = distance;
    Blip &b = g_blips[g_blipHead];
    b.angle = (uint8_t)g_lastAngle;
    b.dist = (uint16_t)constrain(distance, 0, 2000);
    b.t = millis();
    g_blipHead = (g_blipHead + 1) % NX_BLIP_COUNT;
  }
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
  g_alertDirty = true;
}

inline void setNetwork(const char *mode, const char *apip, const char *staip,
                       int rssi, unsigned long up, const char *fw) {
  g_mode  = mode ? mode : "";
  g_apip  = apip ? apip : "";
  g_staip = staip ? staip : "";
  g_rssi  = rssi;
  g_up    = (uint32_t)up;
  g_fw    = fw ? fw : "";
}

inline void loop() {
  pollSerial();

  if (!g_ready) {
    // Custom HMIs without NX:READY still start after a short grace period.
    if ((int32_t)(millis() - g_bootDeadlineMs) >= 0) {
      cmd("page 0");
      g_ready = true;
      g_page = NX_PAGE_RADAR;
      g_lastShownAngle = -1;
      g_lastShownDist  = -1;
      g_lastShownNear  = -1;
      g_alertDirty     = true;
      g_nextDrawMs = millis() + 800;
      g_nextTextMs = millis() + 800;
      Serial.println("[NX] Welcome timeout, forcing radar page");
    }
    return;
  }

  uint32_t now = millis();

  if (g_page == NX_PAGE_RADAR) {
    if ((int32_t)(now - g_nextDrawMs) >= 0) {
      g_nextDrawMs = now + 200;      // 5 fps radar
      drawRadar();
    }
    if ((int32_t)(now - g_nextTextMs) >= 0) {
      g_nextTextMs = now + 250;      // 4 Hz readouts
      updateRadarText();
    }
  } else if (g_page == NX_PAGE_CONTROL) {
    if ((int32_t)(now - g_nextTextMs) >= 0) {
      g_nextTextMs = now + 500;      // 2 Hz control page
      updateControlText();
    }
  } else {
    if ((int32_t)(now - g_nextTextMs) >= 0) {
      g_nextTextMs = now + 500;      // 2 Hz info page
      updateInfoText();
    }
  }
}

}  // namespace NextionUI

#else  // NEXTION_ENABLED

namespace NextionUI {
inline void setCallbacks(NextionRangeCallback, NextionVoidCallback) {}
inline void setCommandCallback(NextionCommandCallback) {}
inline uint32_t rxBytes() { return 0; }
inline void begin() {}
inline void addReading(int, int) {}
inline void setConfig(int, bool, int, bool, int, int, bool) {}
inline void setNetwork(const char *, const char *, const char *, int, unsigned long, const char *) {}
inline void loop() {}
}  // namespace NextionUI

#endif  // NEXTION_ENABLED
