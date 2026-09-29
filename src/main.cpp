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
const uint16_t kCalibrationCharacterIntervalMs = 25;
const uint16_t kCalibrationAfterUserInputDelayMs = 350;
const uint32_t kLoadingScreenMs = 1000;
const uint32_t kRestartingScreenMs = 1000;
const uint16_t kGreyText = 0x7BEF;
// Master volume stays at 100%; each kind of sound gets its own channel and level.
const uint8_t kSpeakerVolume = 255;
const uint8_t kTypingSoundChannel = 0;
const uint8_t kTypingSoundVolume = 128;  // 50%
const uint8_t kButtonSoundChannel = 1;
const uint8_t kButtonSoundVolume = 191;  // 75%
const uint32_t kReadyCursorBlinkMs = 600;
const uint32_t kReadyCursorDelayMs = 1000;

// Held any way but the normal landscape orientation, the app pauses behind a full screen:
// "i'm upside down" in the other landscape orientation, a cat in either portrait orientation.
const uint8_t kScreenRotation = 1;
const uint8_t kUpsideDownRotation = 3;
const uint8_t kPortraitRotationForPositiveY = 0;  // swap these two if the cat is upside down
const uint8_t kPortraitRotationForNegativeY = 2;
const char* const kUpsideDownMessages[] = {
    "I am positioned upside down.",   "My world is inverted right now.",
    "I am hanging by my feet.",       "My head is near the ground.",
    "Everything looks flipped from here.", "I am currently upside down.",
    "My feet are above my head.",     "I am turned bottom to top.",
    "I am suspended upside down.",    "Up and down have swapped places.",
};
const char* const kCatSounds[] = {
    "Meow", "Miaow", "Miau", "Mew", "Meowth", "Maw", "Miaou", "Meoww", "Nya", "Meowf",
};
const float kOrientationThresholdG = 0.5f;
const uint32_t kOrientationSettleMs = 300;

// Typing click: a short tick for each typed character (not spaces) and each new status line.
const bool kTypingClicksEnabled = true;
const char kAppTitle[] = "helloButtons";
const char kResetPromptText[] = "reset app? yes   cancel";
const char kResetQuestion[] = "reset app?";
// The sound on/off screen is hidden for now; config labels count only active screens.
const bool kSoundConfigScreenEnabled = false;
const uint8_t kConfigScreenCount = kSoundConfigScreenEnabled ? 3 : 2;
const char kSoundQuestion[] = "sound?";
const char kChoiceSpacer[] = "   ";

struct HeaderTyping {
  size_t visibleTokens = 0;
  uint32_t lastTokenAtMs = 0;
  bool started = false;
};

struct ChoicePrompt {
  bool active = false;
  bool screenInitialized = false;
  bool waitingForButtonsRelease = true;
  bool yesSelected = true;
  bool pressInProgress = false;
  bool waitingForSecondTap = false;
  bool secondTapInProgress = false;
  uint8_t firstTapButton = 0;
  uint8_t pressedButton = 0;
  uint32_t firstReleaseAtMs = 0;
};

enum class ScreenViewMode { Redraw, NewLine };

struct StatusLine {
  String text;
  String dots;
  size_t visibleCharacters = 0;
};

HeaderTyping headerTyping;
ChoicePrompt resetPrompt;
ScreenViewMode screenViewMode = ScreenViewMode::Redraw;
StatusLine statusLines[3];
size_t statusLineCount = 0;
String renderedHistoryRows[3];
uint16_t renderedHistoryColors[3] = {0, 0, 0};
bool renderedHistoryRowsValid = false;
size_t renderedHistoryRowCount = 0;
bool readyCursorDrawn = false;

void doResetApp();
void askResetApp();

void playTypingClick(char character = '\0') {
  if (!kTypingClicksEnabled || !interactivityState.soundEnabled || character == ' ') {
    return;
  }
  M5.Speaker.tone(kTypingClickHz, kClickDurationMs, kTypingSoundChannel);
}

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

