#pragma once

#include <Arduino.h>
#include <StickUI.h>

// State for the helloButtons demo: which gesture is being shown on the log's current line.
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
  BothLong
};

struct InteractivityState {
  InteractivityStateKind state = InteractivityStateKind::Ready;
  void (*onDoubleLong)() = nullptr;
  stickui::Typewriter currentLineTyping{"ready"};
  bool frontLongPressActive = false;
  bool topLongPressActive = false;
};

extern InteractivityState interactivityState;
