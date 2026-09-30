#include "Setup.h"

#include <M5Unified.h>

#include "Buttons.h"
#include "Choice.h"
#include "Header.h"
#include "Orientation.h"
#include "Reset.h"
#include "Sound.h"
#include "Text.h"
#include "Typewriter.h"

namespace stickui {

namespace {

const char kSoundQuestion[] = "sound?";
const char kCalibrationProgress[] = "once then again";

String configLabel(uint8_t screenNumber) {
  return String("config ") + screenNumber + " of " + kConfigScreenCount;
}

void showCalibrationPrompt(const String& label,
                           const String& instruction,
                           const String& progress,
                           uint8_t highlightedTap,
                           size_t visibleCharacters,
                           bool clearScreen) {
  drawScreenFrame(clearScreen);
  size_t remaining = visibleCharacters;
  drawCenteredLine(label, takeVisible(remaining, label.length()), contentLineY(0), kGreyText);
  drawCenteredLine(instruction, takeVisible(remaining, instruction.length()), contentLineY(1), WHITE);
  const size_t visibleProgress = takeVisible(remaining, progress.length());
  drawCenteredLine(progress, visibleProgress, contentLineY(2), kGreyText);

  const String highlight = highlightedTap == 1 ? "once" : "again";
  const int highlightIndex = progress.indexOf(highlight);
  if (highlightIndex >= 0 && visibleProgress >= highlightIndex + highlight.length()) {
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.drawString(highlight,
                      centeredX(progress) + M5.Lcd.textWidth(progress.substring(0, highlightIndex).c_str()),
                      contentLineY(2));
  }
}

void showSoundPrompt(const ChoicePrompt& prompt, const String& label, size_t visibleCharacters,
                     bool clearScreen) {
  drawScreenFrame(clearScreen);
  size_t remaining = visibleCharacters;
  const String question = kSoundQuestion;
  drawCenteredLine(label, takeVisible(remaining, label.length()), contentLineY(0), kGreyText);
  drawCenteredLine(question, takeVisible(remaining, question.length()), contentLineY(1), WHITE);
  drawChoiceLine("yes", "no", prompt.yesSelected, remaining, contentLineY(2));
}

// "tap <button>" / "once then again". Returns the measured gap plus an allowance.
uint16_t calibrateDoubleTapWindow(Button calibratedButton, uint8_t screenNumber) {
  m5::Button_Class& button = hardwareButton(calibratedButton);
  m5::Button_Class& other = hardwareButton(otherButton(calibratedButton));
  bool firstTapDown = false;
  bool awaitingSecondTap = false;
  bool secondTapDown = false;
  uint8_t highlightedTap = 1;
  uint32_t firstReleaseAtMs = 0;
  uint32_t measuredGapMs = 0;
  uint32_t bothButtonsDownAtMs = 0;
  const String instruction = String("tap ") + buttonName(calibratedButton);
  const String progress = kCalibrationProgress;
  const String label = configLabel(screenNumber);
  Typewriter typing;
  typing.start(label + instruction + progress);

  showCalibrationPrompt(label, instruction, progress, highlightedTap, typing.visible(), true);

  while (true) {
    M5.update();
    bool shouldRedraw = false;
    bool calibrationComplete = false;
    const bool wasPressed = button.wasPressed();
    const bool wasReleased = button.wasReleased();
    const bool wasHeld = button.wasHold();
    (void)other.wasPressed();
    (void)other.wasReleased();
    (void)other.wasHold();
    playRedrawScreenButtonSounds();
    if (handleHeldOrientation()) {
      showCalibrationPrompt(label, instruction, progress, highlightedTap,
                            isHeaderTypingComplete() ? typing.visible() : 0, true);
    }

    IndicatorState& calibrationIndicator = indicators.forButton(calibratedButton);
    if (wasPressed) {
      calibrationIndicator = IndicatorState::Pressed;
      shouldRedraw = true;
    }
    if (wasHeld) {
      calibrationIndicator = IndicatorState::LongPressed;
      shouldRedraw = true;
    }
    if (wasReleased) {
      calibrationIndicator = IndicatorState::Ready;
      shouldRedraw = true;
    }

    if (!isHeaderTypingComplete()) {
      showCalibrationPrompt(label, instruction, progress, highlightedTap, 0, false);
      typing.restartClock();
      delay(10);
      continue;
    }

    if (bothButtonsHeldLong(bothButtonsDownAtMs)) {
      runResetPromptModally();
      firstTapDown = false;
      awaitingSecondTap = false;
      secondTapDown = false;
      highlightedTap = 1;
      showCalibrationPrompt(label, instruction, progress, highlightedTap, typing.visible(), true);
      delay(10);
      continue;
    }

    if (awaitingSecondTap && millis() - firstReleaseAtMs > kMaximumCalibrationGapMs) {
      awaitingSecondTap = false;
      highlightedTap = 1;
      shouldRedraw = true;
    }

    if (wasHeld) {
      firstTapDown = false;
      awaitingSecondTap = false;
      secondTapDown = false;
      highlightedTap = 1;
      shouldRedraw = true;
    }

    if (wasPressed) {
      if (awaitingSecondTap) {
        measuredGapMs = millis() - firstReleaseAtMs;
        if (measuredGapMs <= kMaximumCalibrationGapMs) {
          secondTapDown = true;
          highlightedTap = 2;
        } else {
          awaitingSecondTap = false;
          firstTapDown = true;
          highlightedTap = 1;
        }
        shouldRedraw = true;
      } else if (!firstTapDown) {
        firstTapDown = true;
      }
    }

    if (wasReleased) {
      if (secondTapDown) {
        secondTapDown = false;
        calibrationComplete = true;
        playPattern(buttonSounds.doubleTap);
        highlightedTap = 2;
        shouldRedraw = true;
      } else if (firstTapDown) {
        firstTapDown = false;
        awaitingSecondTap = true;
        firstReleaseAtMs = millis();
        highlightedTap = 2;
        shouldRedraw = true;
      }
    }

    if (typing.advance()) {
      shouldRedraw = true;
    }
    if (shouldRedraw) {
      showCalibrationPrompt(label, instruction, progress, highlightedTap, typing.visible(), false);
    }
    if (calibrationComplete) {
      delay(kAfterUserInputDelayMs);
      return static_cast<uint16_t>(measuredGapMs + kDoubleTapAllowanceMs);
    }
    delay(10);
  }
}

// "sound?" / "yes   no". Returns whether sound should be on.
bool configureSound(uint8_t screenNumber) {
  ChoicePrompt prompt;
  prompt.active = true;
  const String label = configLabel(screenNumber);
  Typewriter typing;
  typing.start(label + kSoundQuestion + "yes" + kChoiceSpacer + "no");
  uint32_t bothButtonsDownAtMs = 0;

  showSoundPrompt(prompt, label, typing.visible(), true);
  while (true) {
    M5.update();
    if (handleHeldOrientation()) {
      showSoundPrompt(prompt, label, typing.visible(), true);
    }
    if (bothButtonsHeldLong(bothButtonsDownAtMs)) {
      runResetPromptModally();
      prompt = ChoicePrompt{};
      prompt.active = true;
      showSoundPrompt(prompt, label, typing.visible(), true);
      delay(10);
      continue;
    }

    bool redraw = false;
    const ChoiceResult result = processChoiceTaps(prompt, redraw);
    if (result != ChoiceResult::None) {
      delay(kAfterUserInputDelayMs);
      return result == ChoiceResult::Yes;
    }

    if (typing.advance()) {
      redraw = true;
    }
    if (redraw) {
      showSoundPrompt(prompt, label, typing.visible(), false);
    }
    delay(10);
  }
}

}  // namespace

void runSetup() {
  handleHeldOrientation(true);
  showFullScreenMessage("loading", kLoadingScreenMs);

  settings.frontDoubleTapWindowMs = calibrateDoubleTapWindow(Button::Front, 1);
  settings.topDoubleTapWindowMs = calibrateDoubleTapWindow(Button::Top, 2);
  if (kSoundConfigScreenEnabled) {
    settings.soundEnabled = configureSound(3);
  }
}

}  // namespace stickui
