#include "ButtonDemoView.h"

using namespace stickui;

namespace {

const uint32_t kStatusResetDelayMs = 1000;

}  // namespace

void ButtonDemoView::onEnter() {
  showReadyState();
}

void ButtonDemoView::onResume(ResumeReason reason) {
  if (reason == ResumeReason::OrientationRestored) {
    logView.render();
    return;
  }
  statusNeedsReset_ = false;
  statusCountdownStarted_ = false;
  showReadyState();
}

void ButtonDemoView::setCurrentLineMessage(const String& message, bool restartTyping) {
  if (currentLineTyping_.text() == message && !restartTyping) {
    return;
  }
  currentLineTyping_.start(message, 1);
}

// Turns a pending "side?" / "face?" line into its resolved text in place, keeping the
// characters already typed. Returns false when the current line isn't that pending line.
bool ButtonDemoView::resolvePendingStatusLine(const String& resolvedText) {
  if (!logView.isActive() || logView.lineCount() == 0) {
    return false;
  }

  LogLine& currentLine = logView.currentLine();
  String mergedText;
  if (currentLine.text == "side?" &&
      (resolvedText == "side" || resolvedText == "side long" || resolvedText == "side double")) {
    mergedText = resolvedText;
  } else if (currentLine.text == "face?" &&
             (resolvedText == "face" || resolvedText == "face long" || resolvedText == "face double")) {
    mergedText = resolvedText;
  } else {
    return false;
  }

  const size_t preservedCharacters = min(currentLine.visibleCharacters, 4u);  // "face" / "side"
  currentLine.text = mergedText;
  currentLine.visibleCharacters = preservedCharacters;
  currentLineTyping_.start(mergedText, preservedCharacters);
  logView.render();
  return true;
}

const ClickPattern* ButtonDemoView::soundFor(Gesture gesture) const {
  switch (gesture) {
    case Gesture::FaceDouble:
    case Gesture::SideDouble:
      return &buttonSounds.doubleTap;
    case Gesture::FaceLong:
    case Gesture::SideLong:
      return &buttonSounds.longPress;
    case Gesture::Both:
      return &buttonSounds.both;
    default:
      return nullptr;  // ready, pending and resolved single taps are silent
  }
}

void ButtonDemoView::playGestureSound(Gesture gesture) {
  const ClickPattern* pattern = soundFor(gesture);
  if (pattern != nullptr) {
    playPattern(*pattern);
  }
}

bool ButtonDemoView::setGesture(Gesture gesture) {
  if (gesture_ == gesture) {
    return false;
  }
  gesture_ = gesture;
  playGestureSound(gesture);
  return true;
}

void ButtonDemoView::showReadyState() {
  setGesture(Gesture::Ready);
  indicators.reset();
  faceLongPressActive_ = false;
  sideLongPressActive_ = false;
  setCurrentLineMessage("ready", true);
  logView.append(currentLineTyping_.text(), currentLineTyping_.visible());
}

// The blinking cursor follows a fully typed "ready" line with no dots.
bool ButtonDemoView::readyCursorEligible() const {
  return gesture_ == Gesture::Ready && logView.lineCount() > 0 &&
         logView.currentLine().text == "ready" && logView.currentLine().dots.isEmpty() &&
         !currentLineTyping_.isTyping();
}

