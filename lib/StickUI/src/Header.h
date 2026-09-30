#pragma once

#include <Arduino.h>

#include "Buttons.h"

namespace stickui {

// Header arrows: dot = ready, outline = pressed, filled = long press, double = double tap.
enum class IndicatorState { Ready, Pressed, LongPressed, DoublePressed };

struct Indicators {
  IndicatorState face = IndicatorState::Ready;  // right, green
  IndicatorState side = IndicatorState::Ready;  // left, red

  void reset() {
    face = IndicatorState::Ready;
    side = IndicatorState::Ready;
  }

  IndicatorState& forButton(Button button) { return button == Button::Face ? face : side; }
};

extern Indicators indicators;

void setAppTitle(const char* title);

// Draws the title, both indicators and the header line. The title and indicators type in
// (with clicks) the first time. clearScreen=true starts a fresh full-screen redraw.
void drawScreenFrame(bool clearScreen = true);

bool isHeaderTypingComplete();

}  // namespace stickui
