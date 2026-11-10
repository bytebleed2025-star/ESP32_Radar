/*
 * ============================================================================
 *  DETEKTOR AIR LACAK  —  ESP32 + HC-SR04 + SG90 Radar Sonar
 * ============================================================================
 *
 *  A 180° ultrasonic radar with a live sonar/PPI web interface.
 *
 *  v1.1 additions
 *  --------------
 *    - AP + STA at the same time ("AP+STA")
 *    - Network settings stored in NVS and editable from the CONFIG tab
 *    - OTA updates: web upload (Config tab) and ArduinoOTA (port 3232)
 *    - WiFi scan, hostname/mDNS, factory reset
 *
 *  Hardware
 *  --------
 *    HC-SR04  VCC  -> 5V
 *             GND  -> GND
 *             TRIG -> GPIO 5
 *             ECHO -> GPIO 18  (through a 1k/2k voltage divider! 5V -> 3.3V)
 *    SG90     +    -> external 5V (1A) + 470uF capacitor recommended
 *             GND  -> common GND with ESP32
 *             SIG  -> GPIO 19
 *    Buzzer   +    -> GPIO 23 (active buzzer; set BUZZER_ACTIVE 0 for passive)
 *             -    -> GND
 *
 *  Libraries (Arduino IDE -> Library Manager)
 *  ------------------------------------------
 *    - ESP32Servo        by Kevin Harrington
 *    - WebSockets        by Markus Sattler (links2004)
 *
 *  Board: "ESP32 Dev Module"
 *
 *  IMPORTANT for OTA
 *  -----------------
 *    Arduino IDE: Tools -> Partition Scheme ->
 *      "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"
 *    PlatformIO: board_build.partitions = min_spiffs.csv  (already set)
 *    Without an OTA partition table the upload endpoint will refuse the
 *    update (the Config tab shows a warning).
 *
 *  Default network (editable from the CONFIG tab afterwards)
 *  ---------------------------------------------------------
 *    AP  : "DetektorAir" / "radar12345"   ->  http://192.168.4.1
 *    STA : blank below; fill STA_SSID/STA_PASS or use the Config tab.
 *
 *  The web assets (index.html + logo) are embedded through web_assets.h.
 *  Edit web/index.html, run  python tools/build_assets.py, then re-upload.
 * ============================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <ArduinoOTA.h>
#include <WebSocketsServer.h>
#include <ESP32Servo.h>

#include "web_assets.h"

// Set to 0 if you do not have the Nextion display attached.
#define NEXTION_ENABLED 1
#include "nextion_ui.h"

/* --------------------------------------------------------------------------
 *  VERSION
 * ------------------------------------------------------------------------ */

#define FW_VERSION "1.1.0"
#define FW_BUILD   __DATE__ " " __TIME__

/* --------------------------------------------------------------------------
 *  COMPILE-TIME DEFAULTS  (used only when NVS has no saved settings)
 * ------------------------------------------------------------------------ */

// Home WiFi. Leave empty to use AP only (still available in the Config tab).
const char *STA_SSID = "";
const char *STA_PASS = "";

// Hotspot / AP
const char *AP_SSID = "DetektorAir";
const char *AP_PASS = "radar12345";       // >= 8 chars, or "" for open
const char *HOSTNAME = "detektorair";     // http://detektorair.local

// ArduinoOTA password. Web OTA is protected by the local network only.
const char *OTA_PASSWORD = "lacakota";

// Pins
#define PIN_TRIG   5
#define PIN_ECHO  18
#define PIN_SERVO 19
#define PIN_BUZZER 23       // active buzzer output

// 1 = active buzzer (HIGH = sound), 0 = passive buzzer (tone)
#define BUZZER_ACTIVE 1
#define BUZZER_TONE_HZ 2200

// SG90 safe mechanical limits. The UI always reports logical 0..180 degrees.
#define SERVO_LEFT   12
#define SERVO_RIGHT 168

/* --------------------------------------------------------------------------
 *  SCAN DEFAULTS
 * ------------------------------------------------------------------------ */

static int     maxRangeCm = 200;   // 40 .. 400 cm
static uint8_t stepDeg    = 2;     // 1 .. 5 degrees per ping

static const uint16_t STEP_SETTLE_MS   = 16;
static const uint16_t STEP_INTERVAL_MS = 36;

/* --------------------------------------------------------------------------
 *  GLOBALS
 * ------------------------------------------------------------------------ */

Servo servo;
WebServer server(80);
WebSocketsServer webSocket(81);
Preferences prefs;

static String g_staSsid, g_staPass, g_apSsid, g_apPass, g_hostname;

