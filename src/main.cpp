#include <Arduino.h>
#include <M5Unified.h>
#include "interactivity_state.h"

InteractivityState interactivityState;

namespace {

// For this project we use the two primary user buttons on the device.
const uint32_t kStatusResetDelayMs = 1000;
const uint32_t kButtonPulseIntervalMs = 250;
const uint16_t kRedIndicator = 0xF800;
const uint16_t kGreenIndicator = 0x07E0;
const uint32_t kLongPressThresholdMs = 900;
const int kTitleY = 10;
const int kHeaderLineY = 40;
const int kContentFirstLineY = 50;
const int kContentLeftX = 12;
const uint16_t kDoubleTapAllowanceMs = 100;
const uint16_t kMaximumCalibrationGapMs = 1000;
const uint16_t kDefaultDoubleTapWindowMs = 300;
const uint32_t kCalibrationTimeoutMs = 10000;
const uint16_t kCalibrationCharacterIntervalMs = 25;
const uint16_t kCalibrationAfterUserInputDelayMs = 350;
const char kAppTitle[] = "helloButtons";
const char kResetPromptText[] = "reset app? yes   cancel";

struct HeaderTyping {
  size_t visibleTokens = 0;
  uint32_t lastTokenAtMs = 0;
  bool started = false;
};

struct ResetPrompt {
  bool active = false;
  bool waitingForButtonsRelease = true;
  bool yesSelected = true;
  bool pressInProgress = false;
  bool waitingForSecondTap = false;
  bool secondTapInProgress = false;
  uint8_t firstTapButton = 0;
  uint8_t pressedButton = 0;
  uint32_t firstReleaseAtMs = 0;
};

HeaderTyping headerTyping;
ResetPrompt resetPrompt;

void doResetApp();
void askResetApp();

struct TapTracker {
  bool pressInProgress = false;
  bool waitingForSecondTap = false;
  bool secondTapInProgress = false;
  uint32_t firstReleaseAtMs = 0;

