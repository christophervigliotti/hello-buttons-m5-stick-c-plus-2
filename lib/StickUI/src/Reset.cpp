#include "Reset.h"

#include <M5Unified.h>

#include "Choice.h"
#include "Header.h"
#include "Orientation.h"
#include "Sound.h"
#include "Text.h"
#include "Typewriter.h"

namespace stickui {

namespace {

const char kResetQuestion[] = "reset app?";
const char kResetYes[] = "yes";
const char kResetCancel[] = "cancel";

ChoicePrompt resetPrompt;
Typewriter resetTyping;
bool resetTypingStarted = false;

void showResetAppPrompt() {
  drawScreenFrame(!resetPrompt.screenInitialized);
  resetPrompt.screenInitialized = true;
  size_t remaining = resetTyping.visible();
  const String question = kResetQuestion;
  drawCenteredLine(question, takeVisible(remaining, question.length()), contentLineY(0), WHITE);
  takeVisible(remaining, 1);
  drawChoiceLine(kResetYes, kResetCancel, resetPrompt.yesSelected, remaining, contentLineY(2));
}

}  // namespace

void askResetApp() {
  resetPrompt = ChoicePrompt{};
  resetPrompt.active = true;
  resetTypingStarted = false;
}

bool isResetPromptActive() {
  return resetPrompt.active;
}

ResetPromptOutcome updateResetPrompt() {
  bool redraw = false;
  if (handleHeldOrientation()) {
    resetPrompt.screenInitialized = false;
    redraw = true;
  }
  if (!resetTypingStarted) {
    resetTyping.start(String(kResetQuestion) + " " + kResetYes + kChoiceSpacer + kResetCancel, 1);
    resetTypingStarted = true;
    redraw = true;
  }

  const ChoiceResult result = processChoiceTaps(resetPrompt, redraw);
  if (result == ChoiceResult::Yes) {
    delay(kAfterUserInputDelayMs);
    restartApp();
  }
  if (result == ChoiceResult::No) {
    resetPrompt.active = false;
    return ResetPromptOutcome::Cancelled;
  }

  if (resetTyping.advance()) {
    redraw = true;
  }
  if (redraw) {
    showResetAppPrompt();
  }
  return ResetPromptOutcome::Open;
}

bool bothButtonsHeldLong(uint32_t& bothButtonsDownAtMs) {
  if (!M5.BtnA.isPressed() || !M5.BtnB.isPressed()) {
    bothButtonsDownAtMs = 0;
    return false;
  }
  if (bothButtonsDownAtMs == 0) {
    bothButtonsDownAtMs = millis();
    return false;
  }
  if (millis() - bothButtonsDownAtMs < kLongPressThresholdMs) {
    return false;
  }
  bothButtonsDownAtMs = 0;
  return true;
}

void runResetPromptModally() {
  playPattern(buttonSounds.bothLong);
  askResetApp();
  while (isResetPromptActive()) {
    M5.update();
    if (updateResetPrompt() == ResetPromptOutcome::Cancelled) {
      indicators.reset();
    }
    delay(20);
  }
}

void restartApp() {
  showFullScreenMessage("restarting", kRestartingScreenMs);
  ESP.restart();
}

}  // namespace stickui
