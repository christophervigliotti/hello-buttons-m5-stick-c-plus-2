#include "Playbill.h"

#include <M5Unified.h>

#include "Choice.h"
#include "Header.h"
#include "Sound.h"
#include "Text.h"
#include "Typewriter.h"

namespace stickui {

namespace {

const char kInstruction[] = "pick one";
const char kResetItem[] = "reset?";
const char kResetYes[] = "yes";
const char kResetCancel[] = "cancel";
const size_t kVisibleRows = 2;  // under the instruction line

class ShowListView : public View {
 public:
  ShowListView() : View("show-list") {}

  size_t itemCount() const { return stage.showCount() + 1; }  // shows, then "reset?"

  String itemText(size_t index) const {
    return index < stage.showCount() ? String(stage.show(index).title()) : String(kResetItem);
  }

  void onEnter() override {
    prompt_ = ChoicePrompt{};
    prompt_.optionCount = itemCount();
    String allText = kInstruction;
    for (size_t index = 0; index < itemCount(); ++index) {
      allText += itemText(index);
    }
    typing_.start(allText);
    firstRow_ = 0;
    draw(true);
  }

  void onResume(ResumeReason reason) override {
    (void)reason;
    prompt_ = ChoicePrompt{};
    prompt_.optionCount = itemCount();
    draw(true);
  }

  void update(const ButtonInput& input) override {
    bool redraw = false;
    if (processChoiceTaps(prompt_, input, redraw) == ChoiceResult::Chosen) {
      delay(kAfterUserInputDelayMs);
      indicators.reset();
      if (prompt_.selectedIndex < stage.showCount()) {
        stage.openShow(stage.show(prompt_.selectedIndex));
      } else {
        scene().goToView("reset-prompt");
      }
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
  // Keeps the selected item within the visible rows.
  void scrollToSelection() {
    if (prompt_.selectedIndex < firstRow_) {
      firstRow_ = prompt_.selectedIndex;
    } else if (prompt_.selectedIndex >= firstRow_ + kVisibleRows) {
      firstRow_ = prompt_.selectedIndex - kVisibleRows + 1;
    }
  }

  void draw(bool clearScreen) {
    const size_t previousFirstRow = firstRow_;
    scrollToSelection();
    drawScreenFrame(clearScreen);
    if (!clearScreen && firstRow_ != previousFirstRow) {
      M5.Lcd.fillRect(0, contentLineY(1), M5.Lcd.width(), kRowHeight * kVisibleRows, BLACK);
    }
    size_t remaining = typing_.visible();
    const String instruction = kInstruction;
    drawText(instruction, takeVisible(remaining, instruction.length()), kContentLeftX, contentLineY(0),
             TextRole::Reading);
    for (size_t index = 0; index < itemCount(); ++index) {
      const String text = itemText(index);
      const size_t visible = takeVisible(remaining, text.length());
      if (index < firstRow_ || index >= firstRow_ + kVisibleRows) {
        continue;
      }
      const TextRole role = index == prompt_.selectedIndex ? TextRole::Selected : TextRole::Selectable;
      drawText(text, visible, kContentLeftX + kSelectableTextInset, contentLineY(index - firstRow_ + 1), role,
               /*withDot=*/true);
    }
    drawScrollbar(1, kVisibleRows, itemCount(), kVisibleRows, firstRow_);
  }

  ChoicePrompt prompt_;
  Typewriter typing_;
  size_t firstRow_ = 0;
};

class ResetPromptView : public View {
 public:
  ResetPromptView() : View("reset-prompt") {}

  void onEnter() override {
    prompt_ = ChoicePrompt{};
    typing_.start(String(kResetItem) + " " + kResetYes + kChoiceSpacer + kResetCancel, 1);
    draw(true);
  }

  void update(const ButtonInput& input) override {
    bool redraw = false;
    // A new hold of both buttons (after any hold that opened the prompt is released) also resets.
    if (input.bothHeldLong && !prompt_.waitingForButtonsRelease) {
      playPattern(buttonSounds.bothLong);
      delay(kAfterUserInputDelayMs);
      restartApp();
    }

    if (processChoiceTaps(prompt_, input, redraw) == ChoiceResult::Chosen) {
      if (prompt_.selectedIndex == 0) {
        delay(kAfterUserInputDelayMs);
        restartApp();
      }
      indicators.reset();
      stage.resetPromptCancelled();
      return;
    }

    if (typing_.advance()) {
      redraw = true;
    }
    if (redraw) {
      draw(false);
    }
  }

 private:
  void draw(bool clearScreen) {
    drawScreenFrame(clearScreen);
    size_t remaining = typing_.visible();
    const String question = kResetItem;
    drawCenteredLine(question, takeVisible(remaining, question.length()), contentLineY(0), WHITE);
    takeVisible(remaining, 1);
    drawChoiceLine(kResetYes, kResetCancel, prompt_.selectedIndex == 0, remaining, contentLineY(2));
  }

  ChoicePrompt prompt_;
  Typewriter typing_;
};

}  // namespace

Show& mainMenuShow() {
  static Show show("main menu");
  return show;
}

Scene& playbillScene() {
  static ShowListView showList;
  static ResetPromptView resetPrompt;
  static Scene scene("playbill");
  static bool built = false;
  if (!built) {
    built = true;
    scene.addView(showList).addView(resetPrompt);
  }
  return scene;
}

}  // namespace stickui