static bool g_apStarted      = false;
static bool g_staWasConnected = false;
static bool g_paused         = false;
static bool g_handMode       = false;
static bool g_handSweep      = false;
static unsigned long g_handHoldUntil = 0;
static int  g_manualTarget   = 90;
static int  g_alertCm        = 50;
static bool g_buzzerEnabled  = true;
static bool g_buzzerLevel    = false;
static unsigned long g_lastNearMs   = 0;
static unsigned long g_buzzerToggleMs = 0;
static unsigned long g_lastHandPingMs = 0;
static int  g_lastMeasurement = 0;
static int  g_angle          = 0;
static int  g_direction      = 1;
static unsigned long g_lastStepMs   = 0;
static unsigned long g_lastStatusMs = 0;
static unsigned long g_lastStaAttempt = 0;

static void buzzerWrite(bool on);

static bool g_otaActive = false;
static bool g_otaOk     = false;
static String g_otaError;
static unsigned long g_rebootAt = 0;

static const unsigned long STA_RETRY_MS = 20000UL;

/* --------------------------------------------------------------------------
 *  SMALL HELPERS
 * ------------------------------------------------------------------------ */

static String jsonEscape(const String &in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if ((unsigned char)c < 0x20) {
          char b[7];
          snprintf(b, sizeof(b), "\\u%04x", (unsigned int)(unsigned char)c);
          out += b;
        } else {
          out += c;
        }
    }
  }
  return out;
}

static bool validSsid(const String &s) {
  return s.length() >= 1 && s.length() <= 32;
}

static bool validPass(const String &s) {
  return s.length() == 0 || (s.length() >= 8 && s.length() <= 63);
}

static bool validHostname(const String &h) {
  if (h.length() < 1 || h.length() > 32) return false;
  if (h[0] == '-' || h[h.length() - 1] == '-') return false;
  for (size_t i = 0; i < h.length(); i++) {
    char c = h[i];
    if (!(isalnum((unsigned char)c) || c == '-')) return false;
  }
  return true;
}

static void sendJson(int code, const String &body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json; charset=utf-8", body);
}

static void sendError(int code, const String &msg) {
  sendJson(code, String("{\"ok\":false,\"msg\":\"") + jsonEscape(msg) + "\"}");
}

static void scheduleReboot(unsigned long delayMs) {
  g_rebootAt = millis() + delayMs;
}

static const char *networkMode() {
  bool sta = (WiFi.status() == WL_CONNECTED);
  if (g_apStarted && sta) return "AP+STA";
  if (g_apStarted)        return "AP";
  if (sta)                return "STA";
  return "OFFLINE";
}

static String staIpString() {
  return (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : String("");
}

static bool otaCapable() {
  return esp_ota_get_next_update_partition(NULL) != NULL;
}

/* --------------------------------------------------------------------------
 *  CONFIG STORAGE (NVS)
 * ------------------------------------------------------------------------ */

static void loadConfig() {
  prefs.begin("lacak", true);
  g_staSsid  = prefs.isKey("sta_ssid") ? prefs.getString("sta_ssid", "") : String(STA_SSID);
  g_staPass  = prefs.isKey("sta_pass") ? prefs.getString("sta_pass", "") : String(STA_PASS);
  g_apSsid   = prefs.isKey("ap_ssid")  ? prefs.getString("ap_ssid", "")  : String(AP_SSID);
  g_apPass   = prefs.isKey("ap_pass")  ? prefs.getString("ap_pass", "")  : String(AP_PASS);
  g_hostname = prefs.isKey("hostname") ? prefs.getString("hostname", "") : String(HOSTNAME);
  prefs.end();

  g_staSsid.trim();
  g_apSsid.trim();
  g_hostname.trim();

  if (!validSsid(g_apSsid)) g_apSsid = AP_SSID;
  if (!validHostname(g_hostname)) g_hostname = HOSTNAME;
  if (!validPass(g_apPass)) g_apPass = AP_PASS;   // empty = open, still valid
}

static void saveConfig() {
  prefs.begin("lacak", false);
  prefs.putString("sta_ssid", g_staSsid);
  prefs.putString("sta_pass", g_staPass);
  prefs.putString("ap_ssid", g_apSsid);
  prefs.putString("ap_pass", g_apPass);
  prefs.putString("hostname", g_hostname);
  prefs.end();
}

static void clearConfig() {
  prefs.begin("lacak", false);
  prefs.clear();
  prefs.end();
}

/* --------------------------------------------------------------------------
 *  SENSOR
 * ------------------------------------------------------------------------ */

static float pingCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(4);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long timeoutUs = (unsigned long)maxRangeCm * 58UL + 2000UL;
  unsigned long duration  = pulseIn(PIN_ECHO, HIGH, timeoutUs);

  if (duration == 0) return -1.0f;

  float cm = duration / 58.0f;
  if (cm < 2.0f || cm > (float)maxRangeCm) return -1.0f;
  return cm;
}

