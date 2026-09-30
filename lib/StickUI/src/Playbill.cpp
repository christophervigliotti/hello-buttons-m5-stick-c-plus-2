#include "Playbill.h"

#include <M5Unified.h>

#include "Choice.h"
#include "Header.h"
#include "Text.h"
#include "Typewriter.h"

namespace stickui {

namespace {

const size_t kVisibleRows = 3;

class PlaybillView : public View {
 public:
  PlaybillView() : View("show-list") {}

  void onEnter() override {
    prompt_ = ChoicePrompt{};
    prompt_.optionCount = stage.showCount();
    String allTitles;
    for (size_t index = 0; index < stage.showCount(); ++index) {
      allTitles += stage.show(index).title();
    }
    typing_.start(allTitles);
    firstRow_ = 0;
    draw(true);
  }

  void onResume(ResumeReason reason) override {
    (void)reason;
    prompt_ = ChoicePrompt{};
    prompt_.optionCount = stage.showCount();
    draw(true);
  }

  void update(const ButtonInput& input) override {
    bool redraw = false;
    if (processChoiceTaps(prompt_, input, redraw) == ChoiceResult::Chosen) {
      delay(kAfterUserInputDelayMs);
      indicators.reset();
      stage.openShow(stage.show(prompt_.selectedIndex));
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
  // Keeps the highlighted show within the three visible rows.
  void scrollToSelection() {
    if (prompt_.selectedIndex < firstRow_) {
      firstRow_ = prompt_.selectedIndex;
    } else if (prompt_.selectedIndex >= firstRow_ + kVisibleRows) {
      firstRow_ = prompt_.selectedIndex - kVisibleRows + 1;
    }
  }

  void draw(bool clearScreen) {
    scrollToSelection();
    drawScreenFrame(clearScreen);
    if (!clearScreen) {
      M5.Lcd.fillRect(0, kContentFirstLineY, M5.Lcd.width(), kRowHeight * kVisibleRows, BLACK);
    }
    size_t remaining = typing_.visible();
    for (size_t index = 0; index < stage.showCount(); ++index) {
      const String title = stage.show(index).title();
      const size_t visible = takeVisible(remaining, title.length());
      if (index < firstRow_ || index >= firstRow_ + kVisibleRows) {
        continue;
      }
      const uint16_t color = index == prompt_.selectedIndex ? WHITE : kGreyText;
      drawCenteredLine(title, visible, contentLineY(index - firstRow_), color);
    }
  }

  ChoicePrompt prompt_;
  Typewriter typing_;
  size_t firstRow_ = 0;
};

}  // namespace

Scene& playbillScene() {
  static PlaybillView view;
  static Scene scene("playbill");
  static bool built = false;
  if (!built) {
    built = true;
    scene.addView(view);
  }
  return scene;
}

}  // namespace stickui
