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
const char* appTitle();

// Makes the title and indicators type in again on the next drawScreenFrame(true).
void restartHeaderTyping();

// Draws the title bar: the title, the button helpers (both indicators) and the line. The
// title and helpers type in (with clicks) the first time. clearScreen=true starts a fresh
// full-screen redraw. withButtonHelpers=false leaves the indicators out.
void drawScreenFrame(bool clearScreen = true, bool withButtonHelpers = true);

bool isHeaderTypingComplete();

}  // namespace stickui
