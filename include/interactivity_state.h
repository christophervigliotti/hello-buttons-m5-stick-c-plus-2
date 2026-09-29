#pragma once

#include <Arduino.h>

enum class IndicatorState { Ready, Pressed, LongPressed, DoublePressed };
enum class CircleIndicatorMode { Sticky, Instant };
enum class InteractivityStateKind {
  Ready,
  FrontPending,
  Front,
  FrontDouble,
  FrontLong,
  TopPending,
  Top,
  TopDouble,
  TopLong,
  Both,
  BothLong,
  ResetConfirmation
};

struct StateBeepConfig {
  bool enabled;
  uint16_t frequencyHz;
  uint16_t durationMs;
  uint8_t pulseCount;
  uint16_t gapMs;

  StateBeepConfig(bool isEnabled = false,
                  uint16_t frequency = 2600,
                  uint16_t duration = 120,
                  uint8_t count = 1,
                  uint16_t gap = 30)
      : enabled(isEnabled),
        frequencyHz(frequency),
        durationMs(duration),
        pulseCount(count),
        gapMs(gap) {}
};

// Typing click, and the button click two semitones above it.
const uint16_t kTypingClickHz = 4000;
const uint16_t kButtonClickHz = 4490;
const uint16_t kClickDurationMs = 4;

struct StateBeepSettings {
  StateBeepConfig ready;
  StateBeepConfig frontPending;
  StateBeepConfig front;
  StateBeepConfig frontDouble;
  StateBeepConfig frontLong;
  StateBeepConfig topPending;
  StateBeepConfig top;
  StateBeepConfig topDouble;
  StateBeepConfig topLong;
  StateBeepConfig both;
  StateBeepConfig bothLong;
  StateBeepConfig resetConfirmation;

  StateBeepSettings()
      : ready(),
        frontPending(),
        front(),
        frontDouble(true, kButtonClickHz, kClickDurationMs, 2, 30),
        frontLong(true, kButtonClickHz, kClickDurationMs, 1),
        topPending(),
        top(),
        topDouble(true, kButtonClickHz, kClickDurationMs, 2, 30),
        topLong(true, kButtonClickHz, kClickDurationMs, 1),
        both(true, kButtonClickHz, kClickDurationMs, 3, 30),
        bothLong(true, kButtonClickHz, kClickDurationMs, 3, 30),
        resetConfirmation() {}
};

struct CircleIndicators {
  CircleIndicatorMode mode = CircleIndicatorMode::Sticky;
  IndicatorState front = IndicatorState::Ready;
  IndicatorState top = IndicatorState::Ready;
};

struct TextIndicators {
  String message;
  size_t visibleCharacters;
  uint32_t lastCharacterAtMs;
  uint16_t characterIntervalMs;

  TextIndicators()
      : message("ready"),
        visibleCharacters(message.length()),
        lastCharacterAtMs(0),
      characterIntervalMs(25) {}
};

struct InteractivityState {
  InteractivityStateKind state = InteractivityStateKind::Ready;
  void (*onDoubleLong)() = nullptr;
  StateBeepSettings beeps;
  CircleIndicators circleIndicators;
  TextIndicators textIndicators;
  bool frontLongPressActive = false;
  bool topLongPressActive = false;
  uint16_t frontDoubleTapWindowMs = 300;
  uint16_t topDoubleTapWindowMs = 300;
  bool soundEnabled = true;
};

extern InteractivityState interactivityState;