static int servoPosFor(int logical) {
  return map(constrain(logical, 0, 180), 0, 180, SERVO_LEFT, SERVO_RIGHT);
}

/* --------------------------------------------------------------------------
 *  NETWORK
 * ------------------------------------------------------------------------ */

static void connectSta() {
  if (g_staSsid.length() == 0) return;
  if (g_staPass.length() > 0) WiFi.begin(g_staSsid.c_str(), g_staPass.c_str());
  else                        WiFi.begin(g_staSsid.c_str());
  g_lastStaAttempt = millis();
}

static void startNetwork() {
  WiFi.persistent(false);            // settings are managed in our own NVS keys
  WiFi.mode(WIFI_AP_STA);            // AP and STA at the same time
  WiFi.setSleep(false);
  WiFi.setHostname(g_hostname.c_str());
  WiFi.softAPsetHostname(g_hostname.c_str());
  WiFi.setAutoReconnect(true);

  if (g_apPass.length() > 0) {
    g_apStarted = WiFi.softAP(g_apSsid.c_str(), g_apPass.c_str());
  } else {
    g_apStarted = WiFi.softAP(g_apSsid.c_str());   // open hotspot
  }

  Serial.printf("AP : %s  ->  http://%s\n",
                g_apSsid.c_str(),
                WiFi.softAPIP().toString().c_str());
  if (!g_apStarted) Serial.println("AP : FAILED to start");

  if (g_staSsid.length() > 0) {
    Serial.printf("STA: connecting to \"%s\"...\n", g_staSsid.c_str());
    connectSta();
  } else {
    Serial.println("STA: disabled (configure it in the CONFIG tab)");
  }
}

static void loopNetwork() {
  if (g_otaActive) return;
  if (g_staSsid.length() == 0) return;

  if (WiFi.status() == WL_CONNECTED) {
    if (!g_staWasConnected) {
      g_staWasConnected = true;
      Serial.printf("STA: connected, IP = %s\n", WiFi.localIP().toString().c_str());
    }
    return;
  }

  g_staWasConnected = false;
  if (millis() - g_lastStaAttempt >= STA_RETRY_MS) {
    Serial.println("STA: retrying...");
    connectSta();
  }
}

/* --------------------------------------------------------------------------
 *  STATUS / WEBSOCKET
 * ------------------------------------------------------------------------ */

static void sendConfig(uint8_t num) {
  char buf[128];
  int n = snprintf(buf, sizeof(buf),
                   "{\"cfg\":1,\"mx\":%d,\"s\":%u,\"p\":%u,\"hm\":%u,"
                   "\"ta\":%d,\"bz\":%u,\"thr\":%d}",
                   maxRangeCm, (unsigned)stepDeg, g_paused ? 1u : 0u,
                   g_handMode ? 1u : 0u,
                   g_handMode ? g_manualTarget : g_angle,
                   g_buzzerEnabled ? 1u : 0u, g_alertCm);
  if (num == 255) webSocket.broadcastTXT(buf, n);
  else            webSocket.sendTXT(num, buf, n);

  NextionUI::setConfig(maxRangeCm, g_paused, g_alertCm,
                       g_handMode,
                       g_handMode ? g_manualTarget : g_angle,
                       stepDeg, g_buzzerEnabled);
}

static void sendStatus() {
  bool   sta   = (WiFi.status() == WL_CONNECTED);
  String apip  = g_apStarted ? WiFi.softAPIP().toString() : String("");
  String staip = sta ? WiFi.localIP().toString() : String("");

  char buf[384];
  int n = snprintf(buf, sizeof(buf),
                   "{\"st\":1,\"mode\":\"%s\",\"apip\":\"%s\",\"staip\":\"%s\","
                   "\"sta\":%d,\"rssi\":%d,\"up\":%lu,\"cl\":%u,\"heap\":%lu,"
                   "\"fw\":\"%s\",\"ota\":%d}",
                   networkMode(), apip.c_str(), staip.c_str(),
                   sta ? 1 : 0, sta ? WiFi.RSSI() : 0,
                   (unsigned long)(millis() / 1000UL),
                   (unsigned)webSocket.connectedClients(),
                   (unsigned long)ESP.getFreeHeap(),
                   FW_VERSION, g_otaActive ? 1 : 0);
  webSocket.broadcastTXT(buf, n);

  NextionUI::setNetwork(networkMode(), apip.c_str(), staip.c_str(),
                        sta ? WiFi.RSSI() : 0,
                        (unsigned long)(millis() / 1000UL), FW_VERSION);
}

// Nextion page 1 buttons: keep the local UI and the web UI in sync.
static void onNextionRange(int cm) {
  maxRangeCm = constrain(cm, 40, 400);
  sendConfig(255);
}

