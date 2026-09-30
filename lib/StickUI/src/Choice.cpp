#include "Choice.h"

#include <M5Unified.h>

#include "Header.h"
#include "Sound.h"

namespace stickui {

ChoiceResult processChoiceTaps(ChoicePrompt& prompt, bool& redraw) {
  playRedrawScreenButtonSounds();
  const bool frontPressed = M5.BtnA.isPressed();
  const bool topPressed = M5.BtnB.isPressed();
  const bool frontWasPressed = M5.BtnA.wasPressed();
  const bool topWasPressed = M5.BtnB.wasPressed();
  const bool frontWasReleased = M5.BtnA.wasReleased();
  const bool topWasReleased = M5.BtnB.wasReleased();
  const bool frontWasHeld = M5.BtnA.wasHold();
  const bool topWasHeld = M5.BtnB.wasHold();
  const uint32_t nowMs = millis();

  if (frontWasPressed) indicators.front = IndicatorState::Pressed;
  if (frontWasHeld) indicators.front = IndicatorState::LongPressed;
  if (frontWasReleased) indicators.front = IndicatorState::Ready;
  if (topWasPressed) indicators.top = IndicatorState::Pressed;
  if (topWasHeld) indicators.top = IndicatorState::LongPressed;
  if (topWasReleased) indicators.top = IndicatorState::Ready;
  if (frontWasPressed || frontWasHeld || frontWasReleased ||
      topWasPressed || topWasHeld || topWasReleased) {
    redraw = true;
  }

  if (prompt.waitingForButtonsRelease) {
    if (!frontPressed && !topPressed) {
      prompt.waitingForButtonsRelease = false;
      redraw = true;
    }
    return ChoiceResult::None;
  }

  if (prompt.waitingForSecondTap &&
      nowMs - prompt.firstReleaseAtMs >= doubleTapWindowMs(prompt.firstTapButton)) {
    prompt.yesSelected = !prompt.yesSelected;
    prompt.waitingForSecondTap = false;
    redraw = true;
  }

  if (frontWasHeld || topWasHeld) {
    prompt.pressInProgress = false;
    prompt.secondTapInProgress = false;
    if (prompt.waitingForSecondTap) {
      prompt.yesSelected = !prompt.yesSelected;
      redraw = true;
    }
    prompt.waitingForSecondTap = false;
  }

  bool buttonWasTapped = false;
  Button tappedButton = Button::Front;
  if (frontWasPressed && !topPressed) {
    buttonWasTapped = true;
    tappedButton = Button::Front;
  } else if (topWasPressed && !frontPressed) {
    buttonWasTapped = true;
    tappedButton = Button::Top;
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
        prompt.yesSelected = !prompt.yesSelected;
        redraw = true;
        prompt.waitingForSecondTap = false;
      }
      prompt.pressInProgress = true;
      prompt.secondTapInProgress = false;
      prompt.pressedButton = tappedButton;
    }
  }

  const bool matchingFrontRelease = frontWasReleased && prompt.pressedButton == Button::Front;
  const bool matchingTopRelease = topWasReleased && prompt.pressedButton == Button::Top;
  if (prompt.pressInProgress && (matchingFrontRelease || matchingTopRelease)) {
    prompt.pressInProgress = false;
    if (prompt.secondTapInProgress) {
      playPattern(buttonSounds.doubleTap);
      return prompt.yesSelected ? ChoiceResult::Yes : ChoiceResult::No;
    }
    prompt.waitingForSecondTap = true;
    prompt.firstTapButton = prompt.pressedButton;
    prompt.firstReleaseAtMs = nowMs;
  }
  return ChoiceResult::None;
}

}  // namespace stickui
