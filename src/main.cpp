// helloButtons: shows every button gesture as a line in the StickUI log.
#include <Arduino.h>
#include <StickUI.h>

#include "interactivity_state.h"

using namespace stickui;

InteractivityState interactivityState;

namespace {

const uint32_t kStatusResetDelayMs = 1000;

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

Typewriter& currentLineTyping() {
  return interactivityState.currentLineTyping;
}

void setCurrentLineMessage(const String& message, bool restartTyping = false) {
  if (currentLineTyping().text() == message && !restartTyping) {
    return;
  }
  currentLineTyping().start(message, 1);
}

// Turns a pending "top?" / "front?" line into its resolved text in place, keeping the
// characters already typed. Returns false when the current line isn't that pending line.
bool resolvePendingStatusLine(const String& resolvedText) {
  if (!logView.isActive() || logView.lineCount() == 0) {
    return false;
  }

  LogLine& currentLine = logView.currentLine();
  String mergedText;
  if (currentLine.text == "top?" &&
      (resolvedText == "top" || resolvedText == "top long" || resolvedText == "top double")) {
    mergedText = resolvedText;
  } else if (currentLine.text == "front?" &&
             (resolvedText == "front" || resolvedText == "front long" || resolvedText == "front double")) {
    mergedText = resolvedText;
  } else {
    return false;
  }

  const size_t preservedCharacters = min(currentLine.visibleCharacters,
                                          resolvedText.startsWith("front") ? 5u : 3u);
  currentLine.text = mergedText;
  currentLine.visibleCharacters = preservedCharacters;
  currentLineTyping().start(mergedText, preservedCharacters);
  logView.render();
  return true;
}

const ClickPattern* soundForState(InteractivityStateKind state) {
  switch (state) {
    case InteractivityStateKind::FrontDouble:
    case InteractivityStateKind::TopDouble:
      return &buttonSounds.doubleTap;
    case InteractivityStateKind::FrontLong:
    case InteractivityStateKind::TopLong:
      return &buttonSounds.longPress;
    case InteractivityStateKind::Both:
      return &buttonSounds.both;
    case InteractivityStateKind::BothLong:
      return &buttonSounds.bothLong;
    default:
      return nullptr;  // ready, pending and resolved single taps are silent
  }
}

void playStateSound(InteractivityStateKind state) {
  const ClickPattern* pattern = soundForState(state);
  if (pattern != nullptr) {
    playPattern(*pattern);
  }
}

bool setInteractivityState(InteractivityStateKind state) {
  if (interactivityState.state == state) {
    return false;
  }
  interactivityState.state = state;
  playStateSound(state);
  if (state == InteractivityStateKind::BothLong && interactivityState.onDoubleLong != nullptr) {
    interactivityState.onDoubleLong();
  }
  return true;
}

void showReadyState() {
  setInteractivityState(InteractivityStateKind::Ready);
  indicators.reset();
  interactivityState.frontLongPressActive = false;
  interactivityState.topLongPressActive = false;
  setCurrentLineMessage("ready", true);
  logView.append(currentLineTyping().text(), currentLineTyping().visible());
}

// The blinking cursor follows a fully typed "ready" line with no dots.
bool readyCursorEligible() {
  return interactivityState.state == InteractivityStateKind::Ready &&
         logView.lineCount() > 0 && logView.currentLine().text == "ready" &&
         logView.currentLine().dots.isEmpty() && !currentLineTyping().isTyping();
}

}  // namespace

void setup() {
  stickui::begin("helloButtons");
  runSetup();
  interactivityState.onDoubleLong = askResetApp;
  showReadyState();
}

