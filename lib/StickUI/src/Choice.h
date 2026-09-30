#pragma once

#include <Arduino.h>

#include "Buttons.h"

namespace stickui {

// Selection state for a two-option prompt (yes/cancel, yes/no).
struct ChoicePrompt {
  bool active = false;
  bool screenInitialized = false;
  bool waitingForButtonsRelease = true;
  bool yesSelected = true;
  bool pressInProgress = false;
  bool waitingForSecondTap = false;
  bool secondTapInProgress = false;
  Button firstTapButton = Button::Front;
  Button pressedButton = Button::Front;
  uint32_t firstReleaseAtMs = 0;
};

enum class ChoiceResult { None, Yes, No };

// Call once per frame after M5.update(). A tap toggles the selection (once the double-tap
// window passes), a double tap chooses it. Also mirrors presses on the header indicators
// and plays button sounds. Sets `redraw` when the prompt needs drawing.
ChoiceResult processChoiceTaps(ChoicePrompt& prompt, bool& redraw);

}  // namespace stickui
