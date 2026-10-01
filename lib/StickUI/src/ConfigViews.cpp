#include "ConfigViews.h"

#include <M5Unified.h>

#include "Header.h"
#include "Sound.h"
#include "Text.h"

namespace stickui {

namespace {

const char kCalibrationProgress[] = "once then again";
const char kSoundQuestion[] = "sound?";

String configLabel(View& view) {
  return String("config ") + (view.scene().indexOf(view) + 1) + " of " + view.scene().viewCount();
}

}  // namespace

// ConfigureButtonView

ConfigureButtonView::ConfigureButtonView(Button button)
    : View(button == Button::Face ? "configure-face" : "configure-side"), button_(button) {}

void ConfigureButtonView::onEnter() {
  label_ = configLabel(*this);
  instruction_ = String("tap ") + buttonName(button_);
  typing_.start(label_ + instruction_ + kCalibrationProgress);
  resetTaps();
  draw(typing_.visible(), true);
}

void ConfigureButtonView::onResume(ResumeReason reason) {
  (void)reason;
  resetTaps();
  indicators.reset();
  draw(typing_.visible(), true);
}

void ConfigureButtonView::resetTaps() {
  firstTapDown_ = false;
  awaitingSecondTap_ = false;
  secondTapDown_ = false;
  highlightedTap_ = 1;
}

void ConfigureButtonView::draw(size_t visibleCharacters, bool clearScreen) {
  const String progress = kCalibrationProgress;
  drawScreenFrame(clearScreen);
  size_t remaining = visibleCharacters;
  drawCenteredLine(label_, takeVisible(remaining, label_.length()), contentLineY(0), kGreyText);
  drawCenteredLine(instruction_, takeVisible(remaining, instruction_.length()), contentLineY(1), WHITE);
  const size_t visibleProgress = takeVisible(remaining, progress.length());
  drawCenteredLine(progress, visibleProgress, contentLineY(2), kGreyText);

  const String highlight = highlightedTap_ == 1 ? "once" : "again";
  const int highlightIndex = progress.indexOf(highlight);
  if (highlightIndex >= 0 && visibleProgress >= highlightIndex + highlight.length()) {
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.drawString(highlight,
                      centeredX(progress) + M5.Lcd.textWidth(progress.substring(0, highlightIndex).c_str()),
                      contentLineY(2));
  }
}

void ConfigureButtonView::update(const ButtonInput& input) {
  const ButtonState& button = input[button_];
  bool shouldRedraw = false;
  bool calibrationComplete = false;
  playButtonSounds(input);

  IndicatorState& indicator = indicators.forButton(button_);
  if (button.wasPressed) {
    indicator = IndicatorState::Pressed;
    shouldRedraw = true;
  }
  if (button.wasHeld) {
    indicator = IndicatorState::LongPressed;
    shouldRedraw = true;
  }
  if (button.wasReleased) {
    indicator = IndicatorState::Ready;
    shouldRedraw = true;
  }

  if (!isHeaderTypingComplete()) {
    draw(0, false);
    typing_.restartClock();
    return;
  }

  if (awaitingSecondTap_ && millis() - firstReleaseAtMs_ > kMaximumCalibrationGapMs) {
    awaitingSecondTap_ = false;
    highlightedTap_ = 1;
    shouldRedraw = true;
  }

  if (button.wasHeld) {
    resetTaps();
    shouldRedraw = true;
  }

  if (button.wasPressed) {
    if (awaitingSecondTap_) {
      measuredGapMs_ = millis() - firstReleaseAtMs_;
      if (measuredGapMs_ <= kMaximumCalibrationGapMs) {
        secondTapDown_ = true;
        highlightedTap_ = 2;
      } else {
        awaitingSecondTap_ = false;
        firstTapDown_ = true;
        highlightedTap_ = 1;
      }
      shouldRedraw = true;
    } else if (!firstTapDown_) {
      firstTapDown_ = true;
    }
  }

  if (button.wasReleased) {
    if (secondTapDown_) {
      secondTapDown_ = false;
      calibrationComplete = true;
      playPattern(buttonSounds.doubleTap);
      highlightedTap_ = 2;
      shouldRedraw = true;
    } else if (firstTapDown_) {
      firstTapDown_ = false;
      awaitingSecondTap_ = true;
      firstReleaseAtMs_ = millis();
      highlightedTap_ = 2;
      shouldRedraw = true;
    }
  }

  if (typing_.advance()) {
    shouldRedraw = true;
  }
  if (shouldRedraw) {
    draw(typing_.visible(), false);
  }
  if (calibrationComplete) {
    delay(kAfterUserInputDelayMs);
    setDoubleTapWindowMs(button_, static_cast<uint16_t>(measuredGapMs_ + kDoubleTapAllowanceMs));
    scene().goToNextView();
  }
}

// ConfigureSoundView

void ConfigureSoundView::onEnter() {
  label_ = configLabel(*this);
  typing_.start(label_ + kSoundQuestion + "yes" + kChoiceSpacer + "no");
  prompt_ = ChoicePrompt{};
  draw(typing_.visible(), true);
}

void ConfigureSoundView::onResume(ResumeReason reason) {
  (void)reason;
  prompt_ = ChoicePrompt{};
  draw(typing_.visible(), true);
}

void ConfigureSoundView::draw(size_t visibleCharacters, bool clearScreen) {
  drawScreenFrame(clearScreen);
  size_t remaining = visibleCharacters;
  const String question = kSoundQuestion;
  drawCenteredLine(label_, takeVisible(remaining, label_.length()), contentLineY(0), kGreyText);
  drawCenteredLine(question, takeVisible(remaining, question.length()), contentLineY(1), WHITE);
  drawChoiceLine("yes", "no", prompt_.selectedIndex == 0, remaining, contentLineY(2));
}

void ConfigureSoundView::update(const ButtonInput& input) {
  bool redraw = false;
  if (processChoiceTaps(prompt_, input, redraw) == ChoiceResult::Chosen) {
    delay(kAfterUserInputDelayMs);
    settings.soundEnabled = prompt_.selectedIndex == 0;
    scene().goToNextView();
    return;
  }

  if (typing_.advance()) {
    redraw = true;
  }
  if (redraw) {
    draw(typing_.visible(), false);
  }
}

Show& configShow() {
  static Show show("config");
  return show;
}

Scene& configureAppScene(bool includeSoundView) {
  static ConfigureButtonView configureFace(Button::Face);
  static ConfigureButtonView configureSide(Button::Side);
  static ConfigureSoundView configureSound;
  static Scene scene("configure-app");
  static bool built = false;
  if (!built) {
    built = true;
    scene.addView(configureFace).addView(configureSide);
    if (includeSoundView) {
      scene.addView(configureSound);
    }
  }
  return scene;
}

}  // namespace stickui