void drawScreenFrame(bool clearScreen = true) {
  const int leftIndicatorX = 22;
  const int rightIndicatorX = M5.Lcd.width() - leftIndicatorX;
  if (clearScreen) {
    screenViewMode = ScreenViewMode::Redraw;
    M5.Lcd.fillScreen(BLACK);
    renderedHistoryRowsValid = false;
    readyCursorDrawn = false;
  } else {
    M5.Lcd.fillRect(leftIndicatorX - 10, kTitleY - 2, 20, 22, BLACK);
    M5.Lcd.fillRect(rightIndicatorX - 10, kTitleY - 2, 20, 22, BLACK);
  }
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
  bool headerAdvanced = false;
  while (headerTyping.visibleTokens < totalHeaderTokens &&
         nowMs - headerTyping.lastTokenAtMs >= interactivityState.textIndicators.characterIntervalMs) {
    ++headerTyping.visibleTokens;
    headerTyping.lastTokenAtMs += interactivityState.textIndicators.characterIntervalMs;
    headerAdvanced = true;
  }
  if (headerAdvanced) {
    playTypingClick();
  }

  const size_t visibleTitleCharacters = min(headerTyping.visibleTokens, titleLength);
  const String visibleTitle = String(kAppTitle).substring(0, visibleTitleCharacters);
  const int titleX = (M5.Lcd.width() - M5.Lcd.textWidth(kAppTitle)) / 2;
  M5.Lcd.drawString(visibleTitle, titleX, kTitleY);

  if (headerTyping.visibleTokens > titleLength) {
    drawButtonIndicator(leftIndicatorX, kRedIndicator, interactivityState.circleIndicators.top, false);
  }
  if (headerTyping.visibleTokens > titleLength + 1) {
    drawButtonIndicator(rightIndicatorX, kGreenIndicator, interactivityState.circleIndicators.front, true);
  }

  M5.Lcd.drawFastHLine(0, kHeaderLineY, M5.Lcd.width(), 0x7A7A7A);
}

String configLabel(uint8_t screenNumber) {
  return String("config ") + screenNumber + " of " + kConfigScreenCount;
}

int contentLineY(size_t row) {
  return kContentFirstLineY + row * 24;
}

int centeredX(const String& text) {
  return (M5.Lcd.width() - M5.Lcd.textWidth(text.c_str())) / 2;
}

// Consumes up to `length` characters from a screen-wide typing budget.
size_t takeVisible(size_t& remaining, size_t length) {
  const size_t visible = min(remaining, length);
  remaining -= visible;
  return visible;
}

// Centers on the full text so partially typed lines don't shift as they grow.
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

// Full-screen message (no header) that types out, then animates dots for `durationMs`.
int fullScreenTextY() {
  return (M5.Lcd.height() - M5.Lcd.fontHeight()) / 2;
}

// Clears the screen and types `text` centered, leaving room for `reservedSuffix` after it.
// Returns the x just past the typed text.
int typeFullScreenText(const String& text, const String& reservedSuffix = "") {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(WHITE, BLACK);
  const int x = centeredX(text + reservedSuffix);
  for (size_t visible = 1; visible <= text.length(); ++visible) {
    M5.Lcd.drawString(text.substring(0, visible), x, fullScreenTextY());
    playTypingClick(text[visible - 1]);
    delay(kCalibrationCharacterIntervalMs);
  }
  return x + M5.Lcd.textWidth(text.c_str());
}

void showFullScreenMessage(const String& text, uint32_t durationMs) {
  screenViewMode = ScreenViewMode::Redraw;
  renderedHistoryRowsValid = false;
  const String dots = "...";
  const int dotsX = typeFullScreenText(text, dots);
  const int y = fullScreenTextY();

  const uint32_t startedAtMs = millis();
  size_t lastDotCount = SIZE_MAX;
  while (millis() - startedAtMs < durationMs) {
    const size_t dotCount = ((millis() - startedAtMs) / kButtonPulseIntervalMs) % 4;
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

enum class HeldOrientation { Unknown, Normal, UpsideDown, PortraitPositiveY, PortraitNegativeY };

HeldOrientation readHeldOrientation() {
  float ax = 0;
  float ay = 0;
  float az = 0;
  if (!M5.Imu.isEnabled() || !M5.Imu.getAccel(&ax, &ay, &az)) {
    return HeldOrientation::Normal;
  }
  if (fabsf(ax) >= kOrientationThresholdG && fabsf(ax) >= fabsf(ay)) {
    return ax > 0 ? HeldOrientation::Normal : HeldOrientation::UpsideDown;
  }
  if (fabsf(ay) >= kOrientationThresholdG) {
    return ay > 0 ? HeldOrientation::PortraitPositiveY : HeldOrientation::PortraitNegativeY;
  }
  return HeldOrientation::Unknown;  // lying flat or mid-turn
}

template <size_t N>
const char* randomChoice(const char* const (&choices)[N]) {
  return choices[random(N)];
}

// Word-wraps `text` to the screen width; returns the number of lines written.
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

// Types lines centered on screen. The first `alignedLineCount` lines are centered as a block
// with a shared left edge (for ASCII art); the rest are centered one by one.
void typeFullScreenLines(const String lines[], size_t lineCount, size_t alignedLineCount = 0) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
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
      delay(kCalibrationCharacterIntervalMs);
    }
  }
}