void loop() {
  static uint32_t lastStatusChangeMs = 0;
  static bool statusNeedsReset = false;
  static bool statusCountdownStarted = false;
  static TapTracker frontTap;
  static TapTracker topTap;

  M5.update();

  if (isResetPromptActive()) {
    if (updateResetPrompt() == ResetPromptOutcome::Cancelled) {
      statusNeedsReset = false;
      statusCountdownStarted = false;
      showReadyState();
    }
    delay(20);
    return;
  }

  if (handleHeldOrientation()) {
    logView.render();
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
      nowMs - frontTap.firstReleaseAtMs >= settings.frontDoubleTapWindowMs) {
    frontTap.reset();
    frontSingleTapResolved = true;
  }
  if (topTap.waitingForSecondTap && !topTap.secondTapInProgress &&
      nowMs - topTap.firstReleaseAtMs >= settings.topDoubleTapWindowMs) {
    topTap.reset();
    topSingleTapResolved = true;
  }

  if (frontWasPressed) {
    interactivityState.frontLongPressActive = false;
    if (frontTap.waitingForSecondTap &&
        nowMs - frontTap.firstReleaseAtMs < settings.frontDoubleTapWindowMs) {
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
        nowMs - topTap.firstReleaseAtMs < settings.topDoubleTapWindowMs) {
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
  }
  if (!topPressed) {
    interactivityState.topLongPressActive = false;
  }

  const bool combinedStateLatched =
      (interactivityState.state == InteractivityStateKind::Both ||
       interactivityState.state == InteractivityStateKind::BothLong) &&
      !frontWasPressed && !topWasPressed;
  InteractivityStateKind nextState = interactivityState.state;
  String nextMessage = currentLineTyping().text();

  if (frontPressed && topPressed) {
    const bool bothLongPressed = interactivityState.frontLongPressActive && interactivityState.topLongPressActive;
    nextState = bothLongPressed ? InteractivityStateKind::BothLong : InteractivityStateKind::Both;
    nextMessage = bothLongPressed ? "both long" : "both";
    indicators.front = interactivityState.frontLongPressActive ? IndicatorState::LongPressed
                                                               : IndicatorState::Pressed;
    indicators.top = interactivityState.topLongPressActive ? IndicatorState::LongPressed
                                                           : IndicatorState::Pressed;
  } else if (combinedStateLatched) {
    // Keep showing "both" / "both long" until a new press.
  } else if (frontPressed) {
    nextState = interactivityState.frontLongPressActive
                    ? InteractivityStateKind::FrontLong
                    : InteractivityStateKind::FrontPending;
    nextMessage = interactivityState.frontLongPressActive ? "front long" : "front?";
    indicators.front = interactivityState.frontLongPressActive ? IndicatorState::LongPressed
                                                               : IndicatorState::Pressed;
    indicators.top = IndicatorState::Ready;
  } else if (topPressed) {
    nextState = interactivityState.topLongPressActive
                    ? InteractivityStateKind::TopLong
                    : InteractivityStateKind::TopPending;
    nextMessage = interactivityState.topLongPressActive ? "top long" : "top?";
    indicators.top = interactivityState.topLongPressActive ? IndicatorState::LongPressed
                                                           : IndicatorState::Pressed;
    indicators.front = IndicatorState::Ready;
  } else if (frontDoubleTapResolved) {
    nextState = InteractivityStateKind::FrontDouble;
    nextMessage = "front double";
    indicators.front = IndicatorState::DoublePressed;
    indicators.top = IndicatorState::Ready;
  } else if (topDoubleTapResolved) {
    nextState = InteractivityStateKind::TopDouble;
    nextMessage = "top double";
    indicators.top = IndicatorState::DoublePressed;
    indicators.front = IndicatorState::Ready;
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
  }

  const bool stateChanged = setInteractivityState(nextState);
  if (isResetPromptActive()) {
    delay(20);
    return;
  }
  const bool pressEvent = frontWasPressed || topWasPressed;
  const bool secondTapStarted = (frontWasPressed && frontTap.secondTapInProgress) ||
                                (topWasPressed && topTap.secondTapInProgress);
  if (pressEvent && !stateChanged) {
    playStateSound(interactivityState.state);
  }
  setCurrentLineMessage(nextMessage, pressEvent && !secondTapStarted);
  const bool statusLineChanged = stateChanged || (pressEvent && !secondTapStarted);
  const bool pendingLineResolved = stateChanged && resolvePendingStatusLine(nextMessage);
  if (statusLineChanged && !pendingLineResolved) {
    logView.append(currentLineTyping().text(), currentLineTyping().visible());
  }
  if (stateChanged || pressEvent) {
    statusCountdownStarted = false;
  }

  const bool textAdvanced = currentLineTyping().advance();
  if (textAdvanced && !statusLineChanged) {
    logView.setCurrentVisible(currentLineTyping().visible());
  }

  static uint32_t lastDisplayedDotCount = UINT32_MAX;
  if (statusNeedsReset) {
    if (!frontPressed && !topPressed) {
      const bool tapPending = frontTap.waitingForSecondTap || frontTap.pressInProgress ||
                              topTap.waitingForSecondTap || topTap.pressInProgress;
      if (tapPending || currentLineTyping().isTyping()) {
        statusCountdownStarted = false;
      } else {
        if (!statusCountdownStarted) {
          lastStatusChangeMs = millis();
          statusCountdownStarted = true;
          lastDisplayedDotCount = UINT32_MAX;
        }

        const uint32_t elapsedMs = millis() - lastStatusChangeMs;
        if (elapsedMs >= kStatusResetDelayMs) {
          setInteractivityState(InteractivityStateKind::Ready);
          indicators.reset();
          interactivityState.frontLongPressActive = false;
          interactivityState.topLongPressActive = false;
          setCurrentLineMessage("ready", true);
          logView.append("ready", currentLineTyping().visible());
          statusNeedsReset = false;
          statusCountdownStarted = false;
          lastDisplayedDotCount = UINT32_MAX;
        } else {
          const uint32_t dotCount = min<uint32_t>(3u, elapsedMs / kDotIntervalMs);
          if (dotCount != lastDisplayedDotCount) {
            String dots;
            for (uint32_t index = 0; index < dotCount; ++index) {
              dots += ".";
            }
            logView.render(dots);
            if (dotCount > 0) {
              playTypingClick('.');
            }
            lastDisplayedDotCount = dotCount;
          }
        }
      }
    }
  }

  logView.updateCursor(readyCursorEligible());
  delay(20);
}
