#pragma once

/*
 * MAX98357A I2S audio player for the embedded proximity alert.
 *
 * Pins: BCLK 19, LRC 21, DIN 22, VIN 5 V, GND common.
 * Uses the embedded MP3 by default; set PROX_AUDIO_USE_MP3 to 0 for the WAV.
 *
 * ESP8266Audio is GPL-3.0. The project source is distributed with this
 * firmware, so the combined binary is distributed under GPL-3.0.
 */

#include <Arduino.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>

#include "audio_prox.h"

#define PROX_AUDIO_USE_MP3 1

namespace ProxAudio {

static AudioOutputI2S *g_out = nullptr;
static AudioGenerator *g_gen = nullptr;
static AudioFileSourcePROGMEM *g_src = nullptr;

inline void begin() {
  if (g_out) return;
  g_out = new AudioOutputI2S();
  g_out->SetPinout(19, 21, 22);   // BCLK, LRC, DIN
  g_out->SetGain(0.55f);
}

inline void stop() {
  if (g_gen) {
    if (g_gen->isRunning()) g_gen->stop();
    delete g_gen;
    g_gen = nullptr;
  }
  if (g_src) {
    if (g_src->isOpen()) g_src->close();
    delete g_src;
    g_src = nullptr;
  }
}

inline bool playAlert() {
  if (!g_out) begin();
  stop();

#if PROX_AUDIO_USE_MP3
  g_src = new AudioFileSourcePROGMEM(PROX_ALERT_MP3, PROX_ALERT_MP3_LEN);
  g_gen = new AudioGeneratorMP3();
#else
  g_src = new AudioFileSourcePROGMEM(PROX_ALERT_WAV, PROX_ALERT_WAV_LEN);
  g_gen = new AudioGeneratorWAV();
#endif

  if (!g_gen->begin(g_src, g_out)) {
    stop();
    return false;
  }
  return true;
}

inline void loop() {
  if (g_gen && g_gen->isRunning()) {
    if (!g_gen->loop()) stop();
  }
}

inline bool isPlaying() {
  return g_gen && g_gen->isRunning();
}

}  // namespace ProxAudio
