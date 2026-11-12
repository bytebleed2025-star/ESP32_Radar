#pragma once

/*
 * LJ12A3-4-Z/BX NPN NO inductive proximity counter.
 *
 * Wiring: black output -> 1k series -> GPIO34
 *         GPIO34 -> 10k pull-up -> 3.3V
 *         blue -> GND, brown -> 6..36V sensor supply (common GND)
 *
 * The NPN output is HIGH when idle and LOW when metal is detected.
 */

#include <Arduino.h>
#include <Preferences.h>

#ifndef PIN_PROX
#define PIN_PROX 34
#endif
#ifndef PIN_PROX_RESET
#define PIN_PROX_RESET 33
#endif

namespace Proximity {

static Preferences g_prefs;
static uint32_t g_count = 0;
static bool g_initialized = false;
static bool g_lastLevel = true;
static bool g_stableLevel = true;
static uint32_t g_lastChangeMs = 0;
static bool g_resetBtnLast = true;
static uint32_t g_resetChangeMs = 0;
static const uint32_t DEBOUNCE_MS = 80;

inline void save() {
  if (!g_initialized) return;
  g_prefs.putUInt("prox_count", g_count);
}

inline void reset();

inline void begin() {
  pinMode(PIN_PROX, INPUT);           // external 10k pull-up required
  pinMode(PIN_PROX_RESET, INPUT_PULLUP);
  g_prefs.begin("lacak", false);
  g_count = g_prefs.getUInt("prox_count", 0);
  g_lastLevel = digitalRead(PIN_PROX);
  g_stableLevel = g_lastLevel;
  g_resetBtnLast = digitalRead(PIN_PROX_RESET);
  g_initialized = true;
}

// Returns true once for each new metal detection.
inline bool poll() {
  if (!g_initialized) return false;

  bool level = digitalRead(PIN_PROX);
  uint32_t now = millis();

  if (level != g_lastLevel) {
    g_lastLevel = level;
    g_lastChangeMs = now;
  }

  if (level != g_stableLevel && (now - g_lastChangeMs) >= DEBOUNCE_MS) {
    g_stableLevel = level;
    if (!level) {                     // NPN pulls LOW on detection
      g_count++;
      save();
      return true;
    }
  }
  return false;
}

// Returns true when the physical reset button was pressed.
inline bool pollReset() {
  if (!g_initialized) return false;
  static bool armed = true;
  bool level = digitalRead(PIN_PROX_RESET);
  uint32_t now = millis();
  if (level != g_resetBtnLast) {
    g_resetBtnLast = level;
    g_resetChangeMs = now;
  }
  if (level) {
    armed = true;
  } else if (armed && (now - g_resetChangeMs) >= 40) {
    armed = false;
    reset();
    return true;
  }
  return false;
}

inline void reset() {
  g_count = 0;
  save();
}

inline uint32_t count() {
  return g_count;
}

}  // namespace Proximity