void showHeldOrientationScreen(HeldOrientation orientation) {
  if (orientation == HeldOrientation::UpsideDown) {
    M5.Lcd.setRotation(kUpsideDownRotation);
    M5.Lcd.setTextSize(2);
    String lines[4];
    const size_t lineCount = wrapToScreenWidth(randomChoice(kUpsideDownMessages), lines, 4);
    typeFullScreenLines(lines, lineCount);
    return;
  }
  M5.Lcd.setRotation(orientation == HeldOrientation::PortraitPositiveY ? kPortraitRotationForPositiveY
                                                                       : kPortraitRotationForNegativeY);
  const String cat[] = {" /\\_/\\", "( o.o )", " > ^ <", "", randomChoice(kCatSounds)};
  typeFullScreenLines(cat, 5, 3);
}

// If the device has been held upside down or in portrait for a moment, shows the matching
// full screen (readable in that orientation) and waits until it is back in the normal
// orientation. Returns true when the caller must redraw its screen.
bool handleHeldOrientation(bool immediate = false) {
  static HeldOrientation pendingOrientation = HeldOrientation::Normal;
  static uint32_t pendingSinceMs = 0;
  const HeldOrientation orientation = readHeldOrientation();
  if (orientation == HeldOrientation::Unknown || orientation == HeldOrientation::Normal) {
    pendingSinceMs = 0;
    return false;
  }
  if (!immediate) {
    if (pendingSinceMs == 0 || pendingOrientation != orientation) {
      pendingOrientation = orientation;
      pendingSinceMs = millis();
      return false;
    }
    if (millis() - pendingSinceMs < kOrientationSettleMs) {
      return false;
    }
  }
  pendingSinceMs = 0;

  HeldOrientation shownOrientation = orientation;
  showHeldOrientationScreen(shownOrientation);
  HeldOrientation candidateOrientation = shownOrientation;
  uint32_t candidateSinceMs = 0;
  while (true) {
    M5.update();  // buttons are ignored on these screens
    const HeldOrientation current = readHeldOrientation();
    if (current == HeldOrientation::Unknown || current == shownOrientation) {
      candidateSinceMs = 0;
    } else if (candidateSinceMs == 0 || candidateOrientation != current) {
      candidateOrientation = current;
      candidateSinceMs = millis();
    } else if (millis() - candidateSinceMs >= kOrientationSettleMs) {
      if (current == HeldOrientation::Normal) {
        break;
      }
      shownOrientation = current;
      showHeldOrientationScreen(shownOrientation);
      candidateSinceMs = 0;
    }
    delay(20);
  }
  M5.Lcd.setRotation(kScreenRotation);
  M5.Lcd.fillScreen(BLACK);
  renderedHistoryRowsValid = false;
  readyCursorDrawn = false;
  return true;
}

void enterNewLineView() {
  if (screenViewMode == ScreenViewMode::NewLine) {
    return;
  }
  drawScreenFrame();
  screenViewMode = ScreenViewMode::NewLine;
  statusLineCount = 0;
  renderedHistoryRowsValid = false;
}

void renderNewLineView(const String& dots = "") {
  if (screenViewMode != ScreenViewMode::NewLine) {
    return;
  }

  drawScreenFrame(false);
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextSize(2);
  if (statusLineCount > 0 && !dots.isEmpty()) {
    statusLines[statusLineCount - 1].dots = dots;
  }

  String renderedLines[9];
  bool renderedLineIsCurrent[9];
  size_t renderedLineCount = 0;
  const int availableWidth = M5.Lcd.width() - kContentLeftX * 2;
  for (size_t index = 0; index < statusLineCount; ++index) {
    const bool isCurrentLine = index == statusLineCount - 1;
    String remaining = statusLines[index].text.substring(0, statusLines[index].visibleCharacters) +
                       statusLines[index].dots;
    while (!remaining.isEmpty() && renderedLineCount < 9) {
      size_t fitCharacters = remaining.length();
      while (fitCharacters > 1 && M5.Lcd.textWidth(remaining.substring(0, fitCharacters).c_str()) > availableWidth) {
        --fitCharacters;
      }

      size_t breakAt = fitCharacters;
      if (fitCharacters < remaining.length()) {
        const int lastSpace = remaining.substring(0, fitCharacters + 1).lastIndexOf(' ');
        if (lastSpace > 0) {
          breakAt = static_cast<size_t>(lastSpace);
        }
      }

      renderedLines[renderedLineCount] = remaining.substring(0, breakAt);
      renderedLineIsCurrent[renderedLineCount] = isCurrentLine;
      ++renderedLineCount;
      remaining = remaining.substring(breakAt);
      while (!remaining.isEmpty() && remaining[0] == ' ') {
        remaining.remove(0, 1);
      }
    }
  }

  const size_t firstVisibleLine = renderedLineCount > 3 ? renderedLineCount - 3 : 0;
  String nextRows[3];
  uint16_t nextColors[3] = {BLACK, BLACK, BLACK};
  const size_t visibleRowCount = renderedLineCount - firstVisibleLine;
  for (size_t row = 0; row < visibleRowCount; ++row) {
    const size_t lineIndex = firstVisibleLine + row;
    nextRows[row] = renderedLines[lineIndex];
    nextColors[row] = renderedLineIsCurrent[lineIndex] ? WHITE : 0x7BEF;
  }

  for (size_t row = 0; row < 3; ++row) {
    if (!renderedHistoryRowsValid || nextRows[row] != renderedHistoryRows[row] ||
        nextColors[row] != renderedHistoryColors[row]) {
      const int rowY = kContentFirstLineY + row * 24;
      M5.Lcd.fillRect(0, rowY, M5.Lcd.width(), 24, BLACK);
      if (!nextRows[row].isEmpty()) {
        M5.Lcd.setTextColor(nextColors[row], BLACK);
        M5.Lcd.drawString(nextRows[row], kContentLeftX, rowY);
      }
      renderedHistoryRows[row] = nextRows[row];
      renderedHistoryColors[row] = nextColors[row];
      readyCursorDrawn = false;
    }
  }
  renderedHistoryRowsValid = true;
  renderedHistoryRowCount = visibleRowCount;
  M5.Lcd.setTextSize(2);
}

