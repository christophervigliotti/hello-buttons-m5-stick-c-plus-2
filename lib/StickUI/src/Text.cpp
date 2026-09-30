#include "Text.h"

#include <M5Unified.h>

#include "LogView.h"
#include "Sound.h"
#include "Theme.h"

namespace stickui {

namespace {

int fullScreenTextY() {
  return (M5.Lcd.height() - M5.Lcd.fontHeight()) / 2;
}

}  // namespace

int contentLineY(size_t row) {
  return kContentFirstLineY + row * kRowHeight;
}

int centeredX(const String& text) {
  return (M5.Lcd.width() - M5.Lcd.textWidth(text.c_str())) / 2;
}

size_t takeVisible(size_t& remaining, size_t length) {
  const size_t visible = min(remaining, length);
  remaining -= visible;
  return visible;
}

void drawCenteredLine(const String& text, size_t visibleCharacters, int y, uint16_t color) {
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(color, BLACK);
  M5.Lcd.drawString(text.substring(0, visibleCharacters), centeredX(text), y);
}

void drawChoiceLine(const String& first, const String& second, bool firstSelected,
                    size_t visibleCharacters, int y) {
  const String leading = first + kChoiceSpacer;
  const int x = centeredX(leading + second);
  size_t remaining = visibleCharacters;
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(firstSelected ? WHITE : kGreyText, BLACK);
  M5.Lcd.drawString(first.substring(0, takeVisible(remaining, first.length())), x, y);
  takeVisible(remaining, leading.length() - first.length());
  M5.Lcd.setTextColor(firstSelected ? kGreyText : WHITE, BLACK);
  M5.Lcd.drawString(second.substring(0, remaining), x + M5.Lcd.textWidth(leading.c_str()), y);
}

size_t wrapToScreenWidth(const String& text, String lines[], size_t maxLines) {
  const int availableWidth = M5.Lcd.width() - kContentLeftX * 2;
  size_t lineCount = 0;
  String remaining = text;
  while (!remaining.isEmpty() && lineCount < maxLines) {
    size_t breakAt = remaining.length();
    while (M5.Lcd.textWidth(remaining.substring(0, breakAt).c_str()) > availableWidth) {
      const int lastSpace = remaining.substring(0, breakAt).lastIndexOf(' ');
      if (lastSpace <= 0) {
        break;
      }
      breakAt = static_cast<size_t>(lastSpace);
    }
    lines[lineCount++] = remaining.substring(0, breakAt);
    remaining = remaining.substring(breakAt);
    remaining.trim();
  }
  return lineCount;
}

int typeFullScreenText(const String& text, const String& reservedSuffix) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(kTextSize);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(WHITE, BLACK);
  const int x = centeredX(text + reservedSuffix);
  for (size_t visible = 1; visible <= text.length(); ++visible) {
    M5.Lcd.drawString(text.substring(0, visible), x, fullScreenTextY());
    playTypingClick(text[visible - 1]);
    delay(kTypingIntervalMs);
  }
  return x + M5.Lcd.textWidth(text.c_str());
}

void typeFullScreenLines(const String lines[], size_t lineCount, size_t alignedLineCount) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(kTextSize);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(WHITE, BLACK);
  const int lineHeight = M5.Lcd.fontHeight() + 4;
  int blockWidth = 0;
  for (size_t line = 0; line < alignedLineCount; ++line) {
    blockWidth = max(blockWidth, static_cast<int>(M5.Lcd.textWidth(lines[line].c_str())));
  }
  const int blockX = (M5.Lcd.width() - blockWidth) / 2;
  const int firstY = (M5.Lcd.height() - lineHeight * static_cast<int>(lineCount) + 4) / 2;
  for (size_t line = 0; line < lineCount; ++line) {
    const int x = line < alignedLineCount ? blockX : centeredX(lines[line]);
    for (size_t visible = 1; visible <= lines[line].length(); ++visible) {
      M5.Lcd.drawString(lines[line].substring(0, visible), x, firstY + line * lineHeight);
      playTypingClick(lines[line][visible - 1]);
      delay(kTypingIntervalMs);
    }
  }
}

void showFullScreenMessage(const String& text, uint32_t durationMs) {
  logView.deactivate();
  const String dots = "...";
  const int dotsX = typeFullScreenText(text, dots);
  const int y = fullScreenTextY();

  const uint32_t startedAtMs = millis();
  size_t lastDotCount = SIZE_MAX;
  while (millis() - startedAtMs < durationMs) {
    const size_t dotCount = ((millis() - startedAtMs) / kDotIntervalMs) % 4;
    if (dotCount != lastDotCount) {
      M5.Lcd.setTextColor(BLACK, BLACK);
      M5.Lcd.drawString(dots, dotsX, y);
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.drawString(dots.substring(0, dotCount), dotsX, y);
      if (dotCount > 0) {
        playTypingClick('.');
      }
      lastDotCount = dotCount;
    }
    delay(20);
  }
}

}  // namespace stickui
