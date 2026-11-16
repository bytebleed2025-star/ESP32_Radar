#include <Arduino.h>
#include <BluetoothSerial.h>
#include <Preferences.h>

#include "st7735_ui.h"
#include "proximity.h"
#include "buzzer.h"

/* --------------------------------------------------------------------------
 *  v2.0.2 — Bluetooth Classic SPP, no WiFi AP/STA, no OTA
 * ------------------------------------------------------------------------ */

#define FW_VERSION "2.0.2"
#define BT_NAME    "DetektorAir"
#define BT_PIN     "1234"

#define PIN_TRIG 5
#define PIN_ECHO 18

#define STEP_MS 100
#define VIRTUAL_STEP_DEG 3

BluetoothSerial SerialBT;
Preferences prefs;

static int      maxRangeCm = 200;
static int      alertCm = 50;
static int      angle = 90;
static int      dir = 1;
static int      lastMeasurement = 0;
static uint32_t lastStepMs = 0;
static bool     clientWasConnected = false;
static String   rxLine;

/* --------------------------------------------------------------------------
 *  CONFIG
 * ------------------------------------------------------------------------ */

static void loadConfig() {
  prefs.begin("lacak", false);
  maxRangeCm = constrain((int)prefs.getUInt("max_cm", 200), 40, 400);
  alertCm    = constrain((int)prefs.getUInt("thr_cm", 50), 10, 150);
}

static void saveConfig() {
  prefs.putUInt("max_cm", maxRangeCm);
  prefs.putUInt("thr_cm", alertCm);
}

/* --------------------------------------------------------------------------
 *  DISTANCE
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

/* --------------------------------------------------------------------------
 *  BLUETOOTH PROTOCOL
 * ------------------------------------------------------------------------ */

static void sendConfig() {
  if (!SerialBT.hasClient()) return;
  char buf[224];
  snprintf(buf, sizeof(buf),
           "{\"cfg\":1,\"mx\":%d,\"thr\":%d,\"pc\":%lu,"
           "\"bz\":%u,\"ba\":%u,\"b0\":%lu,\"b1\":%lu,"
           "\"b2\":%lu,\"b3\":%lu,\"b4\":%lu}",
           maxRangeCm, alertCm,
           (unsigned long)Proximity::count(),
           Buzzer::enabled() ? 1u : 0u,
           Buzzer::activeType() ? 1u : 0u,
           (unsigned long)Buzzer::longOn(),
           (unsigned long)Buzzer::longOff(),
           (unsigned long)Buzzer::shortOn(),
           (unsigned long)Buzzer::shortOff(),
           (unsigned long)Buzzer::alertHold());
  SerialBT.println(buf);
}

static void sendTelemetry(int distance) {
  if (!SerialBT.hasClient()) return;
  char buf[96];
  snprintf(buf, sizeof(buf), "{\"a\":%d,\"d\":%d,\"mx\":%d,\"pc\":%lu}",
           angle, distance, maxRangeCm,
           (unsigned long)Proximity::count());
  SerialBT.println(buf);
}

static void sendProx() {
  if (!SerialBT.hasClient()) return;
  char buf[40];
  snprintf(buf, sizeof(buf), "{\"pc\":%lu}",
           (unsigned long)Proximity::count());
  SerialBT.println(buf);
}

static void handleCommand(const String &cmd) {
  if (cmd.length() < 2) return;

  switch (cmd[0]) {
    case 'C':
      if (cmd[1] == '0') {
        Proximity::reset();
        sendProx();
        sendConfig();
      }
      break;

    case 'B':
      Buzzer::setEnabled(cmd[1] == '1');
      sendConfig();
      break;

    case 'Y':
      Buzzer::setActive(cmd[1] == '1');
      sendConfig();
      break;

    case 'Z': {
      unsigned long lo, lf, so, sf, hd;
      if (sscanf(cmd.c_str() + 1, "%lu,%lu,%lu,%lu,%lu",
                 &lo, &lf, &so, &sf, &hd) == 5) {
        Buzzer::setTimings(lo, lf, so, sf, hd);
      }
      sendConfig();
      break;
    }

    case 'R':
      maxRangeCm = constrain(cmd.substring(1).toInt(), 40, 400);
      saveConfig();
      St7735UI::setConfig(maxRangeCm, alertCm);
      sendConfig();
      break;

    case 'T':
      alertCm = constrain(cmd.substring(1).toInt(), 10, 150);
      saveConfig();
      St7735UI::setConfig(maxRangeCm, alertCm);
      sendConfig();
      break;

    default:
      break;
  }
}

static void handleBluetooth() {
  while (SerialBT.available()) {
    char c = (char)SerialBT.read();
    if (c == '\n' || c == '\r') {
      if (rxLine.length()) {
        handleCommand(rxLine);
        rxLine = "";
      }
    } else if (rxLine.length() < 63) {
      rxLine += c;
    }
  }
}

/* --------------------------------------------------------------------------
 *  MEASURE / SWEEP
 * ------------------------------------------------------------------------ */

static void measure() {
  int distance = 0;
  float cm = pingCm();
  if (cm > 0.0f) distance = (int)(cm + 0.5f);

  lastMeasurement = distance;
  St7735UI::addReading(distance, angle);
  sendTelemetry(distance);
}

static void takeStep() {
  angle += dir * VIRTUAL_STEP_DEG;
  if (angle > 180) { angle = 360 - angle; dir = -1; }
  if (angle < 0)   { angle = -angle;     dir =  1; }
  measure();
}

/* --------------------------------------------------------------------------
 *  SETUP / LOOP
 * ------------------------------------------------------------------------ */

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("==========================================");
  Serial.println(" DETEKTOR AIR LACAK - Bluetooth build");
  Serial.printf(" Version %s\n", FW_VERSION);
  Serial.println("==========================================");

  if (!SerialBT.begin(BT_NAME)) {
    Serial.println("Bluetooth init failed");
  } else {
    SerialBT.setPin(BT_PIN);
    Serial.printf("Bluetooth SPP: %s  PIN %s\n", BT_NAME, BT_PIN);
  }

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  loadConfig();

  Proximity::begin();
  Buzzer::begin();

  St7735UI::setConfig(maxRangeCm, alertCm);
  St7735UI::setProximity(Proximity::count());
  St7735UI::begin();

  Serial.println("Ready.");
}

void loop() {
  if (SerialBT.hasClient()) {
    if (!clientWasConnected) {
      clientWasConnected = true;
      sendConfig();
    }
  } else {
    clientWasConnected = false;
  }

  handleBluetooth();
  Buzzer::loop();

  if (Proximity::poll()) {
    Buzzer::trigger();
    sendProx();
  }
  if (Proximity::pollReset()) {
    sendProx();
  }

  uint32_t now = millis();
  if ((uint32_t)(now - lastStepMs) >= STEP_MS) {
    lastStepMs = now;
    takeStep();
  }

  St7735UI::loop();
}