static void onNextionPause() {
  g_paused = !g_paused;
  servo.write(servoPosFor(g_angle));
  sendConfig(255);
}

// Control page commands from the Nextion (page 1).
static void onNextionCommand(const char *cmd) {
  if (strcmp(cmd, "NX:AUTO") == 0) {
    g_handMode = false;
    g_handSweep = false;
    g_paused = false;
    g_direction = 1;
  } else if (strcmp(cmd, "NX:HAND") == 0) {
    g_handMode = true;
    g_handSweep = false;
    g_paused = true;
    g_manualTarget = g_angle;
    servo.write(servoPosFor(g_angle));
  } else if (strcmp(cmd, "NX:HOLDON") == 0) {
    g_handMode = true;
    g_handSweep = true;
    g_handHoldUntil = millis() + 800UL;
    g_paused = false;
    g_lastHandPingMs = 0;
  } else if (strcmp(cmd, "NX:HOLDOFF") == 0) {
    g_handSweep = false;
    g_paused = true;
  } else if (strcmp(cmd, "NX:ANG+") == 0) {
    g_handMode = true;
    g_handSweep = false;
    g_manualTarget = constrain(g_manualTarget + 5, 0, 180);
    g_paused = false;
  } else if (strcmp(cmd, "NX:ANG-") == 0) {
    g_handMode = true;
    g_handSweep = false;
    g_manualTarget = constrain(g_manualTarget - 5, 0, 180);
    g_paused = false;
  } else if (strcmp(cmd, "NX:SPD1") == 0) {
    stepDeg = 1;
  } else if (strcmp(cmd, "NX:SPD2") == 0) {
    stepDeg = 2;
  } else if (strcmp(cmd, "NX:SPD5") == 0) {
    stepDeg = 5;
  } else if (strcmp(cmd, "NX:BUZZ") == 0) {
    g_buzzerEnabled = !g_buzzerEnabled;
    if (!g_buzzerEnabled) {
      g_buzzerLevel = false;
      buzzerWrite(false);
    }
  } else if (strcmp(cmd, "NX:THR+") == 0) {
    g_alertCm = constrain(g_alertCm + 5, 10, 150);
  } else if (strcmp(cmd, "NX:THR-") == 0) {
    g_alertCm = constrain(g_alertCm - 5, 10, 150);
  } else {
    return;
  }
  sendConfig(255);
}

static void onWsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[WS] client %u connected from %s\n", num, ip.toString().c_str());
      sendConfig(num);
      break;
    }

    case WStype_DISCONNECTED:
      Serial.printf("[WS] client %u disconnected\n", num);
      break;

    case WStype_TEXT: {
      if (length == 0 || length > 31) break;

      char msg[32];
      memcpy(msg, payload, length);
      msg[length] = '\0';
      Serial.printf("[WS] cmd %s\n", msg);

      switch (msg[0]) {
        case 'R':                       // "R250" -> range in cm
          maxRangeCm = constrain(atoi(msg + 1), 40, 400);
          sendConfig(255);
          break;

        case 'S':                       // "S2" -> degrees per step
          stepDeg = (uint8_t)constrain(atoi(msg + 1), 1, 5);
          sendConfig(255);
          break;

        case 'P': {                     // "P1" stop / "P0" start/hold
          bool paused = (atoi(msg + 1) != 0);
          if (g_handMode) {
            if (!paused) {
              // Momentary HOLD: sweep while P0 heartbeats arrive.
              g_handSweep = true;
              g_handHoldUntil = millis() + 800UL;
              g_lastHandPingMs = 0;
            } else {
              g_handSweep = false;
            }
          }
          g_paused = paused;
          servo.write(servoPosFor(g_angle));
          Serial.printf("[WS] %s paused=%d sweep=%d target=%d angle=%d\n",
                        g_handMode ? "HAND" : "AUTO", g_paused ? 1 : 0,
                        g_handSweep ? 1 : 0, g_manualTarget, g_angle);
          sendConfig(255);
          break;
        }

        case 'M':                       // "M0" auto / "M1" hand
          g_handMode = (atoi(msg + 1) != 0);
          g_handSweep = false;
          g_handHoldUntil = 0;
          g_paused = g_handMode;        // HAND starts stopped, AUTO runs
          g_lastHandPingMs = 0;
          if (g_handMode) {
            g_manualTarget = g_angle;
            servo.write(servoPosFor(g_angle));
          } else {
            g_direction = 1;
          }
          Serial.printf("[WS] mode=%s target=%d angle=%d paused=%d\n",
                        g_handMode ? "HAND" : "AUTO", g_manualTarget, g_angle,
                        g_paused ? 1 : 0);
          sendConfig(255);
          break;

        case 'A':                       // "A120" manual target angle
          g_handMode = true;
          g_handSweep = false;
          g_handHoldUntil = 0;
          g_manualTarget = constrain(atoi(msg + 1), 0, 180);
          g_paused = false;
          Serial.printf("[WS] hand target=%d angle=%d paused=%d\n",
                        g_manualTarget, g_angle, g_paused ? 1 : 0);
          sendConfig(255);
          break;

        case 'B':                       // "B1" buzzer on / "B0" off
          g_buzzerEnabled = (atoi(msg + 1) != 0);
          if (!g_buzzerEnabled) {
            g_buzzerLevel = false;
            buzzerWrite(false);
          }
          sendConfig(255);
          break;

        case 'T':                       // "T50" alert/buzzer threshold in cm
          g_alertCm = constrain(atoi(msg + 1), 10, 150);
          sendConfig(255);
          break;

        default:
          break;
      }
      break;
    }

    default:
      break;
  }
}

