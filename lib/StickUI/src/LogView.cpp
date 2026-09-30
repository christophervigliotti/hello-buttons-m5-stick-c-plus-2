#include "LogView.h"

#include <M5Unified.h>

#include "Header.h"
#include "Sound.h"
#include "Text.h"
#include "Theme.h"

namespace stickui {

LogView logView;

void LogView::enter() {
  if (active_) {
    return;
  }
  drawScreenFrame();
  active_ = true;
  lineCount_ = 0;
  rowsValid_ = false;
}

void LogView::render(const String& dots) {
  if (!active_) {
    return;
  }

  drawScreenFrame(false);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextSize(kTextSize);
  if (lineCount_ > 0 && !dots.isEmpty()) {
    lines_[lineCount_ - 1].dots = dots;
  }

  String wrappedLines[kMaxWrappedLines];
  bool wrappedLineIsCurrent[kMaxWrappedLines];
  size_t wrappedLineCount = 0;
  const int availableWidth = M5.Lcd.width() - kContentLeftX * 2;
  for (size_t index = 0; index < lineCount_; ++index) {
    const bool isCurrentLine = index == lineCount_ - 1;
    String remaining = lines_[index].text.substring(0, lines_[index].visibleCharacters) +
                       lines_[index].dots;
    while (!remaining.isEmpty() && wrappedLineCount < kMaxWrappedLines) {
      size_t fitCharacters = remaining.length();
      while (fitCharacters > 1 &&
             M5.Lcd.textWidth(remaining.substring(0, fitCharacters).c_str()) > availableWidth) {
        --fitCharacters;
      }

      size_t breakAt = fitCharacters;
      if (fitCharacters < remaining.length()) {
        const int lastSpace = remaining.substring(0, fitCharacters + 1).lastIndexOf(' ');
        if (lastSpace > 0) {
          breakAt = static_cast<size_t>(lastSpace);
        }
      }

      wrappedLines[wrappedLineCount] = remaining.substring(0, breakAt);
      wrappedLineIsCurrent[wrappedLineCount] = isCurrentLine;
      ++wrappedLineCount;
      remaining = remaining.substring(breakAt);
      while (!remaining.isEmpty() && remaining[0] == ' ') {
        remaining.remove(0, 1);
      }
    }
  }

  const size_t firstVisibleLine = wrappedLineCount > kRows ? wrappedLineCount - kRows : 0;
  String nextRows[kRows];
  uint16_t nextColors[kRows] = {BLACK, BLACK, BLACK};
  const size_t visibleRowCount = wrappedLineCount - firstVisibleLine;
  for (size_t row = 0; row < visibleRowCount; ++row) {
    const size_t lineIndex = firstVisibleLine + row;
    nextRows[row] = wrappedLines[lineIndex];
    nextColors[row] = wrappedLineIsCurrent[lineIndex] ? WHITE : kGreyText;
  }

  for (size_t row = 0; row < kRows; ++row) {
    if (!rowsValid_ || nextRows[row] != rows_[row] || nextColors[row] != rowColors_[row]) {
      const int rowY = contentLineY(row);
      M5.Lcd.fillRect(0, rowY, M5.Lcd.width(), kRowHeight, BLACK);
      if (!nextRows[row].isEmpty()) {
        M5.Lcd.setTextColor(nextColors[row], BLACK);
        M5.Lcd.drawString(nextRows[row], kContentLeftX, rowY);
      }
      rows_[row] = nextRows[row];
      rowColors_[row] = nextColors[row];
      cursorDrawn_ = false;
    }
  }
  rowsValid_ = true;
  rowCount_ = visibleRowCount;
}

void LogView::append(const String& text, size_t visibleCharacters) {
  enter();
  if (lineCount_ == kRows) {
    for (size_t index = 1; index < kRows; ++index) {
      lines_[index - 1] = lines_[index];
    }
    --lineCount_;
  }
  lines_[lineCount_].text = text;
  lines_[lineCount_].dots = "";
  lines_[lineCount_].visibleCharacters = visibleCharacters;
  ++lineCount_;
  playTypingClick();
  render();
}

void LogView::setCurrentVisible(size_t visibleCharacters) {
  if (!active_ || lineCount_ == 0) {
    return;
  }
  lines_[lineCount_ - 1].visibleCharacters = visibleCharacters;
  render();
}

void LogView::updateCursor(bool eligible) {
  const bool showCursor = active_ && lineCount_ > 0 && rowCount_ > 0 && eligible;
  if (!showCursor) {
    cursorEligibleSinceMs_ = 0;
    cursorAppeared_ = false;
  } else if (cursorEligibleSinceMs_ == 0) {
    cursorEligibleSinceMs_ = millis();
  }
  const uint32_t eligibleElapsedMs = showCursor ? millis() - cursorEligibleSinceMs_ : 0;
  const bool cursorOn = showCursor && eligibleElapsedMs >= kCursorDelayMs &&
                        ((eligibleElapsedMs - kCursorDelayMs) / kCursorBlinkMs) % 2 == 0;
  if (cursorOn == cursorDrawn_) {
    return;
  }
  if (cursorOn && !cursorAppeared_) {
    cursorAppeared_ = true;
    playTypingClick();
  }

  const int cursorWidth = M5.Lcd.textWidth("0") - 2;
  const int cursorHeight = M5.Lcd.fontHeight();
  if (cursorOn) {
    const size_t row = rowCount_ - 1;
    cursorX_ = kContentLeftX + M5.Lcd.textWidth(rows_[row].c_str()) + 2;
    cursorY_ = contentLineY(row);
  }
  M5.Lcd.fillRect(cursorX_, cursorY_, cursorWidth, cursorHeight, cursorOn ? WHITE : BLACK);
  cursorDrawn_ = cursorOn;
}

}  // namespace stickui
