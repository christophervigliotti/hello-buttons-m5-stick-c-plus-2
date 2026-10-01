#include "Header.h"

#include <M5Unified.h>

#include "LogView.h"
#include "Sound.h"

namespace stickui {

Indicators indicators;

namespace {

const char* title = "";

struct HeaderTyping {
  size_t visibleTokens = 0;
  uint32_t lastTokenAtMs = 0;
  bool started = false;
} headerTyping;

size_t totalHeaderTokens() {
  return strlen(title) + 2;  // title characters, then the two indicators
}

void drawButtonIndicator(int x, uint16_t color, IndicatorState state, bool pointRight) {
  if (state == IndicatorState::Ready) {
    M5.Lcd.setTextColor(color, BLACK);
    M5.Lcd.drawString(".", x, kTitleY);
  } else if (state == IndicatorState::DoublePressed) {
    const int centerY = kTitleY + 9;
    const int radius = 4;
    const int direction = pointRight ? 1 : -1;
    for (int offset : {-4, 4}) {
      const int arrowCenterX = x + direction * offset;
      const int tipX = arrowCenterX + direction * radius;
      const int baseX = arrowCenterX - direction * radius;
      M5.Lcd.fillTriangle(tipX, centerY, baseX, centerY - radius,
                          baseX, centerY + radius, color);
    }
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

}  // namespace

void setAppTitle(const char* newTitle) {
  title = newTitle;
}

const char* appTitle() {
  return title;
}

void restartHeaderTyping() {
  headerTyping = HeaderTyping{};
}

void drawScreenFrame(bool clearScreen, bool withButtonHelpers) {
  const int leftIndicatorX = kIndicatorX;
  const int rightIndicatorX = M5.Lcd.width() - kIndicatorX;
  if (clearScreen) {
    logView.deactivate();
    M5.Lcd.fillScreen(BLACK);
  } else {
    M5.Lcd.fillRect(leftIndicatorX - 10, kTitleY - 2, 20, 22, BLACK);
    M5.Lcd.fillRect(rightIndicatorX - 10, kTitleY - 2, 20, 22, BLACK);
  }
  M5.Lcd.setTextColor(kHeaderColor, BLACK);
  M5.Lcd.setTextSize(kTextSize);
  M5.Lcd.setTextDatum(TL_DATUM);
  const size_t titleLength = strlen(title);
  const uint32_t nowMs = millis();
  if (!headerTyping.started) {
    headerTyping.started = true;
    headerTyping.lastTokenAtMs = nowMs;
  }
  bool headerAdvanced = false;
  while (headerTyping.visibleTokens < totalHeaderTokens() &&
         nowMs - headerTyping.lastTokenAtMs >= kTypingIntervalMs) {
    ++headerTyping.visibleTokens;
    headerTyping.lastTokenAtMs += kTypingIntervalMs;
    headerAdvanced = true;
  }
  if (headerAdvanced) {
    playTypingClick();
  }

  const size_t visibleTitleCharacters = min(headerTyping.visibleTokens, titleLength);
  const String visibleTitle = String(title).substring(0, visibleTitleCharacters);
  const int titleX = (M5.Lcd.width() - M5.Lcd.textWidth(title)) / 2;
  M5.Lcd.drawString(visibleTitle, titleX, kTitleY);

  if (withButtonHelpers && headerTyping.visibleTokens > titleLength) {
    drawButtonIndicator(leftIndicatorX, kSideIndicatorColor, indicators.side, false);
  }
  if (withButtonHelpers && headerTyping.visibleTokens > titleLength + 1) {
    drawButtonIndicator(rightIndicatorX, kFaceIndicatorColor, indicators.face, true);
  }

  M5.Lcd.drawFastHLine(0, kHeaderLineY, M5.Lcd.width(), kHeaderColor);
}

bool isHeaderTypingComplete() {
  return headerTyping.visibleTokens >= totalHeaderTokens();
}

}  // namespace stickui