/* --------------------------------------------------------------------------
 *  CONFIG / INFO HTTP API
 * ------------------------------------------------------------------------ */

static void handleInfo() {
  bool sta = (WiFi.status() == WL_CONNECTED);
  bool cantOta = otaCapable();
  String body;
  body.reserve(640);
  body += "{\"ok\":true";
  body += ",\"fw\":\"" FW_VERSION "\"";
  body += ",\"build\":\"" FW_BUILD "\"";
  body += ",\"mode\":\"" + String(networkMode()) + "\"";
  body += ",\"host\":\"" + jsonEscape(g_hostname) + "\"";
  body += ",\"chip\":\"" + jsonEscape(String(ESP.getChipModel())) + "\"";
  body += ",\"sdk\":\"" + jsonEscape(String(ESP.getSdkVersion())) + "\"";
  body += ",\"apip\":\"" + (g_apStarted ? WiFi.softAPIP().toString() : String("")) + "\"";
  body += ",\"staip\":\"" + (sta ? WiFi.localIP().toString() : String("")) + "\"";
  body += ",\"sta_connected\":" + String(sta ? "true" : "false");
  body += ",\"ap_started\":" + String(g_apStarted ? "true" : "false");
  body += ",\"ap_ssid\":\"" + jsonEscape(g_apSsid) + "\"";
  body += ",\"ap_open\":" + String(g_apPass.length() == 0 ? "true" : "false");
  body += ",\"sta_ssid\":\"" + jsonEscape(g_staSsid) + "\"";
  body += ",\"has_sta_pass\":" + String(g_staPass.length() ? "true" : "false");
  body += ",\"has_ap_pass\":" + String(g_apPass.length() ? "true" : "false");
  body += ",\"rssi\":" + String(sta ? WiFi.RSSI() : 0);
  body += ",\"up\":" + String((unsigned long)(millis() / 1000UL));
  body += ",\"heap\":" + String((unsigned long)ESP.getFreeHeap());
  body += ",\"flash\":" + String((unsigned long)ESP.getFlashChipSize());
  body += ",\"clients\":" + String((unsigned)webSocket.connectedClients());
  body += ",\"ota_capable\":" + String(cantOta ? "true" : "false");
  body += ",\"ota\":" + String(g_otaActive ? "true" : "false");
  body += "}";
  sendJson(200, body);
}

static void handleDebug() {
  char body[256];
  snprintf(body, sizeof(body),
           "{\"ok\":true,\"mode\":\"%s\",\"paused\":%d,\"sweep\":%d,"
           "\"angle\":%d,\"target\":%d,\"step\":%u,\"alert\":%d,"
           "\"buzzer\":%d,\"last_cm\":%d,\"nx_rx\":%lu}",
           g_handMode ? "HAND" : "AUTO", g_paused ? 1 : 0,
           g_handSweep ? 1 : 0, g_angle, g_manualTarget, (unsigned)stepDeg,
           g_alertCm, g_buzzerEnabled ? 1 : 0, g_lastMeasurement,
           (unsigned long)NextionUI::rxBytes());
  sendJson(200, body);
}

static void handleConfigGet() {
  String body;
  body.reserve(400);
  body += "{\"ok\":true";
  body += ",\"sta_ssid\":\"" + jsonEscape(g_staSsid) + "\"";
  body += ",\"has_sta_pass\":" + String(g_staPass.length() ? "true" : "false");
  body += ",\"ap_ssid\":\"" + jsonEscape(g_apSsid) + "\"";
  body += ",\"has_ap_pass\":" + String(g_apPass.length() ? "true" : "false");
  body += ",\"ap_open\":" + String(g_apPass.length() == 0 ? "true" : "false");
  body += ",\"hostname\":\"" + jsonEscape(g_hostname) + "\"";
  body += ",\"fw\":\"" FW_VERSION "\"";
  body += "}";
  sendJson(200, body);
}

