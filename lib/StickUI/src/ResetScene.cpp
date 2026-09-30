#include "ResetScene.h"

#include <M5Unified.h>

#include "Choice.h"
#include "Header.h"
#include "Sound.h"
#include "Text.h"
#include "Typewriter.h"

namespace stickui {

namespace {

const char kResetQuestion[] = "reset app?";
const char kResetYes[] = "yes";
const char kResetCancel[] = "cancel";

class ResetPromptView : public View {
 public:
  ResetPromptView() : View("reset-prompt") {}

  void onEnter() override {
    prompt_ = ChoicePrompt{};
    typing_.start(String(kResetQuestion) + " " + kResetYes + kChoiceSpacer + kResetCancel, 1);
    draw(true);
  }

  void update(const ButtonInput& input) override {
    bool redraw = false;
    // A new hold of both buttons (after the one that opened the prompt is released) also resets.
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
      stage.returnToPreviousScene();
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
    const String question = kResetQuestion;
    drawCenteredLine(question, takeVisible(remaining, question.length()), contentLineY(0), WHITE);
    takeVisible(remaining, 1);
    drawChoiceLine(kResetYes, kResetCancel, prompt_.selectedIndex == 0, remaining, contentLineY(2));
  }

  ChoicePrompt prompt_;
  Typewriter typing_;
};

}  // namespace

Scene& resetAppScene() {
  static ResetPromptView view;
  static Scene scene("reset-app");
  static bool built = false;
  if (!built) {
    built = true;
    scene.addView(view);
  }
  return scene;
}

}  // namespace stickui