  void reset() {
    pressInProgress = false;
    waitingForSecondTap = false;
    secondTapInProgress = false;
    firstReleaseAtMs = 0;
  }
};

void drawButtonIndicator(int x, uint16_t color, IndicatorState state, bool pointRight) {
  if (state == IndicatorState::Ready) {
    M5.Lcd.setTextColor(color, BLACK);
    M5.Lcd.drawString(".", x, kTitleY);
  } else {
    const int centerY = kTitleY + 9;
    const int radius = 6;
    const int tipX = x + (pointRight ? radius : -radius);
    const int baseX = x - (pointRight ? radius : -radius);
    const int topY = centerY - radius;
    const int bottomY = centerY + radius;
    if (state == IndicatorState::Pressed) {
      M5.Lcd.drawTriangle(tipX, centerY, baseX, topY, baseX, bottomY, color);
    } else {
      M5.Lcd.fillTriangle(tipX, centerY, baseX, topY, baseX, bottomY, color);
    }
  }
}

void drawScreenFrame() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(0x7A7A7A, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextDatum(TL_DATUM);
  const size_t titleLength = sizeof(kAppTitle) - 1;
  const size_t totalHeaderTokens = titleLength + 2;
  const uint32_t nowMs = millis();
  if (!headerTyping.started) {
    headerTyping.started = true;
    headerTyping.lastTokenAtMs = nowMs;
  }
  while (headerTyping.visibleTokens < totalHeaderTokens &&
         nowMs - headerTyping.lastTokenAtMs >= interactivityState.textIndicators.characterIntervalMs) {
    ++headerTyping.visibleTokens;
    headerTyping.lastTokenAtMs += interactivityState.textIndicators.characterIntervalMs;
  }

  const size_t visibleTitleCharacters = min(headerTyping.visibleTokens, titleLength);
  const String visibleTitle = String(kAppTitle).substring(0, visibleTitleCharacters);
  const int titleX = (M5.Lcd.width() - M5.Lcd.textWidth(kAppTitle)) / 2;
  M5.Lcd.drawString(visibleTitle, titleX, kTitleY);

  const int leftIndicatorX = 22;
  const int rightIndicatorX = M5.Lcd.width() - leftIndicatorX;
  if (headerTyping.visibleTokens > titleLength) {
    drawButtonIndicator(leftIndicatorX, kRedIndicator, interactivityState.circleIndicators.top, false);
  }
  if (headerTyping.visibleTokens > titleLength + 1) {
    drawButtonIndicator(rightIndicatorX, kGreenIndicator, interactivityState.circleIndicators.front, true);
  }

  M5.Lcd.drawFastHLine(0, kHeaderLineY, M5.Lcd.width(), 0x7A7A7A);
}

bool isHeaderTypingComplete() {
  return headerTyping.visibleTokens >= sizeof(kAppTitle) - 1 + 2;
}

void showTextIndicator(int yPos = kContentFirstLineY) {
  drawScreenFrame();
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextDatum(TL_DATUM);
  const String visibleMessage = interactivityState.textIndicators.message.substring(
      0, interactivityState.textIndicators.visibleCharacters);
  M5.Lcd.drawString(visibleMessage, kContentLeftX, yPos);
}

bool isTextIndicatorTyping() {
  return interactivityState.textIndicators.visibleCharacters <
         interactivityState.textIndicators.message.length();
}

void setTextIndicatorMessage(const String& message, bool restartTyping = false) {
  TextIndicators& textIndicators = interactivityState.textIndicators;
  if (textIndicators.message == message && !restartTyping) {
    return;
  }

  textIndicators.message = message;
  textIndicators.visibleCharacters = message.isEmpty() ? 0 : 1;
  textIndicators.lastCharacterAtMs = millis();
}

bool advanceTextIndicatorTyping() {
  TextIndicators& textIndicators = interactivityState.textIndicators;
  if (!isTextIndicatorTyping() ||
      millis() - textIndicators.lastCharacterAtMs < textIndicators.characterIntervalMs) {
    return false;
  }

  ++textIndicators.visibleCharacters;
  textIndicators.lastCharacterAtMs = millis();
  return true;
}

const StateBeepConfig& beepConfigForState(InteractivityStateKind state) {
  switch (state) {
    case InteractivityStateKind::Ready:
      return interactivityState.beeps.ready;
    case InteractivityStateKind::FrontPending:
      return interactivityState.beeps.frontPending;
    case InteractivityStateKind::Front:
      return interactivityState.beeps.front;
    case InteractivityStateKind::FrontDouble:
      return interactivityState.beeps.frontDouble;
    case InteractivityStateKind::FrontLong:
      return interactivityState.beeps.frontLong;
    case InteractivityStateKind::TopPending:
      return interactivityState.beeps.topPending;
    case InteractivityStateKind::Top:
      return interactivityState.beeps.top;
    case InteractivityStateKind::TopDouble:
      return interactivityState.beeps.topDouble;
    case InteractivityStateKind::TopLong:
      return interactivityState.beeps.topLong;
    case InteractivityStateKind::Both:
      return interactivityState.beeps.both;
    case InteractivityStateKind::BothLong:
      return interactivityState.beeps.bothLong;
    case InteractivityStateKind::ResetConfirmation:
      return interactivityState.beeps.resetConfirmation;
  }
  return interactivityState.beeps.ready;
}

void playStateBeep(InteractivityStateKind state) {
  const StateBeepConfig& config = beepConfigForState(state);
  if (!config.enabled || config.pulseCount == 0) {
    return;
  }

  for (uint8_t pulse = 0; pulse < config.pulseCount; ++pulse) {
    M5.Speaker.tone(config.frequencyHz, config.durationMs);
    delay(config.durationMs);
    M5.Speaker.stop();
    if (pulse + 1 < config.pulseCount) {
      delay(config.gapMs);
    }
  }
}

bool setInteractivityState(InteractivityStateKind state) {
  if (interactivityState.state == state) {
    return false;
  }
  interactivityState.state = state;
  playStateBeep(state);
  if (state == InteractivityStateKind::BothLong && interactivityState.onDoubleLong != nullptr) {
    interactivityState.onDoubleLong();
  }
  return true;
}

void showReadyState() {
  const bool stateChanged = setInteractivityState(InteractivityStateKind::Ready);
  interactivityState.circleIndicators.front = IndicatorState::Ready;
  interactivityState.circleIndicators.top = IndicatorState::Ready;
  interactivityState.frontLongPressActive = false;
  interactivityState.topLongPressActive = false;
  setTextIndicatorMessage("ready", stateChanged);
  showTextIndicator();
}

void showTextIndicatorWithDots(uint32_t elapsedMs) {
  const uint32_t dotCount = min<uint32_t>(3u, elapsedMs / kButtonPulseIntervalMs);

  String dots;
  for (uint32_t i = 0; i < dotCount; ++i) {
    dots += ".";
  }

  const String visibleMessage = interactivityState.textIndicators.message.substring(
      0, interactivityState.textIndicators.visibleCharacters);
  String displayLine = visibleMessage + dots;
  drawScreenFrame();
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.drawString(displayLine, kContentLeftX, kContentFirstLineY);
}

void showCalibrationPrompt(const String& instruction,
                           const String& progress,
                           uint8_t highlightedTap,
                           size_t visibleCharacters,
                           size_t visibleProgressCharacters) {
  const String firstLine = instruction.substring(0, min<size_t>(10, visibleCharacters));
  String secondLine;
  if (visibleCharacters > 11) {
    secondLine = instruction.substring(11, visibleCharacters);
  }

  drawScreenFrame();
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.drawString(firstLine, kContentLeftX, kContentFirstLineY);
  M5.Lcd.drawString(secondLine, kContentLeftX, kContentFirstLineY + 24);

  if (isHeaderTypingComplete() && visibleCharacters >= instruction.length()) {
    const int progressY = kContentFirstLineY + 48;
    const uint16_t grey = 0x7BEF;
    const String visibleProgress = progress.substring(0, visibleProgressCharacters);
    const int progressX = (M5.Lcd.width() - M5.Lcd.textWidth(progress.c_str())) / 2;
    const int onceIndex = progress.indexOf("once");
    const int againIndex = progress.indexOf("again");
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextColor(grey, BLACK);
    M5.Lcd.drawString(visibleProgress, progressX, progressY);
    if (highlightedTap == 1 && visibleProgressCharacters >= onceIndex + 4) {
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.drawString("once", progressX + M5.Lcd.textWidth(progress.substring(0, onceIndex).c_str()), progressY);
    }
    if (highlightedTap == 2 && visibleProgressCharacters >= againIndex + 5) {
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.drawString("again", progressX + M5.Lcd.textWidth(progress.substring(0, againIndex).c_str()), progressY);
    }
  }
}

void showResetAppPrompt() {
  const size_t visibleCharacters = interactivityState.textIndicators.visibleCharacters;
  const String firstLine = String(kResetPromptText).substring(0, min<size_t>(10, visibleCharacters));
  const size_t visibleYesCharacters = visibleCharacters > 11
                                         ? min<size_t>(3, visibleCharacters - 11)
                                         : 0;
  const size_t visibleCancelCharacters = visibleCharacters > 17
                                             ? min<size_t>(6, visibleCharacters - 17)
                                             : 0;

  drawScreenFrame();
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.drawString(firstLine, kContentLeftX, kContentFirstLineY);

  const int choicesY = kContentFirstLineY + 48;
  const uint16_t unselectedColor = 0x7BEF;
  M5.Lcd.setTextSize(2);
  const int choicesX = (M5.Lcd.width() - M5.Lcd.textWidth("yes   cancel")) / 2;
  M5.Lcd.setTextColor(resetPrompt.yesSelected ? WHITE : unselectedColor, BLACK);
  M5.Lcd.drawString(String("yes").substring(0, visibleYesCharacters), choicesX, choicesY);
  const int cancelX = choicesX + M5.Lcd.textWidth("yes   ");
  M5.Lcd.setTextColor(resetPrompt.yesSelected ? unselectedColor : WHITE, BLACK);
  M5.Lcd.drawString(String("cancel").substring(0, visibleCancelCharacters), cancelX, choicesY);
  M5.Lcd.setTextSize(2);
}

uint16_t resetTapWindowMs(uint8_t buttonId) {
  return buttonId == 1 ? interactivityState.frontDoubleTapWindowMs
                       : interactivityState.topDoubleTapWindowMs;
}

enum class ResetPromptResult { None, ConfirmReset, Cancel };

ResetPromptResult processResetAppPrompt() {
  const bool frontPressed = M5.BtnA.isPressed();
  const bool topPressed = M5.BtnB.isPressed();
  const bool frontWasPressed = M5.BtnA.wasPressed();
  const bool topWasPressed = M5.BtnB.wasPressed();
  const bool frontWasReleased = M5.BtnA.wasReleased();
  const bool topWasReleased = M5.BtnB.wasReleased();
  const bool frontWasHeld = M5.BtnA.wasHold();
  const bool topWasHeld = M5.BtnB.wasHold();
  const uint32_t nowMs = millis();
  bool redraw = false;

  if (interactivityState.textIndicators.message != kResetPromptText) {
    setTextIndicatorMessage(kResetPromptText, true);
    redraw = true;
  }

  if (resetPrompt.waitingForButtonsRelease) {
    if (!frontPressed && !topPressed) {
      resetPrompt.waitingForButtonsRelease = false;
      redraw = true;
    }
  } else {
    if (resetPrompt.waitingForSecondTap &&
        nowMs - resetPrompt.firstReleaseAtMs >= resetTapWindowMs(resetPrompt.firstTapButton)) {
      resetPrompt.yesSelected = !resetPrompt.yesSelected;
      resetPrompt.waitingForSecondTap = false;
      redraw = true;
    }

    if (frontWasHeld || topWasHeld) {
      resetPrompt.pressInProgress = false;
      resetPrompt.secondTapInProgress = false;
      if (resetPrompt.waitingForSecondTap) {
        resetPrompt.yesSelected = !resetPrompt.yesSelected;
        redraw = true;
      }
      resetPrompt.waitingForSecondTap = false;
    }

    uint8_t pressedButton = 0;
    if (frontWasPressed && !topPressed) {
      pressedButton = 1;
    } else if (topWasPressed && !frontPressed) {
      pressedButton = 2;
    }

    if (pressedButton != 0) {
      if (resetPrompt.waitingForSecondTap &&
          pressedButton == resetPrompt.firstTapButton &&
          nowMs - resetPrompt.firstReleaseAtMs < resetTapWindowMs(pressedButton)) {
        resetPrompt.secondTapInProgress = true;
        resetPrompt.pressInProgress = true;
        resetPrompt.pressedButton = pressedButton;
      } else {
        if (resetPrompt.waitingForSecondTap) {
          resetPrompt.yesSelected = !resetPrompt.yesSelected;
          redraw = true;
          resetPrompt.waitingForSecondTap = false;
        }
        resetPrompt.pressInProgress = true;
        resetPrompt.secondTapInProgress = false;
        resetPrompt.pressedButton = pressedButton;
      }
    }

    const bool matchingFrontRelease = frontWasReleased && resetPrompt.pressedButton == 1;
    const bool matchingTopRelease = topWasReleased && resetPrompt.pressedButton == 2;
    if (resetPrompt.pressInProgress && (matchingFrontRelease || matchingTopRelease)) {
      resetPrompt.pressInProgress = false;
      if (resetPrompt.secondTapInProgress) {
        return resetPrompt.yesSelected ? ResetPromptResult::ConfirmReset : ResetPromptResult::Cancel;
      }
      resetPrompt.waitingForSecondTap = true;
      resetPrompt.firstTapButton = resetPrompt.pressedButton;
      resetPrompt.firstReleaseAtMs = nowMs;
    }
  }

  if (advanceTextIndicatorTyping()) {
    redraw = true;
  }
  if (redraw) {
    showResetAppPrompt();
  }
  return ResetPromptResult::None;
}

template <typename ButtonType>
uint16_t calibrateDoubleTapWindow(ButtonType& button, ButtonType& otherButton, const char* buttonName) {
  bool firstTapDown = false;
  bool awaitingSecondTap = false;
  bool secondTapDown = false;
  uint8_t highlightedTap = 1;
  uint32_t firstReleaseAtMs = 0;
  uint32_t measuredGapMs = 0;
  size_t visibleCharacters = 0;
  uint32_t lastCharacterAtMs = millis();
  size_t visibleProgressCharacters = 0;
  uint32_t lastProgressCharacterAtMs = 0;
  bool progressTypingStarted = false;
  uint32_t bothButtonsDownAtMs = 0;
  uint32_t calibrationStartedAtMs = 0;
  bool calibrationTimerStarted = false;
  const String instruction = String("tap ") + buttonName;
  const String progress = "once then again";

  showCalibrationPrompt(instruction, progress, highlightedTap, visibleCharacters,
                        visibleProgressCharacters);

  while (true) {
    M5.update();
    bool shouldRedraw = false;
    bool calibrationComplete = false;
    const bool wasPressed = button.wasPressed();
    const bool wasReleased = button.wasReleased();
    const bool wasHeld = button.wasHold();
    (void)otherButton.wasPressed();
    (void)otherButton.wasReleased();
    (void)otherButton.wasHold();

    if (!isHeaderTypingComplete()) {
      showCalibrationPrompt(instruction, progress, highlightedTap, 0, 0);
      delay(10);
      continue;
    }

    if (!calibrationTimerStarted) {
      calibrationStartedAtMs = millis();
      calibrationTimerStarted = true;
    } else if (millis() - calibrationStartedAtMs >= kCalibrationTimeoutMs) {
      return 0;
    }

    const bool bothButtonsDown = M5.BtnA.isPressed() && M5.BtnB.isPressed();
    if (bothButtonsDown) {
      if (bothButtonsDownAtMs == 0) {
        bothButtonsDownAtMs = millis();
      } else if (millis() - bothButtonsDownAtMs >= kLongPressThresholdMs) {
        askResetApp();
        while (resetPrompt.active) {
          M5.update();
          const ResetPromptResult result = processResetAppPrompt();
          if (result == ResetPromptResult::ConfirmReset && resetPrompt.yesSelected) {
            delay(kCalibrationAfterUserInputDelayMs);
            doResetApp();
          } else if (result == ResetPromptResult::Cancel ||
                     (result == ResetPromptResult::ConfirmReset && !resetPrompt.yesSelected)) {
            resetPrompt.active = false;
            showReadyState();
          }
          delay(20);
        }
        firstTapDown = false;
        awaitingSecondTap = false;
        secondTapDown = false;
        highlightedTap = 1;
        bothButtonsDownAtMs = 0;
        calibrationStartedAtMs = millis();
        showCalibrationPrompt(instruction, progress, highlightedTap, visibleCharacters,
                  visibleProgressCharacters);
        delay(10);
        continue;
      }
    } else {
      bothButtonsDownAtMs = 0;
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

    if (isHeaderTypingComplete() && visibleCharacters < instruction.length() &&
        millis() - lastCharacterAtMs >= kCalibrationCharacterIntervalMs) {
      ++visibleCharacters;
      lastCharacterAtMs = millis();
      shouldRedraw = true;
    }
    if (visibleCharacters >= instruction.length() &&
        visibleProgressCharacters < progress.length()) {
      if (!progressTypingStarted) {
        progressTypingStarted = true;
        lastProgressCharacterAtMs = millis();
      } else if (millis() - lastProgressCharacterAtMs >= kCalibrationCharacterIntervalMs) {
        ++visibleProgressCharacters;
        lastProgressCharacterAtMs = millis();
        shouldRedraw = true;
      }
    }
    if (shouldRedraw) {
      showCalibrationPrompt(instruction, progress, highlightedTap, visibleCharacters,
                            visibleProgressCharacters);
    }
    if (calibrationComplete) {
      delay(kCalibrationAfterUserInputDelayMs);
      return static_cast<uint16_t>(measuredGapMs + kDoubleTapAllowanceMs);
    }
    delay(10);
  }
}

void playStartupTone() {
  M5.Speaker.tone(3100, 90);
  delay(110);
  M5.Speaker.stop();
  delay(90);
  M5.Speaker.tone(4200, 110);
  delay(140);
  M5.Speaker.stop();
}

void doResetApp() {
  ESP.restart();
}

void askResetApp() {
  resetPrompt = ResetPrompt{};
  resetPrompt.active = true;
  setInteractivityState(InteractivityStateKind::ResetConfirmation);
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Lcd.setRotation(1);
  M5.Speaker.setVolume(255);
  M5.BtnA.setHoldThresh(kLongPressThresholdMs);
  M5.BtnB.setHoldThresh(kLongPressThresholdMs);

  const uint16_t frontDoubleTapWindowMs = calibrateDoubleTapWindow(M5.BtnA, M5.BtnB, "front");
  const uint16_t topDoubleTapWindowMs = calibrateDoubleTapWindow(M5.BtnB, M5.BtnA, "top");
  if (frontDoubleTapWindowMs == 0 || topDoubleTapWindowMs == 0) {
    interactivityState.frontDoubleTapWindowMs = kDefaultDoubleTapWindowMs;
    interactivityState.topDoubleTapWindowMs = kDefaultDoubleTapWindowMs;
  } else {
    interactivityState.frontDoubleTapWindowMs = frontDoubleTapWindowMs;
    interactivityState.topDoubleTapWindowMs = topDoubleTapWindowMs;
  }
  interactivityState.onDoubleLong = askResetApp;
  showReadyState();
  playStartupTone();
}

void loop() {
  static uint32_t lastStatusChangeMs = 0;
  static bool statusNeedsReset = false;
  static bool statusCountdownStarted = false;
  static TapTracker frontTap;
  static TapTracker topTap;

  M5.update();

  if (resetPrompt.active) {
    const ResetPromptResult result = processResetAppPrompt();
    if (result == ResetPromptResult::ConfirmReset && resetPrompt.yesSelected) {
      delay(kCalibrationAfterUserInputDelayMs);
      doResetApp();
    } else if (result == ResetPromptResult::Cancel ||
               (result == ResetPromptResult::ConfirmReset && !resetPrompt.yesSelected)) {
      resetPrompt.active = false;
      statusNeedsReset = false;
      statusCountdownStarted = false;
      showReadyState();
    }
    delay(20);
    return;
  }

  const bool frontWasPressed = M5.BtnA.wasPressed();
  const bool frontWasHold = M5.BtnA.wasHold();
  const bool frontWasReleased = M5.BtnA.wasReleased();
  const bool topWasPressed = M5.BtnB.wasPressed();
  const bool topWasHold = M5.BtnB.wasHold();
  const bool topWasReleased = M5.BtnB.wasReleased();
  const bool frontPressed = M5.BtnA.isPressed();
  const bool topPressed = M5.BtnB.isPressed();
  const uint32_t nowMs = millis();
  bool frontSingleTapResolved = false;
  bool topSingleTapResolved = false;
  bool frontDoubleTapResolved = false;
  bool topDoubleTapResolved = false;

  if (frontTap.waitingForSecondTap && !frontTap.secondTapInProgress &&
      nowMs - frontTap.firstReleaseAtMs >= interactivityState.frontDoubleTapWindowMs) {
    frontTap.reset();
    frontSingleTapResolved = true;
  }
  if (topTap.waitingForSecondTap && !topTap.secondTapInProgress &&
      nowMs - topTap.firstReleaseAtMs >= interactivityState.topDoubleTapWindowMs) {
    topTap.reset();
    topSingleTapResolved = true;
  }

  if (frontWasPressed) {
    interactivityState.frontLongPressActive = false;
    if (frontTap.waitingForSecondTap &&
      nowMs - frontTap.firstReleaseAtMs < interactivityState.frontDoubleTapWindowMs) {
      frontTap.secondTapInProgress = true;
    } else {
      frontTap.waitingForSecondTap = false;
      frontTap.secondTapInProgress = false;
    }
    frontTap.pressInProgress = true;
    statusNeedsReset = true;
    statusCountdownStarted = false;
  }
  if (frontWasHold) {
    interactivityState.frontLongPressActive = true;
    frontTap.reset();
    statusNeedsReset = true;
    statusCountdownStarted = false;
  }

  if (topWasPressed) {
    interactivityState.topLongPressActive = false;
    if (topTap.waitingForSecondTap &&
      nowMs - topTap.firstReleaseAtMs < interactivityState.topDoubleTapWindowMs) {
      topTap.secondTapInProgress = true;
    } else {
      topTap.waitingForSecondTap = false;
      topTap.secondTapInProgress = false;
    }
    topTap.pressInProgress = true;
    statusNeedsReset = true;
    statusCountdownStarted = false;
  }
  if (topWasHold) {
    interactivityState.topLongPressActive = true;
    topTap.reset();
    statusNeedsReset = true;
    statusCountdownStarted = false;
  }

  if (frontWasPressed && !topPressed) {
    topTap.reset();
  }
  if (topWasPressed && !frontPressed) {
    frontTap.reset();
  }

  if (frontWasReleased) {
    if (interactivityState.frontLongPressActive) {
      frontTap.reset();
    } else if (frontTap.pressInProgress) {
      frontTap.pressInProgress = false;
      if (frontTap.secondTapInProgress) {
        frontTap.reset();
        frontDoubleTapResolved = true;
      } else {
        frontTap.waitingForSecondTap = true;
        frontTap.firstReleaseAtMs = nowMs;
      }
    }
  }
  if (topWasReleased) {
    if (interactivityState.topLongPressActive) {
      topTap.reset();
    } else if (topTap.pressInProgress) {
      topTap.pressInProgress = false;
      if (topTap.secondTapInProgress) {
        topTap.reset();
        topDoubleTapResolved = true;
      } else {
        topTap.waitingForSecondTap = true;
        topTap.firstReleaseAtMs = nowMs;
      }
    }
  }

  if (frontWasPressed && !topPressed && topTap.waitingForSecondTap) {
    topTap.reset();
  }
  if (topWasPressed && !frontPressed && frontTap.waitingForSecondTap) {
    frontTap.reset();
  }

  if (frontPressed && topPressed) {
    frontTap.reset();
    topTap.reset();
    frontDoubleTapResolved = false;
    topDoubleTapResolved = false;
  }

  if (!frontPressed) {
    interactivityState.frontLongPressActive = false;
    if (interactivityState.circleIndicators.mode == CircleIndicatorMode::Instant) {
      interactivityState.circleIndicators.front = IndicatorState::Ready;
    }
  }
  if (!topPressed) {
    interactivityState.topLongPressActive = false;
    if (interactivityState.circleIndicators.mode == CircleIndicatorMode::Instant) {
      interactivityState.circleIndicators.top = IndicatorState::Ready;
    }
  }

  const bool combinedStateLatched =
      (interactivityState.state == InteractivityStateKind::Both ||
       interactivityState.state == InteractivityStateKind::BothLong) &&
      !frontWasPressed && !topWasPressed;
  InteractivityStateKind nextState = interactivityState.state;
  String nextMessage = interactivityState.textIndicators.message;

  if (frontPressed && topPressed) {
    const bool bothLongPressed = interactivityState.frontLongPressActive && interactivityState.topLongPressActive;
    nextState = bothLongPressed ? InteractivityStateKind::BothLong : InteractivityStateKind::Both;
    nextMessage = bothLongPressed ? "both long" : "both";
    interactivityState.circleIndicators.front = interactivityState.frontLongPressActive
                                                     ? IndicatorState::LongPressed
                                                     : IndicatorState::Pressed;
    interactivityState.circleIndicators.top = interactivityState.topLongPressActive
                                                    ? IndicatorState::LongPressed
                                                    : IndicatorState::Pressed;
  } else if (combinedStateLatched) {
        if (interactivityState.circleIndicators.mode == CircleIndicatorMode::Instant) {
          interactivityState.circleIndicators.front = frontPressed
                      ? (interactivityState.frontLongPressActive
                        ? IndicatorState::LongPressed
                        : IndicatorState::Pressed)
                      : IndicatorState::Ready;
          interactivityState.circleIndicators.top = topPressed
                     ? (interactivityState.topLongPressActive
                       ? IndicatorState::LongPressed
                       : IndicatorState::Pressed)
                     : IndicatorState::Ready;
        }
  } else if (frontPressed) {
    nextState = interactivityState.frontLongPressActive
                    ? InteractivityStateKind::FrontLong
                    : InteractivityStateKind::FrontPending;
    nextMessage = interactivityState.frontLongPressActive ? "front long" : "front?";
    interactivityState.circleIndicators.front = interactivityState.frontLongPressActive
                                                     ? IndicatorState::LongPressed
                                                     : IndicatorState::Pressed;
    interactivityState.circleIndicators.top = IndicatorState::Ready;
  } else if (topPressed) {
    nextState = interactivityState.topLongPressActive
                    ? InteractivityStateKind::TopLong
                    : InteractivityStateKind::TopPending;
    nextMessage = interactivityState.topLongPressActive ? "top long" : "top?";
    interactivityState.circleIndicators.top = interactivityState.topLongPressActive
                                                    ? IndicatorState::LongPressed
                                                    : IndicatorState::Pressed;
    interactivityState.circleIndicators.front = IndicatorState::Ready;
  } else if (frontDoubleTapResolved) {
    nextState = InteractivityStateKind::FrontDouble;
    nextMessage = "front double";
  } else if (topDoubleTapResolved) {
    nextState = InteractivityStateKind::TopDouble;
    nextMessage = "top double";
  } else if (frontSingleTapResolved) {
    nextState = InteractivityStateKind::Front;
    nextMessage = "front";
  } else if (topSingleTapResolved) {
    nextState = InteractivityStateKind::Top;
    nextMessage = "top";
  } else if (frontTap.waitingForSecondTap || frontTap.pressInProgress) {
    nextState = InteractivityStateKind::FrontPending;
    nextMessage = "front?";
  } else if (topTap.waitingForSecondTap || topTap.pressInProgress) {
    nextState = InteractivityStateKind::TopPending;
    nextMessage = "top?";
  } else if (interactivityState.circleIndicators.mode == CircleIndicatorMode::Instant) {
    interactivityState.circleIndicators.front = IndicatorState::Ready;
    interactivityState.circleIndicators.top = IndicatorState::Ready;
  }

  const bool stateChanged = setInteractivityState(nextState);
  const bool pressEvent = frontWasPressed || topWasPressed;
  if (pressEvent && !stateChanged) {
    playStateBeep(interactivityState.state);
  }
  setTextIndicatorMessage(nextMessage, pressEvent);
  if (stateChanged || pressEvent) {
    statusCountdownStarted = false;
  }

  const bool textAdvanced = advanceTextIndicatorTyping();
  if (statusNeedsReset) {
    if (!frontPressed && !topPressed) {
      const bool tapPending = frontTap.waitingForSecondTap || frontTap.pressInProgress ||
                              topTap.waitingForSecondTap || topTap.pressInProgress;
      if (tapPending || isTextIndicatorTyping()) {
        statusCountdownStarted = false;
        showTextIndicator();
      } else if (!statusCountdownStarted) {
        lastStatusChangeMs = millis();
        statusCountdownStarted = true;
      }

      if (!isTextIndicatorTyping()) {
        const uint32_t elapsedMs = millis() - lastStatusChangeMs;
        if (elapsedMs >= kStatusResetDelayMs) {
          showReadyState();
          statusNeedsReset = false;
          statusCountdownStarted = false;
        } else {
          showTextIndicatorWithDots(elapsedMs);
        }
      }
    } else {
      showTextIndicator();
    }
  } else if (textAdvanced) {
    showTextIndicator();
  }

  delay(20);
}
