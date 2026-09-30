#pragma once

#include <StickUI.h>

// Stand-in for a show that doesn't exist yet, so the playbill has something to list.
// Types the show's title, "nothing here yet" and "double tap: back"; a double tap ends
// the scene, which returns to the playbill. Delete once the real show lands.
class PlaceholderView : public stickui::View {
 public:
  PlaceholderView() : View("placeholder") {}

  void onEnter() override {
    prompt_ = stickui::ChoicePrompt{};
    prompt_.optionCount = 1;  // tap does nothing; double tap chooses
    title_ = stickui::stage.currentShow()->title();
    typing_.start(title_ + kLine2 + kLine3);
    draw(true);
  }

  void onResume(stickui::ResumeReason reason) override {
    (void)reason;
    prompt_ = stickui::ChoicePrompt{};
    prompt_.optionCount = 1;
    draw(true);
  }

  void update(const stickui::ButtonInput& input) override {
    bool redraw = false;
    if (stickui::processChoiceTaps(prompt_, input, redraw) == stickui::ChoiceResult::Chosen) {
      delay(stickui::kAfterUserInputDelayMs);
      stickui::indicators.reset();
      scene().goToNextView();  // last view of the last scene: back to the playbill
      return;
    }
    if (typing_.advance()) {
      redraw = true;
    }
    if (redraw) {
      draw(false);
    }
  }

  uint32_t frameDelayMs() const override { return 10; }

 private:
  static constexpr const char* kLine2 = "nothing here yet";
  static constexpr const char* kLine3 = "double tap: back";

  void draw(bool clearScreen) {
    using namespace stickui;
    drawScreenFrame(clearScreen);
    size_t remaining = typing_.visible();
    drawCenteredLine(title_, takeVisible(remaining, title_.length()), contentLineY(0), WHITE);
    drawCenteredLine(kLine2, takeVisible(remaining, strlen(kLine2)), contentLineY(1), kGreyText);
    drawCenteredLine(kLine3, takeVisible(remaining, strlen(kLine3)), contentLineY(2), kGreyText);
  }

  String title_;
  stickui::ChoicePrompt prompt_;
  stickui::Typewriter typing_;
};
