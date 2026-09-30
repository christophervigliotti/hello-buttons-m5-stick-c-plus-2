#pragma once

#include <Arduino.h>

namespace stickui {

struct LogLine {
  String text;
  String dots;
  size_t visibleCharacters = 0;
};

// The scrolling log under the header: each new line pushes the others up, older lines are
// grey, the current line is white, and at most three rows are visible. Only rows whose text
// or color changed are redrawn. Other screens are full redraws; the log is inactive while
// they show.
class LogView {
 public:
  bool isActive() const { return active_; }

  // Clears the screen and starts an empty log, unless the log is already showing.
  void enter();

  // Called when another screen takes over the display.
  void deactivate() {
    active_ = false;
    markStale();
  }

  // Forces every row (and the cursor) to redraw next render, e.g. after the screen was cleared.
  void markStale() {
    rowsValid_ = false;
    cursorDrawn_ = false;
  }

  // Redraws changed rows; non-empty `dots` become the current line's trailing dots.
  void render(const String& dots = "");

  // Enters the log if needed, then pushes a line with `visibleCharacters` already typed.
  void append(const String& text, size_t visibleCharacters);

  size_t lineCount() const { return lineCount_; }
  LogLine& currentLine() { return lines_[lineCount_ - 1]; }

  // Updates how much of the current line has typed, then renders.
  void setCurrentVisible(size_t visibleCharacters);

  // Slow blinking box after the current line while `eligible`, starting (with a click)
  // kCursorDelayMs after it becomes eligible.
  void updateCursor(bool eligible);

 private:
  static const size_t kRows = 3;
  static const size_t kMaxWrappedLines = 9;

  bool active_ = false;
  LogLine lines_[kRows];
  size_t lineCount_ = 0;

  String rows_[kRows];
  uint16_t rowColors_[kRows] = {0, 0, 0};
  bool rowsValid_ = false;
  size_t rowCount_ = 0;

  bool cursorDrawn_ = false;
  int cursorX_ = 0;
  int cursorY_ = 0;
  uint32_t cursorEligibleSinceMs_ = 0;
  bool cursorAppeared_ = false;
};

extern LogView logView;

}  // namespace stickui