static void handleConfigSave() {
  if (g_otaActive) { sendError(409, "OTA sedang berjalan"); return; }

  String staSsid = server.hasArg("sta_ssid") ? server.arg("sta_ssid") : g_staSsid;
  String staPass = server.hasArg("sta_pass") ? server.arg("sta_pass") : String("");
  String apSsid  = server.hasArg("ap_ssid")  ? server.arg("ap_ssid")  : g_apSsid;
  String apPass  = server.hasArg("ap_pass")  ? server.arg("ap_pass")  : String("");
  String host    = server.hasArg("hostname") ? server.arg("hostname") : g_hostname;
  bool   apOpen  = (server.hasArg("ap_open") && server.arg("ap_open") == "1");

  staSsid.trim();
  apSsid.trim();
  host.trim();

  if (staSsid.length() > 0 && !validSsid(staSsid)) {
    sendError(400, "SSID WiFi rumah tidak valid (1-32 karakter)");
    return;
  }
  if (!validSsid(apSsid)) {
    sendError(400, "SSID hotspot wajib diisi (1-32 karakter)");
    return;
  }
  if (!validHostname(host)) {
    sendError(400, "Hostname tidak valid (huruf, angka, tanda minus)");
    return;
  }

  // Password: empty means "keep the current one"; AP open is explicit.
  String newStaPass = g_staPass;
  if (staPass.length() > 0) newStaPass = staPass;
  if (staSsid.length() == 0) newStaPass = "";
  if (!validPass(newStaPass)) {
    sendError(400, "Password WiFi rumah harus 8-63 karakter");
    return;
  }

  String newApPass = g_apPass;
  if (apOpen) {
    newApPass = "";
  } else if (apPass.length() > 0) {
    newApPass = apPass;
  }
  if (!apOpen && newApPass.length() == 0) {
    sendError(400, "Isi password hotspot, atau aktifkan hotspot terbuka");
    return;
  }
  if (!validPass(newApPass)) {
    sendError(400, "Password hotspot harus 8-63 karakter");
    return;
  }

  g_staSsid  = staSsid;
  g_staPass  = newStaPass;
  g_apSsid   = apSsid;
  g_apPass   = newApPass;
  g_hostname = host;
  saveConfig();

  sendJson(200, "{\"ok\":true,\"reboot\":true,"
                "\"msg\":\"Pengaturan disimpan. Perangkat akan restart.\"}");
  scheduleReboot(1500);
}

static void handleScan() {
  if (g_otaActive) { sendError(409, "OTA sedang berjalan"); return; }

  int n = WiFi.scanNetworks();
  if (n < 0) {
    sendError(500, "Scan WiFi gagal");
    return;
  }

  struct ScanItem { String ssid; int rssi; bool secure; };
  static const int MAX_ITEMS = 24;
  ScanItem items[MAX_ITEMS];
  int count = 0;

  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    ssid.trim();
    if (ssid.length() == 0) continue;
    if (ssid == g_apSsid) continue;                 // skip our own hotspot

    int  rssi   = WiFi.RSSI(i);
    bool secure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

    int found = -1;
    for (int j = 0; j < count; j++) {
      if (items[j].ssid == ssid) { found = j; break; }
    }
    if (found >= 0) {
      if (rssi > items[found].rssi) {
        items[found].rssi = rssi;
        items[found].secure = secure;
      }
    } else if (count < MAX_ITEMS) {
      items[count].ssid = ssid;
      items[count].rssi = rssi;
      items[count].secure = secure;
      count++;
    }
  }
  WiFi.scanDelete();

  // Sort strongest first.
  for (int i = 1; i < count; i++) {
    ScanItem key = items[i];
    int j = i - 1;
    while (j >= 0 && items[j].rssi < key.rssi) {
      items[j + 1] = items[j];
      j--;
    }
    items[j + 1] = key;
  }

  String body;
  body.reserve(64 + count * 64);
  body += "{\"ok\":true,\"networks\":[";
  for (int i = 0; i < count; i++) {
    if (i) body += ",";
    body += "{\"ssid\":\"" + jsonEscape(items[i].ssid) + "\",";
    body += "\"rssi\":" + String(items[i].rssi) + ",";
    body += "\"secure\":" + String(items[i].secure ? "true" : "false") + "}";
  }
  body += "]}";
  sendJson(200, body);
}

static void handleReboot() {
  sendJson(200, "{\"ok\":true,\"reboot\":true,\"msg\":\"Restart...\"}");
  scheduleReboot(800);
}

