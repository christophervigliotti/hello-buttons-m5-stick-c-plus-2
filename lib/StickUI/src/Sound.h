#pragma once

#include <Arduino.h>

#include "Theme.h"

namespace stickui {

// A run of identical clicks on the button channel.
struct ClickPattern {
  bool enabled;
  uint16_t frequencyHz;
  uint16_t durationMs;
  uint8_t pulseCount;
  uint16_t gapMs;

  ClickPattern(bool isEnabled = false,
               uint16_t frequency = kButtonClickHz,
               uint16_t duration = kClickDurationMs,
               uint8_t count = 1,
               uint16_t gap = 30)
      : enabled(isEnabled),
        frequencyHz(frequency),
        durationMs(duration),
        pulseCount(count),
        gapMs(gap) {}
};

// Sounds for each button gesture. A resolved single tap is silent.
struct ButtonSounds {
  ClickPattern doubleTap{true, kButtonClickHz, kClickDurationMs, 2, 30};
  ClickPattern longPress{true, kButtonClickHz, kClickDurationMs, 1};
  ClickPattern both{true, kButtonClickHz, kClickDurationMs, 3, 30};
  ClickPattern bothLong{true, kButtonClickHz, kClickDurationMs, 3, 30};
};

extern ButtonSounds buttonSounds;

void beginSound();

// Typing tick on the typing channel; never for spaces.
void playTypingClick(char character = '\0');

// Blocks until each pulse has actually finished playing so nothing cuts it off.
void playPattern(const ClickPattern& pattern);

// Setup and prompt screens handle taps themselves; this adds the long and both-button sounds.
void playRedrawScreenButtonSounds();

}  // namespace stickui
