#pragma once

#include <Arduino.h>

#include "Choice.h"
#include "Scenes.h"
#include "Typewriter.h"

namespace stickui {

// Views for a "configure-app" scene. Each shows "config n of N" (n and N come from the
// scene), an instruction, and an interaction line, all centered and typed in order.

// "tap face" / "once then again": measures the gap between two taps and stores it (plus an
// allowance) as that button's double-tap window, then moves to the next view.
class ConfigureButtonView : public View {
 public:
  explicit ConfigureButtonView(Button button);

  void onEnter() override;
  void onResume(ResumeReason reason) override;
  void update(const ButtonInput& input) override;
  uint32_t frameDelayMs() const override { return 10; }

 private:
  void draw(size_t visibleCharacters, bool clearScreen);
  void resetTaps();

  Button button_;
  String label_;
  String instruction_;
  Typewriter typing_;
  bool firstTapDown_ = false;
  bool awaitingSecondTap_ = false;
  bool secondTapDown_ = false;
  uint8_t highlightedTap_ = 1;
  uint32_t firstReleaseAtMs_ = 0;
  uint32_t measuredGapMs_ = 0;
};

// "sound?" / "yes   no": tap toggles, double tap chooses, result lands in settings.
class ConfigureSoundView : public View {
 public:
  ConfigureSoundView() : View("configure-sound") {}

  void onEnter() override;
  void onResume(ResumeReason reason) override;
  void update(const ButtonInput& input) override;
  uint32_t frameDelayMs() const override { return 10; }

 private:
  void draw(size_t visibleCharacters, bool clearScreen);

  String label_;
  Typewriter typing_;
  ChoicePrompt prompt_;
};

}  // namespace stickui