void appendStatusLine(const String& message) {
  enterNewLineView();
  if (statusLineCount == 3) {
    for (size_t index = 1; index < 3; ++index) {
      statusLines[index - 1] = statusLines[index];
    }
    --statusLineCount;
  }
  statusLines[statusLineCount].text = message;
  statusLines[statusLineCount].dots = "";
  statusLines[statusLineCount].visibleCharacters = interactivityState.textIndicators.visibleCharacters;
  ++statusLineCount;
  playTypingClick();
  renderNewLineView();
}

bool resolvePendingStatusLine(const String& resolvedText) {
  if (screenViewMode != ScreenViewMode::NewLine || statusLineCount == 0) {
    return false;
  }

  StatusLine& currentLine = statusLines[statusLineCount - 1];
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
  interactivityState.textIndicators.message = mergedText;
  interactivityState.textIndicators.visibleCharacters = preservedCharacters;
  interactivityState.textIndicators.lastCharacterAtMs = millis();
  renderNewLineView();
  return true;
}

void updateCurrentStatusLine() {
  if (screenViewMode != ScreenViewMode::NewLine || statusLineCount == 0) {
    return;
  }
  statusLines[statusLineCount - 1].visibleCharacters = interactivityState.textIndicators.visibleCharacters;
  renderNewLineView();
}

