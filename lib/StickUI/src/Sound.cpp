#include "Sound.h"

#include <M5Unified.h>

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

void playButtonSounds(const ButtonInput& input) {
  if ((input.face.wasPressed && input.side.isPressed) || (input.side.wasPressed && input.face.isPressed)) {
    playPattern(buttonSounds.both);
  } else if ((input.face.wasHeld && !input.side.isPressed) || (input.side.wasHeld && !input.face.isPressed)) {
    playPattern(buttonSounds.longPress);
  }
}

}  // namespace stickui