static void handleFactoryReset() {
  clearConfig();
  sendJson(200, "{\"ok\":true,\"reboot\":true,"
                "\"msg\":\"Pengaturan jaringan dihapus. Restart...\"}");
  scheduleReboot(1200);
}

/* --------------------------------------------------------------------------
 *  OTA
 * ------------------------------------------------------------------------ */

static void otaUpload() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    g_otaActive = true;
    g_paused    = true;
    g_otaOk     = false;
    g_otaError  = "";
    Serial.printf("\nOTA: start \"%s\"\n", upload.filename.c_str());

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      g_otaError = Update.errorString();
      Serial.printf("OTA: begin failed: %s\n", g_otaError.c_str());
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (g_otaError.length() == 0) {
      size_t written = Update.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        g_otaError = Update.errorString();
        Serial.printf("OTA: write failed: %s\n", g_otaError.c_str());
      }
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (g_otaError.length() == 0) {
      if (!Update.end(true)) {
        g_otaError = Update.errorString();
        Serial.printf("OTA: end failed: %s\n", g_otaError.c_str());
      } else {
        g_otaOk = true;
        Serial.println("OTA: upload complete");
      }
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    g_otaError = "Upload dibatalkan";
    g_otaActive = false;
    g_paused    = false;
    Serial.println("OTA: aborted");
  }
}

static void handleOtaComplete() {
  if (g_otaActive && g_otaOk && g_otaError.length() == 0) {
    sendJson(200, "{\"ok\":true,\"reboot\":true,"
                  "\"msg\":\"Firmware berhasil diperbarui. Restart...\"}");
    scheduleReboot(1800);
    return;
  }

  String err = g_otaError.length() ? g_otaError : "Upload firmware gagal";
  sendError(500, err);
  g_otaActive = false;
  g_paused    = false;
}

static void startOta() {
  ArduinoOTA.setHostname(g_hostname.c_str());
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    g_otaActive = true;
    g_paused    = true;
    Serial.println("\nArduinoOTA: start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nArduinoOTA: done");
    g_otaActive = false;
    g_paused    = false;
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned int last = 0;
    unsigned int pct = total ? (progress * 100 / total) : 0;
    if (pct >= last + 10 || pct == 100) {
      last = pct;
      Serial.printf("ArduinoOTA: %u%%\n", pct);
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("ArduinoOTA error[%u]\n", (unsigned)error);
    g_otaActive = false;
    g_paused    = false;
  });
  ArduinoOTA.begin();
}

/* --------------------------------------------------------------------------
 *  SCAN
 * ------------------------------------------------------------------------ */

static void buzzerWrite(bool on) {
#if BUZZER_ACTIVE
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
#else
  if (on) tone(PIN_BUZZER, BUZZER_TONE_HZ);
  else    noTone(PIN_BUZZER);
#endif
}

static void updateBuzzer(uint32_t now) {
  bool near = g_lastNearMs != 0 &&
              (uint32_t)(now - g_lastNearMs) < 1500UL;
  bool active = g_buzzerEnabled && near;

  if (!active) {
    if (g_buzzerLevel) {
      g_buzzerLevel = false;
      buzzerWrite(false);
    }
    return;
  }

  // Faster beep when the object is very close.
  uint32_t period = (g_lastMeasurement > 0 && g_lastMeasurement <= 20) ? 90UL : 170UL;
  if ((uint32_t)(now - g_buzzerToggleMs) >= period) {
    g_buzzerToggleMs = now;
    g_buzzerLevel = !g_buzzerLevel;
    buzzerWrite(g_buzzerLevel);
  }
}

static void measureAtCurrent() {
  int distance = 0;
  float cm = pingCm();
  if (cm > 0.0f) distance = (int)(cm + 0.5f);

  g_lastMeasurement = distance;
  if (distance > 0 && distance <= g_alertCm) {
    g_lastNearMs = millis();
  }

  NextionUI::addReading(g_angle, distance);

  char buf[80];
  int n = snprintf(buf, sizeof(buf), "{\"a\":%d,\"d\":%d,\"mx\":%d}",
                   g_angle, distance, maxRangeCm);
  webSocket.broadcastTXT(buf, n);
}

