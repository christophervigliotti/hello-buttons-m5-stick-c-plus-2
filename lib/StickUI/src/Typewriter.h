#pragma once

#include <Arduino.h>

#include "Sound.h"
#include "Theme.h"

namespace stickui {

// Reveals text one character per kTypingIntervalMs, clicking for each non-space character.
// Screens with several lines type one combined string and split the visible count with
// takeVisible() (see Text.h).
class Typewriter {
 public:
  explicit Typewriter(const String& text = "") : text_(text), visible_(text.length()) {}

  // Replaces the text, showing `visibleCharacters` of it right away, and restarts the clock.
  void start(const String& text, size_t visibleCharacters = 0) {
    text_ = text;
    visible_ = min(visibleCharacters, text.length());
    lastCharacterAtMs_ = millis();
  }

  // Delays the next character by a full interval from now.
  void restartClock() { lastCharacterAtMs_ = millis(); }

  // Reveals the next character if its interval has passed; returns whether one appeared.
  bool advance() {
    if (!isTyping() || millis() - lastCharacterAtMs_ < kTypingIntervalMs) {
      return false;
    }
    ++visible_;
    lastCharacterAtMs_ = millis();
    playTypingClick(text_[visible_ - 1]);
    return true;
  }

  const String& text() const { return text_; }
  size_t visible() const { return visible_; }
  bool isTyping() const { return visible_ < text_.length(); }

 private:
  String text_;
  size_t visible_;
  uint32_t lastCharacterAtMs_ = 0;
};

}  // namespace stickui
