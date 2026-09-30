#pragma once

#include <Arduino.h>

#include "Buttons.h"

namespace stickui {

// Selection state for a prompt with a fixed list of options (yes/cancel, yes/no, a list of
// shows). A tap moves the selection to the next option, wrapping; a double tap chooses.
struct ChoicePrompt {
  size_t optionCount = 2;
  size_t selectedIndex = 0;
  bool waitingForButtonsRelease = true;
  bool pressInProgress = false;
  bool waitingForSecondTap = false;
  bool secondTapInProgress = false;
  Button firstTapButton = Button::Face;
  Button pressedButton = Button::Face;
  uint32_t firstReleaseAtMs = 0;

  void selectNext() {
    if (optionCount > 0) {
      selectedIndex = (selectedIndex + 1) % optionCount;
    }
  }
};

enum class ChoiceResult { None, Chosen };

// Call once per frame. A tap moves the selection (once the double-tap window passes), a
// double tap chooses it: returns Chosen with prompt.selectedIndex set. Also mirrors presses
// on the header indicators and plays button sounds. Sets `redraw` when the prompt needs
// drawing.
ChoiceResult processChoiceTaps(ChoicePrompt& prompt, const ButtonInput& input, bool& redraw);

}  // namespace stickui
