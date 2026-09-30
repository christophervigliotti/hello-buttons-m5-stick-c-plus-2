#pragma once

#include <M5Unified.h>

#include "Theme.h"

namespace stickui {

// The two user buttons, named so they make sense in any orientation:
// Face is the big button on the screen's face (BtnA), Side is the one on the edge (BtnB).
enum class Button { Face, Side };

struct Settings {
  uint16_t faceDoubleTapWindowMs = kDefaultDoubleTapWindowMs;
  uint16_t sideDoubleTapWindowMs = kDefaultDoubleTapWindowMs;
  bool soundEnabled = true;
};

extern Settings settings;

inline m5::Button_Class& hardwareButton(Button button) {
  return button == Button::Face ? M5.BtnA : M5.BtnB;
}

inline Button otherButton(Button button) {
  return button == Button::Face ? Button::Side : Button::Face;
}

inline const char* buttonName(Button button) {
  return button == Button::Face ? "face" : "side";
}

inline uint16_t doubleTapWindowMs(Button button) {
  return button == Button::Face ? settings.faceDoubleTapWindowMs : settings.sideDoubleTapWindowMs;
}

inline void setDoubleTapWindowMs(Button button, uint16_t windowMs) {
  (button == Button::Face ? settings.faceDoubleTapWindowMs : settings.sideDoubleTapWindowMs) = windowMs;
}

// One button's events for the current frame.
struct ButtonState {
  bool isPressed = false;
  bool wasPressed = false;
  bool wasReleased = false;
  bool wasHeld = false;  // the long-press threshold was reached this frame
};

// Everything a view needs to know about the buttons this frame (filled in by the App).
struct ButtonInput {
  ButtonState face;
  ButtonState side;
  // True on the one frame where both buttons have each been held past the long-press
  // threshold. Fires once per hold; releasing either button re-arms it.
  bool bothHeldLong = false;

  const ButtonState& operator[](Button button) const {
    return button == Button::Face ? face : side;
  }
};

}  // namespace stickui