static void takeStep() {
  if (g_handMode) {
    if (g_handSweep) {
      g_manualTarget = g_angle;
      servo.write(servoPosFor(g_angle));
      delay(STEP_SETTLE_MS);
      measureAtCurrent();

      g_angle += g_direction * (int)stepDeg;
      if (g_angle > 180) { g_angle = 360 - g_angle; g_direction = -1; }
      if (g_angle < 0)   { g_angle = -g_angle;     g_direction =  1; }
      return;
    }

    int target = constrain(g_manualTarget, 0, 180);
    int delta  = target - g_angle;

    if (delta != 0) {
      int move = constrain(delta, -(int)stepDeg, (int)stepDeg);
      g_angle += move;
      servo.write(servoPosFor(g_angle));
      delay(STEP_SETTLE_MS);
      measureAtCurrent();
      g_lastHandPingMs = millis();
    } else if (g_lastHandPingMs == 0 ||
               (uint32_t)(millis() - g_lastHandPingMs) >= 500UL) {
      // At the target: keep the distance readout fresh.
      servo.write(servoPosFor(g_angle));
      measureAtCurrent();
      g_lastHandPingMs = millis();
    }
    return;
  }

  servo.write(servoPosFor(g_angle));
  delay(STEP_SETTLE_MS);
  measureAtCurrent();

  g_angle += g_direction * (int)stepDeg;
  if (g_angle > 180) { g_angle = 360 - g_angle; g_direction = -1; }
  if (g_angle < 0)   { g_angle = -g_angle;     g_direction =  1; }
}

/* --------------------------------------------------------------------------
 *  SETUP
 * ------------------------------------------------------------------------ */

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("==========================================");
  Serial.println(" DETEKTOR AIR LACAK - ESP32 Radar Sonar");
  Serial.println(" Version " FW_VERSION);
  Serial.println("==========================================");

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 500, 2400);
  servo.write(servoPosFor(0));
  delay(450);

  loadConfig();
  startNetwork();

  if (MDNS.begin(g_hostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("mDNS: http://%s.local\n", g_hostname.c_str());
  }

  // HTTP
  server.on("/", []() {
    server.sendHeader("Cache-Control", "no-cache");
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML, INDEX_HTML_LEN);
  });

  server.on("/logo.jpg", []() {
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.send_P(200, "image/jpeg", (PGM_P)LOGO_JPG, LOGO_JPG_LEN);
  });

  server.on("/favicon.ico", []() {
    server.send(204, "text/plain", "");
  });

  server.on("/api/info",     HTTP_GET,  handleInfo);
  server.on("/api/debug",    HTTP_GET,  handleDebug);
  server.on("/api/config",   HTTP_GET,  handleConfigGet);
  server.on("/api/config",   HTTP_POST, handleConfigSave);
  server.on("/api/scan",     HTTP_GET,  handleScan);
  server.on("/api/reboot",   HTTP_POST, handleReboot);
  server.on("/api/factory",  HTTP_POST, handleFactoryReset);
  server.on("/ota",          HTTP_POST, handleOtaComplete, otaUpload);

  server.onNotFound([]() {
    if (server.uri().startsWith("/api/")) {
      sendError(404, "Endpoint tidak ditemukan");
    } else {
      server.sendHeader("Location", "/");
      server.send(302, "text/plain", "");
    }
  });

  server.begin();

  // WebSocket
  webSocket.begin();
  webSocket.onEvent(onWsEvent);

  // ArduinoOTA (port 3232)
  startOta();

  // Nextion display (UART2). It boots slower than the ESP32.
  NextionUI::setCallbacks(onNextionRange, onNextionPause);
  NextionUI::setCommandCallback(onNextionCommand);
  NextionUI::begin();

  Serial.println("------------------------------------------");
  Serial.println("AP+STA mode. HTTP :80, WebSocket :81");
  Serial.printf("AP  IP : %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("STA IP : %s\n", staIpString().length() ? staIpString().c_str() : "(not connected yet)");
  Serial.printf("OTA    : ArduinoOTA password \"%s\", web OTA in Config tab\n", OTA_PASSWORD);
  if (!otaCapable()) {
    Serial.println("WARNING: no OTA partition! Select an OTA partition scheme.");
  }
  Serial.println("Ready.");
}

/* --------------------------------------------------------------------------
 *  LOOP
 * ------------------------------------------------------------------------ */

void loop() {
  server.handleClient();
  webSocket.loop();
  ArduinoOTA.handle();
  NextionUI::loop();

  if (g_rebootAt && (long)(millis() - g_rebootAt) >= 0) {
    Serial.println("Rebooting...");
    delay(150);
    ESP.restart();
  }

  loopNetwork();

  unsigned long now = millis();

  updateBuzzer(now);

  // Momentary HAND control: stop if the browser stops sending P0 heartbeats.
  if (g_handMode && g_handSweep &&
      (int32_t)(now - g_handHoldUntil) >= 0) {
    g_handSweep = false;
    g_paused = true;
    Serial.println("[WS] HAND hold timeout, servo stopped");
    sendConfig(255);
  }

  if (!g_otaActive && !g_paused && now - g_lastStepMs >= STEP_INTERVAL_MS) {
    g_lastStepMs = now;
    takeStep();
  }

  if (!g_otaActive && now - g_lastStatusMs >= 1000UL) {
    g_lastStatusMs = now;
    sendStatus();
  }
}