bool isHeaderTypingComplete() {
  return headerTyping.visibleTokens >= sizeof(kAppTitle) - 1 + 2;
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
  playTypingClick(textIndicators.message[textIndicators.visibleCharacters - 1]);
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

void playBeep(const StateBeepConfig& config) {
  if (!interactivityState.soundEnabled || !config.enabled || config.pulseCount == 0) {
    return;
  }

  // Blocks until each pulse has actually finished playing so nothing cuts it off.
  for (uint8_t pulse = 0; pulse < config.pulseCount; ++pulse) {
    M5.Speaker.tone(config.frequencyHz, config.durationMs, kButtonSoundChannel);
    const uint32_t startedAtMs = millis();
    while (M5.Speaker.isPlaying(kButtonSoundChannel) &&
           millis() - startedAtMs < config.durationMs + 100u) {
      delay(1);
    }
    if (pulse + 1 < config.pulseCount) {
      delay(config.gapMs);
    }
  }
}

void playStateBeep(InteractivityStateKind state) {
  playBeep(beepConfigForState(state));
}

// Redraw screens handle taps themselves; this adds the long and both-button sounds.
void playRedrawScreenButtonSounds() {
  const bool frontPressed = M5.BtnA.isPressed();
  const bool topPressed = M5.BtnB.isPressed();
  if ((M5.BtnA.wasPressed() && topPressed) || (M5.BtnB.wasPressed() && frontPressed)) {
    playBeep(interactivityState.beeps.both);
  } else if (M5.BtnA.wasHold() && !topPressed) {
    playBeep(interactivityState.beeps.frontLong);
  } else if (M5.BtnB.wasHold() && !frontPressed) {
    playBeep(interactivityState.beeps.topLong);
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
  setInteractivityState(InteractivityStateKind::Ready);
  interactivityState.circleIndicators.front = IndicatorState::Ready;
  interactivityState.circleIndicators.top = IndicatorState::Ready;
  interactivityState.frontLongPressActive = false;
  interactivityState.topLongPressActive = false;
  setTextIndicatorMessage("ready", true);
  appendStatusLine(interactivityState.textIndicators.message);
}

void showCalibrationPrompt(const String& label,
                           const String& instruction,
                           const String& progress,
                           uint8_t highlightedTap,
                           size_t visibleCharacters,
                           bool clearScreen) {
  drawScreenFrame(clearScreen);
  size_t remaining = visibleCharacters;
  drawCenteredLine(label, takeVisible(remaining, label.length()), contentLineY(0), kGreyText);
  drawCenteredLine(instruction, takeVisible(remaining, instruction.length()), contentLineY(1), WHITE);
  const size_t visibleProgress = takeVisible(remaining, progress.length());
  drawCenteredLine(progress, visibleProgress, contentLineY(2), kGreyText);

  const String highlight = highlightedTap == 1 ? "once" : "again";
  const int highlightIndex = progress.indexOf(highlight);
  if (highlightIndex >= 0 && visibleProgress >= highlightIndex + highlight.length()) {
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.drawString(highlight,
                      centeredX(progress) + M5.Lcd.textWidth(progress.substring(0, highlightIndex).c_str()),
                      contentLineY(2));
  }
}

void showResetAppPrompt() {
  drawScreenFrame(!resetPrompt.screenInitialized);
  resetPrompt.screenInitialized = true;
  size_t remaining = interactivityState.textIndicators.visibleCharacters;
  const String question = kResetQuestion;
  drawCenteredLine(question, takeVisible(remaining, question.length()), contentLineY(0), WHITE);
  takeVisible(remaining, 1);
  drawChoiceLine("yes", "cancel", resetPrompt.yesSelected, remaining, contentLineY(2));
}

void showSoundPrompt(const ChoicePrompt& prompt, size_t visibleCharacters, bool clearScreen) {
  drawScreenFrame(clearScreen);
  size_t remaining = visibleCharacters;
  const String label = configLabel(3);
  const String question = kSoundQuestion;
  drawCenteredLine(label, takeVisible(remaining, label.length()), contentLineY(0), kGreyText);
  drawCenteredLine(question, takeVisible(remaining, question.length()), contentLineY(1), WHITE);
  drawChoiceLine("yes", "no", prompt.yesSelected, remaining, contentLineY(2));
}

uint16_t resetTapWindowMs(uint8_t buttonId) {
  return buttonId == 1 ? interactivityState.frontDoubleTapWindowMs
                       : interactivityState.topDoubleTapWindowMs;
}

enum class ChoiceResult { None, Yes, No };

// Tap toggles the selection, double tap chooses it. Also mirrors presses on the header arrows.
ChoiceResult processChoiceTaps(ChoicePrompt& prompt, bool& redraw) {
  playRedrawScreenButtonSounds();
  const bool frontPressed = M5.BtnA.isPressed();
  const bool topPressed = M5.BtnB.isPressed();
  const bool frontWasPressed = M5.BtnA.wasPressed();
  const bool topWasPressed = M5.BtnB.wasPressed();
  const bool frontWasReleased = M5.BtnA.wasReleased();
  const bool topWasReleased = M5.BtnB.wasReleased();
  const bool frontWasHeld = M5.BtnA.wasHold();
  const bool topWasHeld = M5.BtnB.wasHold();
  const uint32_t nowMs = millis();

  IndicatorState& frontIndicator = interactivityState.circleIndicators.front;
  IndicatorState& topIndicator = interactivityState.circleIndicators.top;
  if (frontWasPressed) frontIndicator = IndicatorState::Pressed;
  if (frontWasHeld) frontIndicator = IndicatorState::LongPressed;
  if (frontWasReleased) frontIndicator = IndicatorState::Ready;
  if (topWasPressed) topIndicator = IndicatorState::Pressed;
  if (topWasHeld) topIndicator = IndicatorState::LongPressed;
  if (topWasReleased) topIndicator = IndicatorState::Ready;
  if (frontWasPressed || frontWasHeld || frontWasReleased ||
      topWasPressed || topWasHeld || topWasReleased) {
    redraw = true;
  }

  if (prompt.waitingForButtonsRelease) {
    if (!frontPressed && !topPressed) {
      prompt.waitingForButtonsRelease = false;
      redraw = true;
    }
  } else {
    if (prompt.waitingForSecondTap &&
        nowMs - prompt.firstReleaseAtMs >= resetTapWindowMs(prompt.firstTapButton)) {
      prompt.yesSelected = !prompt.yesSelected;
      prompt.waitingForSecondTap = false;
      redraw = true;
    }

    if (frontWasHeld || topWasHeld) {
      prompt.pressInProgress = false;
      prompt.secondTapInProgress = false;
      if (prompt.waitingForSecondTap) {
        prompt.yesSelected = !prompt.yesSelected;
        redraw = true;
      }
      prompt.waitingForSecondTap = false;
    }

    uint8_t pressedButton = 0;
    if (frontWasPressed && !topPressed) {
      pressedButton = 1;
    } else if (topWasPressed && !frontPressed) {
      pressedButton = 2;
    }

    if (pressedButton != 0) {
      if (prompt.waitingForSecondTap &&
          pressedButton == prompt.firstTapButton &&
          nowMs - prompt.firstReleaseAtMs < resetTapWindowMs(pressedButton)) {
        prompt.secondTapInProgress = true;
        prompt.pressInProgress = true;
        prompt.pressedButton = pressedButton;
      } else {
        if (prompt.waitingForSecondTap) {
          prompt.yesSelected = !prompt.yesSelected;
          redraw = true;
          prompt.waitingForSecondTap = false;
        }
        prompt.pressInProgress = true;
        prompt.secondTapInProgress = false;
        prompt.pressedButton = pressedButton;
      }
    }

    const bool matchingFrontRelease = frontWasReleased && prompt.pressedButton == 1;
    const bool matchingTopRelease = topWasReleased && prompt.pressedButton == 2;
    if (prompt.pressInProgress && (matchingFrontRelease || matchingTopRelease)) {
      prompt.pressInProgress = false;
      if (prompt.secondTapInProgress) {
        playBeep(prompt.pressedButton == 1 ? interactivityState.beeps.frontDouble
                                           : interactivityState.beeps.topDouble);
        return prompt.yesSelected ? ChoiceResult::Yes : ChoiceResult::No;
      }
      prompt.waitingForSecondTap = true;
      prompt.firstTapButton = prompt.pressedButton;
      prompt.firstReleaseAtMs = nowMs;
    }
  }
  return ChoiceResult::None;
}

ChoiceResult processResetAppPrompt() {
  bool redraw = false;
  if (handleHeldOrientation()) {
    resetPrompt.screenInitialized = false;
    redraw = true;
  }
  if (interactivityState.textIndicators.message != kResetPromptText) {
    setTextIndicatorMessage(kResetPromptText, true);
    redraw = true;
  }

  const ChoiceResult result = processChoiceTaps(resetPrompt, redraw);
  if (result != ChoiceResult::None) {
    return result;
  }

  if (advanceTextIndicatorTyping()) {
    redraw = true;
  }
  if (redraw) {
    showResetAppPrompt();
  }
  return ChoiceResult::None;
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

// Runs the reset prompt from a config screen; returns only if the user cancels.
void runResetPromptDuringSetup() {
  playBeep(interactivityState.beeps.bothLong);
  askResetApp();
  while (resetPrompt.active) {
    M5.update();
    const ChoiceResult result = processResetAppPrompt();
    if (result == ChoiceResult::Yes) {
      delay(kCalibrationAfterUserInputDelayMs);
      doResetApp();
    } else if (result == ChoiceResult::No) {
      resetPrompt.active = false;
      setInteractivityState(InteractivityStateKind::Ready);
      interactivityState.circleIndicators.front = IndicatorState::Ready;
      interactivityState.circleIndicators.top = IndicatorState::Ready;
    }
    delay(20);
  }
}

template <typename ButtonType>
uint16_t calibrateDoubleTapWindow(ButtonType& button, ButtonType& otherButton, const char* buttonName,
                                  const char* configLabel) {
  bool firstTapDown = false;
  bool awaitingSecondTap = false;
  bool secondTapDown = false;
  uint8_t highlightedTap = 1;
  uint32_t firstReleaseAtMs = 0;
  uint32_t measuredGapMs = 0;
  size_t visibleCharacters = 0;
  uint32_t lastCharacterAtMs = millis();
  uint32_t bothButtonsDownAtMs = 0;
  const String instruction = String("tap ") + buttonName;
  const String progress = "once then again";
  const String label = configLabel;
  const size_t totalCharacters = label.length() + instruction.length() + progress.length();

  showCalibrationPrompt(label, instruction, progress, highlightedTap, visibleCharacters, true);

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
    playRedrawScreenButtonSounds();
    if (handleHeldOrientation()) {
      showCalibrationPrompt(label, instruction, progress, highlightedTap,
                            isHeaderTypingComplete() ? visibleCharacters : 0, true);
    }

    IndicatorState& calibrationIndicator = buttonName[0] == 'f'
                                               ? interactivityState.circleIndicators.front
                                               : interactivityState.circleIndicators.top;
    if (wasPressed) {
      calibrationIndicator = IndicatorState::Pressed;
      shouldRedraw = true;
    }
    if (wasHeld) {
      calibrationIndicator = IndicatorState::LongPressed;
      shouldRedraw = true;
    }
    if (wasReleased) {
      calibrationIndicator = IndicatorState::Ready;
      shouldRedraw = true;
    }

    if (!isHeaderTypingComplete()) {
      showCalibrationPrompt(label, instruction, progress, highlightedTap, 0, false);
      lastCharacterAtMs = millis();
      delay(10);
      continue;
    }

    if (bothButtonsHeldLong(bothButtonsDownAtMs)) {
      runResetPromptDuringSetup();
      firstTapDown = false;
      awaitingSecondTap = false;
      secondTapDown = false;
      highlightedTap = 1;
      showCalibrationPrompt(label, instruction, progress, highlightedTap, visibleCharacters, true);
      delay(10);
      continue;
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
        playBeep(buttonName[0] == 'f' ? interactivityState.beeps.frontDouble
                                      : interactivityState.beeps.topDouble);
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

    if (visibleCharacters < totalCharacters &&
        millis() - lastCharacterAtMs >= kCalibrationCharacterIntervalMs) {
      ++visibleCharacters;
      lastCharacterAtMs = millis();
      playTypingClick((label + instruction + progress)[visibleCharacters - 1]);
      shouldRedraw = true;
    }
    if (shouldRedraw) {
      showCalibrationPrompt(label, instruction, progress, highlightedTap, visibleCharacters, false);
    }
    if (calibrationComplete) {
      delay(kCalibrationAfterUserInputDelayMs);
      return static_cast<uint16_t>(measuredGapMs + kDoubleTapAllowanceMs);
    }
    delay(10);
  }
}

// Config 3 of 3. Returns whether sound should be on.
bool configureSound() {
  ChoicePrompt prompt;
  prompt.active = true;
  const String soundPromptText = configLabel(3) + kSoundQuestion + "yes" + kChoiceSpacer + "no";
  const size_t totalCharacters = soundPromptText.length();
  size_t visibleCharacters = 0;
  uint32_t lastCharacterAtMs = millis();
  uint32_t bothButtonsDownAtMs = 0;

  showSoundPrompt(prompt, visibleCharacters, true);
  while (true) {
    M5.update();
    if (handleHeldOrientation()) {
      showSoundPrompt(prompt, visibleCharacters, true);
    }
    if (bothButtonsHeldLong(bothButtonsDownAtMs)) {
      runResetPromptDuringSetup();
      prompt = ChoicePrompt{};
      prompt.active = true;
      showSoundPrompt(prompt, visibleCharacters, true);
      delay(10);
      continue;
    }

    bool redraw = false;
    const ChoiceResult result = processChoiceTaps(prompt, redraw);
    if (result != ChoiceResult::None) {
      delay(kCalibrationAfterUserInputDelayMs);
      return result == ChoiceResult::Yes;
    }

    if (visibleCharacters < totalCharacters &&
        millis() - lastCharacterAtMs >= kCalibrationCharacterIntervalMs) {
      ++visibleCharacters;
      lastCharacterAtMs = millis();
      playTypingClick(soundPromptText[visibleCharacters - 1]);
      redraw = true;
    }
    if (redraw) {
      showSoundPrompt(prompt, visibleCharacters, false);
    }
    delay(10);
  }
}

// Slow blinking box after the current "ready" line, starting (with a click) a second after
// it finishes typing.
void updateReadyCursor() {
  static int cursorX = 0;
  static int cursorY = 0;
  static uint32_t readySinceMs = 0;
  static bool cursorAppeared = false;
  const bool showCursor = screenViewMode == ScreenViewMode::NewLine && statusLineCount > 0 &&
                          renderedHistoryRowCount > 0 &&
                          interactivityState.state == InteractivityStateKind::Ready &&
                          statusLines[statusLineCount - 1].text == "ready" &&
                          statusLines[statusLineCount - 1].dots.isEmpty() && !isTextIndicatorTyping();
  if (!showCursor) {
    readySinceMs = 0;
    cursorAppeared = false;
  } else if (readySinceMs == 0) {
    readySinceMs = millis();
  }
  const uint32_t readyElapsedMs = showCursor ? millis() - readySinceMs : 0;
  const bool cursorOn = showCursor && readyElapsedMs >= kReadyCursorDelayMs &&
                        ((readyElapsedMs - kReadyCursorDelayMs) / kReadyCursorBlinkMs) % 2 == 0;
  if (cursorOn == readyCursorDrawn) {
    return;
  }
  if (cursorOn && !cursorAppeared) {
    cursorAppeared = true;
    playTypingClick();
  }

  const int cursorWidth = M5.Lcd.textWidth("0") - 2;
  const int cursorHeight = M5.Lcd.fontHeight();
  if (cursorOn) {
    const size_t row = renderedHistoryRowCount - 1;
    cursorX = kContentLeftX + M5.Lcd.textWidth(renderedHistoryRows[row].c_str()) + 2;
    cursorY = contentLineY(row);
  }
  M5.Lcd.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, cursorOn ? WHITE : BLACK);
  readyCursorDrawn = cursorOn;
}

void doResetApp() {
  showFullScreenMessage("restarting", kRestartingScreenMs);
  ESP.restart();
}

void askResetApp() {
  resetPrompt = ChoicePrompt{};
  resetPrompt.active = true;
  setInteractivityState(InteractivityStateKind::ResetConfirmation);
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Lcd.setRotation(kScreenRotation);
  M5.Speaker.setVolume(kSpeakerVolume);
  M5.Speaker.setChannelVolume(kTypingSoundChannel, kTypingSoundVolume);
  M5.Speaker.setChannelVolume(kButtonSoundChannel, kButtonSoundVolume);
  M5.BtnA.setHoldThresh(kLongPressThresholdMs);
  M5.BtnB.setHoldThresh(kLongPressThresholdMs);

  handleHeldOrientation(true);
  showFullScreenMessage("loading", kLoadingScreenMs);

  interactivityState.frontDoubleTapWindowMs =
      calibrateDoubleTapWindow(M5.BtnA, M5.BtnB, "front", configLabel(1).c_str());
  interactivityState.topDoubleTapWindowMs =
      calibrateDoubleTapWindow(M5.BtnB, M5.BtnA, "top", configLabel(2).c_str());
  if (kSoundConfigScreenEnabled) {
    interactivityState.soundEnabled = configureSound();
  }
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

  if (resetPrompt.active) {
    const ChoiceResult result = processResetAppPrompt();
    if (result == ChoiceResult::Yes) {
      delay(kCalibrationAfterUserInputDelayMs);
      doResetApp();
    } else if (result == ChoiceResult::No) {
      resetPrompt.active = false;
      statusNeedsReset = false;
      statusCountdownStarted = false;
      showReadyState();
    }
    delay(20);
    return;
  }

  if (handleHeldOrientation()) {
    renderNewLineView();
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
    interactivityState.circleIndicators.front = IndicatorState::DoublePressed;
    interactivityState.circleIndicators.top = IndicatorState::Ready;
  } else if (topDoubleTapResolved) {
    nextState = InteractivityStateKind::TopDouble;
    nextMessage = "top double";
    interactivityState.circleIndicators.top = IndicatorState::DoublePressed;
    interactivityState.circleIndicators.front = IndicatorState::Ready;
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
  if (resetPrompt.active) {
    delay(20);
    return;
  }
  const bool pressEvent = frontWasPressed || topWasPressed;
  const bool secondTapStarted = (frontWasPressed && frontTap.secondTapInProgress) ||
                                (topWasPressed && topTap.secondTapInProgress);
  if (pressEvent && !stateChanged) {
    playStateBeep(interactivityState.state);
  }
  setTextIndicatorMessage(nextMessage, pressEvent && !secondTapStarted);
  const bool statusLineChanged = stateChanged || (pressEvent && !secondTapStarted);
  const bool pendingLineResolved = stateChanged && resolvePendingStatusLine(nextMessage);
  if (statusLineChanged && !pendingLineResolved) {
    appendStatusLine(interactivityState.textIndicators.message);
  }
  if (stateChanged || pressEvent) {
    statusCountdownStarted = false;
  }

  const bool textAdvanced = advanceTextIndicatorTyping();
  if (textAdvanced && !statusLineChanged) {
    updateCurrentStatusLine();
  }

  static uint32_t lastDisplayedDotCount = UINT32_MAX;
  if (statusNeedsReset) {
    if (!frontPressed && !topPressed) {
      const bool tapPending = frontTap.waitingForSecondTap || frontTap.pressInProgress ||
                              topTap.waitingForSecondTap || topTap.pressInProgress;
      if (tapPending || isTextIndicatorTyping()) {
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
          interactivityState.circleIndicators.front = IndicatorState::Ready;
          interactivityState.circleIndicators.top = IndicatorState::Ready;
          interactivityState.frontLongPressActive = false;
          interactivityState.topLongPressActive = false;
          setTextIndicatorMessage("ready", true);
          appendStatusLine("ready");
          statusNeedsReset = false;
          statusCountdownStarted = false;
          lastDisplayedDotCount = UINT32_MAX;
        } else {
          const uint32_t dotCount = min<uint32_t>(3u, elapsedMs / kButtonPulseIntervalMs);
          if (dotCount != lastDisplayedDotCount) {
            String dots;
            for (uint32_t index = 0; index < dotCount; ++index) {
              dots += ".";
            }
            renderNewLineView(dots);
            if (dotCount > 0) {
              playTypingClick('.');
            }
            lastDisplayedDotCount = dotCount;
          }
        }
      }
    }
  }

  updateReadyCursor();
  delay(20);
}
