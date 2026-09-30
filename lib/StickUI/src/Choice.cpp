#include "Choice.h"

#include "Header.h"
#include "Sound.h"

namespace stickui {

ChoiceResult processChoiceTaps(ChoicePrompt& prompt, const ButtonInput& input, bool& redraw) {
  playButtonSounds(input);
  const ButtonState& face = input.face;
  const ButtonState& side = input.side;
  const uint32_t nowMs = millis();

  if (face.wasPressed) indicators.face = IndicatorState::Pressed;
  if (face.wasHeld) indicators.face = IndicatorState::LongPressed;
  if (face.wasReleased) indicators.face = IndicatorState::Ready;
  if (side.wasPressed) indicators.side = IndicatorState::Pressed;
  if (side.wasHeld) indicators.side = IndicatorState::LongPressed;
  if (side.wasReleased) indicators.side = IndicatorState::Ready;
  if (face.wasPressed || face.wasHeld || face.wasReleased ||
      side.wasPressed || side.wasHeld || side.wasReleased) {
    redraw = true;
  }

  if (prompt.waitingForButtonsRelease) {
    if (!face.isPressed && !side.isPressed) {
      prompt.waitingForButtonsRelease = false;
      redraw = true;
    }
    return ChoiceResult::None;
  }

  if (prompt.waitingForSecondTap &&
      nowMs - prompt.firstReleaseAtMs >= doubleTapWindowMs(prompt.firstTapButton)) {
    prompt.selectNext();
    prompt.waitingForSecondTap = false;
    redraw = true;
  }

  if (face.wasHeld || side.wasHeld) {
    prompt.pressInProgress = false;
    prompt.secondTapInProgress = false;
    if (prompt.waitingForSecondTap) {
      prompt.selectNext();
      redraw = true;
    }
    prompt.waitingForSecondTap = false;
  }

  bool buttonWasTapped = false;
  Button tappedButton = Button::Face;
  if (face.wasPressed && !side.isPressed) {
    buttonWasTapped = true;
    tappedButton = Button::Face;
  } else if (side.wasPressed && !face.isPressed) {
    buttonWasTapped = true;
    tappedButton = Button::Side;
  }

  if (buttonWasTapped) {
    if (prompt.waitingForSecondTap &&
        tappedButton == prompt.firstTapButton &&
        nowMs - prompt.firstReleaseAtMs < doubleTapWindowMs(tappedButton)) {
      prompt.secondTapInProgress = true;
      prompt.pressInProgress = true;
      prompt.pressedButton = tappedButton;
    } else {
      if (prompt.waitingForSecondTap) {
        prompt.selectNext();
        redraw = true;
        prompt.waitingForSecondTap = false;
      }
      prompt.pressInProgress = true;
      prompt.secondTapInProgress = false;
      prompt.pressedButton = tappedButton;
    }
  }

  const bool matchingFaceRelease = face.wasReleased && prompt.pressedButton == Button::Face;
  const bool matchingSideRelease = side.wasReleased && prompt.pressedButton == Button::Side;
  if (prompt.pressInProgress && (matchingFaceRelease || matchingSideRelease)) {
    prompt.pressInProgress = false;
    if (prompt.secondTapInProgress) {
      playPattern(buttonSounds.doubleTap);
      return ChoiceResult::Chosen;
    }
    prompt.waitingForSecondTap = true;
    prompt.firstTapButton = prompt.pressedButton;
    prompt.firstReleaseAtMs = nowMs;
  }
  return ChoiceResult::None;
}

}  // namespace stickui
