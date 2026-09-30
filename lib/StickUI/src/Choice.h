#pragma once

#include <Arduino.h>

#include "Buttons.h"

namespace stickui {

// Selection state for a two-option prompt (yes/cancel, yes/no).
struct ChoicePrompt {
  bool waitingForButtonsRelease = true;
  bool yesSelected = true;
  bool pressInProgress = false;
  bool waitingForSecondTap = false;
  bool secondTapInProgress = false;
  Button firstTapButton = Button::Face;
  Button pressedButton = Button::Face;
  uint32_t firstReleaseAtMs = 0;
};

enum class ChoiceResult { None, Yes, No };

// Call once per frame. A tap toggles the selection (once the double-tap window passes), a
// double tap chooses it. Also mirrors presses on the header indicators and plays button
// sounds. Sets `redraw` when the prompt needs drawing.
ChoiceResult processChoiceTaps(ChoicePrompt& prompt, const ButtonInput& input, bool& redraw);

}  // namespace stickui
