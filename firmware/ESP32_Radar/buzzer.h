#pragma once

/*
 * v2.0.1 GPIO23 buzzer pattern.
 *
 * Active buzzer (default): HIGH = sound.
 * Passive buzzer: tone(PIN, BUZZER_TONE_HZ) / noTone(PIN).
 *
 * Idle pattern:   longOn  ON / longOff  OFF (default 1000 / 1500 ms)
 * Alert pattern:  shortOn ON / shortOff OFF (default 500 / 750 ms)
 *
 * The alert pattern runs for `alertHold` ms after each proximity pulse, then
 * the long idle pattern resumes. All timings are stored in NVS and adjustable
 * from the web UI.
 */

#include <Arduino.h>
#include <Preferences.h>

#ifndef PIN_BUZZER
#define PIN_BUZZER 23
#endif

#define BUZZER_TONE_HZ 2200

namespace Buzzer {

static Preferences g_prefs;

static bool     g_ready = false;
static bool     g_enabled = true;
static bool     g_active = true;          // true = active buzzer
static bool     g_on = false;

static uint32_t g_longOn = 1000;
static uint32_t g_longOff = 1500;
static uint32_t g_shortOn = 500;
static uint32_t g_shortOff = 750;
static uint32_t g_alertHold = 5000;

static uint32_t g_alertUntil = 0;
static uint32_t g_phaseMs = 0;

static void output(bool on) {
  if (g_active) {
    digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
  } else {
    if (on) tone(PIN_BUZZER, BUZZER_TONE_HZ);
    else    noTone(PIN_BUZZER);
  }
}

static uint32_t clampMs(uint32_t value, uint32_t minValue, uint32_t maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

inline void save() {
  if (!g_ready) return;
  g_prefs.putBool("bz_en", g_enabled);
  g_prefs.putBool("bz_act", g_active);
  g_prefs.putUInt("bz_lo", g_longOn);
  g_prefs.putUInt("bz_lf", g_longOff);
  g_prefs.putUInt("bz_so", g_shortOn);
  g_prefs.putUInt("bz_sf", g_shortOff);
  g_prefs.putUInt("bz_hd", g_alertHold);
}

inline void begin() {
  pinMode(PIN_BUZZER, OUTPUT);
  g_active = true;
  output(false);

  g_prefs.begin("lacak", false);
  g_enabled  = g_prefs.getBool("bz_en", true);
  g_active   = g_prefs.getBool("bz_act", true);
  g_longOn   = clampMs(g_prefs.getUInt("bz_lo", 1000), 50, 60000);
  g_longOff  = clampMs(g_prefs.getUInt("bz_lf", 1500), 50, 60000);
  g_shortOn  = clampMs(g_prefs.getUInt("bz_so", 500),  50, 60000);
  g_shortOff = clampMs(g_prefs.getUInt("bz_sf", 750),  50, 60000);
  g_alertHold = clampMs(g_prefs.getUInt("bz_hd", 5000), 0, 600000);

  g_ready = true;
  g_phaseMs = millis();
  g_on = g_enabled;
  output(g_on);
}

inline bool enabled() {
  return g_enabled;
}

inline bool activeType() {
  return g_active;
}

inline uint32_t longOn()    { return g_longOn; }
inline uint32_t longOff()   { return g_longOff; }
inline uint32_t shortOn()   { return g_shortOn; }
inline uint32_t shortOff()  { return g_shortOff; }
inline uint32_t alertHold() { return g_alertHold; }

inline void setEnabled(bool enabled) {
  g_enabled = enabled;
  save();
  if (!g_enabled) {
    g_on = false;
    output(false);
  } else {
    g_on = true;
    g_phaseMs = millis();
    output(true);
  }
}

inline void setActive(bool active) {
  noTone(PIN_BUZZER);
  g_active = active;
  save();
  output(g_on && g_enabled);
}

inline void setTimings(uint32_t longOnMs, uint32_t longOffMs,
                       uint32_t shortOnMs, uint32_t shortOffMs,
                       uint32_t holdMs) {
  g_longOn    = clampMs(longOnMs, 50, 60000);
  g_longOff   = clampMs(longOffMs, 50, 60000);
  g_shortOn   = clampMs(shortOnMs, 50, 60000);
  g_shortOff  = clampMs(shortOffMs, 50, 60000);
  g_alertHold = clampMs(holdMs, 0, 600000);
  save();
  g_phaseMs = millis();
}

// Call once per proximity pulse.
inline void trigger() {
  g_alertUntil = millis() + g_alertHold;
  if (!g_enabled) return;

  // Start or restart the ON phase immediately so the short beep is audible.
  g_on = true;
  g_phaseMs = millis();
  output(true);
}

inline void loop() {
  if (!g_ready) return;

  if (!g_enabled) {
    if (g_on) {
      g_on = false;
      output(false);
    }
    return;
  }

  uint32_t now = millis();
  bool alert = (int32_t)(g_alertUntil - now) > 0;

  uint32_t onMs  = alert ? g_shortOn  : g_longOn;
  uint32_t offMs = alert ? g_shortOff : g_longOff;

  if (g_on) {
    if ((uint32_t)(now - g_phaseMs) >= onMs) {
      g_on = false;
      g_phaseMs = now;
      output(false);
    }
  } else {
    if ((uint32_t)(now - g_phaseMs) >= offMs) {
      g_on = true;
      g_phaseMs = now;
      output(true);
    }
  }
}

}  // namespace Buzzer
