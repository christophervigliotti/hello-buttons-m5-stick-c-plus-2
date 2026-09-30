#include "Sound.h"

#include <M5Unified.h>

#include "Buttons.h"

namespace stickui {

ButtonSounds buttonSounds;

void beginSound() {
  M5.Speaker.setVolume(kSpeakerVolume);
  M5.Speaker.setChannelVolume(kTypingSoundChannel, kTypingSoundVolume);
  M5.Speaker.setChannelVolume(kButtonSoundChannel, kButtonSoundVolume);
}

void playTypingClick(char character) {
  if (!kTypingClicksEnabled || !settings.soundEnabled || character == ' ') {
    return;
  }
  M5.Speaker.tone(kTypingClickHz, kClickDurationMs, kTypingSoundChannel);
}

void playPattern(const ClickPattern& pattern) {
  if (!settings.soundEnabled || !pattern.enabled || pattern.pulseCount == 0) {
    return;
  }

  for (uint8_t pulse = 0; pulse < pattern.pulseCount; ++pulse) {
    M5.Speaker.tone(pattern.frequencyHz, pattern.durationMs, kButtonSoundChannel);
    const uint32_t startedAtMs = millis();
    while (M5.Speaker.isPlaying(kButtonSoundChannel) &&
           millis() - startedAtMs < pattern.durationMs + 100u) {
      delay(1);
    }
    if (pulse + 1 < pattern.pulseCount) {
      delay(pattern.gapMs);
    }
  }
}

void playRedrawScreenButtonSounds() {
  const bool frontPressed = M5.BtnA.isPressed();
  const bool topPressed = M5.BtnB.isPressed();
  if ((M5.BtnA.wasPressed() && topPressed) || (M5.BtnB.wasPressed() && frontPressed)) {
    playPattern(buttonSounds.both);
  } else if ((M5.BtnA.wasHold() && !topPressed) || (M5.BtnB.wasHold() && !frontPressed)) {
    playPattern(buttonSounds.longPress);
  }
}

}  // namespace stickui
