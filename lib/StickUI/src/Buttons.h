#pragma once

#include <M5Unified.h>

#include "Theme.h"

namespace stickui {

// The two user buttons: BtnA on the front face, BtnB on the top edge.
enum class Button { Front, Top };

struct Settings {
  uint16_t frontDoubleTapWindowMs = kDefaultDoubleTapWindowMs;
  uint16_t topDoubleTapWindowMs = kDefaultDoubleTapWindowMs;
  bool soundEnabled = true;
};

extern Settings settings;

inline m5::Button_Class& hardwareButton(Button button) {
  return button == Button::Front ? M5.BtnA : M5.BtnB;
}

inline Button otherButton(Button button) {
  return button == Button::Front ? Button::Top : Button::Front;
}

inline const char* buttonName(Button button) {
  return button == Button::Front ? "front" : "top";
}

inline uint16_t doubleTapWindowMs(Button button) {
  return button == Button::Front ? settings.frontDoubleTapWindowMs : settings.topDoubleTapWindowMs;
}

}  // namespace stickui