void ButtonDemoView::update(const ButtonInput& input) {
  const ButtonState& face = input.face;
  const ButtonState& side = input.side;
  const uint32_t nowMs = millis();
  bool faceSingleTapResolved = false;
  bool sideSingleTapResolved = false;
  bool faceDoubleTapResolved = false;
  bool sideDoubleTapResolved = false;

  if (faceTap_.waitingForSecondTap && !faceTap_.secondTapInProgress &&
      nowMs - faceTap_.firstReleaseAtMs >= settings.faceDoubleTapWindowMs) {
    faceTap_.reset();
    faceSingleTapResolved = true;
  }
  if (sideTap_.waitingForSecondTap && !sideTap_.secondTapInProgress &&
      nowMs - sideTap_.firstReleaseAtMs >= settings.sideDoubleTapWindowMs) {
    sideTap_.reset();
    sideSingleTapResolved = true;
  }

  if (face.wasPressed) {
    faceLongPressActive_ = false;
    if (faceTap_.waitingForSecondTap &&
        nowMs - faceTap_.firstReleaseAtMs < settings.faceDoubleTapWindowMs) {
      faceTap_.secondTapInProgress = true;
    } else {
      faceTap_.waitingForSecondTap = false;
      faceTap_.secondTapInProgress = false;
    }
    faceTap_.pressInProgress = true;
    statusNeedsReset_ = true;
    statusCountdownStarted_ = false;
  }
  if (face.wasHeld) {
    faceLongPressActive_ = true;
    faceTap_.reset();
    statusNeedsReset_ = true;
    statusCountdownStarted_ = false;
  }

  if (side.wasPressed) {
    sideLongPressActive_ = false;
    if (sideTap_.waitingForSecondTap &&
        nowMs - sideTap_.firstReleaseAtMs < settings.sideDoubleTapWindowMs) {
      sideTap_.secondTapInProgress = true;
    } else {
      sideTap_.waitingForSecondTap = false;
      sideTap_.secondTapInProgress = false;
    }
    sideTap_.pressInProgress = true;
    statusNeedsReset_ = true;
    statusCountdownStarted_ = false;
  }
  if (side.wasHeld) {
    sideLongPressActive_ = true;
    sideTap_.reset();
    statusNeedsReset_ = true;
    statusCountdownStarted_ = false;
  }

  if (face.wasPressed && !side.isPressed) {
    sideTap_.reset();
  }
  if (side.wasPressed && !face.isPressed) {
    faceTap_.reset();
  }

  if (face.wasReleased) {
    if (faceLongPressActive_) {
      faceTap_.reset();
    } else if (faceTap_.pressInProgress) {
      faceTap_.pressInProgress = false;
      if (faceTap_.secondTapInProgress) {
        faceTap_.reset();
        faceDoubleTapResolved = true;
      } else {
        faceTap_.waitingForSecondTap = true;
        faceTap_.firstReleaseAtMs = nowMs;
      }
    }
  }
  if (side.wasReleased) {
    if (sideLongPressActive_) {
      sideTap_.reset();
    } else if (sideTap_.pressInProgress) {
      sideTap_.pressInProgress = false;
      if (sideTap_.secondTapInProgress) {
        sideTap_.reset();
        sideDoubleTapResolved = true;
      } else {
        sideTap_.waitingForSecondTap = true;
        sideTap_.firstReleaseAtMs = nowMs;
      }
    }
  }

  if (face.wasPressed && !side.isPressed && sideTap_.waitingForSecondTap) {
    sideTap_.reset();
  }
  if (side.wasPressed && !face.isPressed && faceTap_.waitingForSecondTap) {
    faceTap_.reset();
  }

  if (face.isPressed && side.isPressed) {
    faceTap_.reset();
    sideTap_.reset();
    faceDoubleTapResolved = false;
    sideDoubleTapResolved = false;
  }

  if (!face.isPressed) {
    faceLongPressActive_ = false;
  }
  if (!side.isPressed) {
    sideLongPressActive_ = false;
  }

  const bool combinedStateLatched = gesture_ == Gesture::Both && !face.wasPressed && !side.wasPressed;
  Gesture nextGesture = gesture_;
  String nextMessage = currentLineTyping_.text();

  if (face.isPressed && side.isPressed) {
    nextGesture = Gesture::Both;
    nextMessage = "both";
    indicators.face = faceLongPressActive_ ? IndicatorState::LongPressed : IndicatorState::Pressed;
    indicators.side = sideLongPressActive_ ? IndicatorState::LongPressed : IndicatorState::Pressed;
  } else if (combinedStateLatched) {
    // Keep showing "both" until a new press.
  } else if (face.isPressed) {
    nextGesture = faceLongPressActive_ ? Gesture::FaceLong : Gesture::FacePending;
    nextMessage = faceLongPressActive_ ? "face long" : "face?";
    indicators.face = faceLongPressActive_ ? IndicatorState::LongPressed : IndicatorState::Pressed;
    indicators.side = IndicatorState::Ready;
  } else if (side.isPressed) {
    nextGesture = sideLongPressActive_ ? Gesture::SideLong : Gesture::SidePending;
    nextMessage = sideLongPressActive_ ? "side long" : "side?";
    indicators.side = sideLongPressActive_ ? IndicatorState::LongPressed : IndicatorState::Pressed;
    indicators.face = IndicatorState::Ready;
  } else if (faceDoubleTapResolved) {
    nextGesture = Gesture::FaceDouble;
    nextMessage = "face double";
    indicators.face = IndicatorState::DoublePressed;
    indicators.side = IndicatorState::Ready;
  } else if (sideDoubleTapResolved) {
    nextGesture = Gesture::SideDouble;
    nextMessage = "side double";
    indicators.side = IndicatorState::DoublePressed;
    indicators.face = IndicatorState::Ready;
  } else if (faceSingleTapResolved) {
    nextGesture = Gesture::Face;
    nextMessage = "face";
  } else if (sideSingleTapResolved) {
    nextGesture = Gesture::Side;
    nextMessage = "side";
  } else if (faceTap_.waitingForSecondTap || faceTap_.pressInProgress) {
    nextGesture = Gesture::FacePending;
    nextMessage = "face?";
  } else if (sideTap_.waitingForSecondTap || sideTap_.pressInProgress) {
    nextGesture = Gesture::SidePending;
    nextMessage = "side?";
  }

  const bool gestureChanged = setGesture(nextGesture);
  const bool pressEvent = face.wasPressed || side.wasPressed;
  const bool secondTapStarted = (face.wasPressed && faceTap_.secondTapInProgress) ||
                                (side.wasPressed && sideTap_.secondTapInProgress);
  if (pressEvent && !gestureChanged) {
    playGestureSound(gesture_);
  }
  setCurrentLineMessage(nextMessage, pressEvent && !secondTapStarted);
  const bool statusLineChanged = gestureChanged || (pressEvent && !secondTapStarted);
  const bool pendingLineResolved = gestureChanged && resolvePendingStatusLine(nextMessage);
  if (statusLineChanged && !pendingLineResolved) {
    logView.append(currentLineTyping_.text(), currentLineTyping_.visible());
  }
  if (gestureChanged || pressEvent) {
    statusCountdownStarted_ = false;
  }

  const bool textAdvanced = currentLineTyping_.advance();
  if (textAdvanced && !statusLineChanged) {
    logView.setCurrentVisible(currentLineTyping_.visible());
  }

  if (statusNeedsReset_ && !face.isPressed && !side.isPressed) {
    const bool tapPending = faceTap_.waitingForSecondTap || faceTap_.pressInProgress ||
                            sideTap_.waitingForSecondTap || sideTap_.pressInProgress;
    if (tapPending || currentLineTyping_.isTyping()) {
      statusCountdownStarted_ = false;
    } else {
      if (!statusCountdownStarted_) {
        lastStatusChangeMs_ = millis();
        statusCountdownStarted_ = true;
        lastDisplayedDotCount_ = UINT32_MAX;
      }

      const uint32_t elapsedMs = millis() - lastStatusChangeMs_;
      if (elapsedMs >= kStatusResetDelayMs) {
        setGesture(Gesture::Ready);
        indicators.reset();
        faceLongPressActive_ = false;
        sideLongPressActive_ = false;
        setCurrentLineMessage("ready", true);
        logView.append("ready", currentLineTyping_.visible());
        statusNeedsReset_ = false;
        statusCountdownStarted_ = false;
        lastDisplayedDotCount_ = UINT32_MAX;
      } else {
        const uint32_t dotCount = min<uint32_t>(3u, elapsedMs / kDotIntervalMs);
        if (dotCount != lastDisplayedDotCount_) {
          String dots;
          for (uint32_t index = 0; index < dotCount; ++index) {
            dots += ".";
          }
          logView.render(dots);
          if (dotCount > 0) {
            playTypingClick('.');
          }
          lastDisplayedDotCount_ = dotCount;
        }
      }
    }
  }

  logView.updateCursor(readyCursorEligible());
}
