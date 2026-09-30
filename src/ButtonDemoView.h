#pragma once

#include <Arduino.h>
#include <StickUI.h>

// "button-demo": shows every button gesture as a line in the StickUI log
// ("face?", "face double", "side long", "both", ...) and settles back to "ready".
class ButtonDemoView : public stickui::View {
 public:
  ButtonDemoView() : View("button-demo") {}

  void onEnter() override;
  void onResume(stickui::ResumeReason reason) override;
  void update(const stickui::ButtonInput& input) override;

 private:
  enum class Gesture {
    Ready,
    FacePending,
    Face,
    FaceDouble,
    FaceLong,
    SidePending,
    Side,
    SideDouble,
    SideLong,
    Both  // both held long is handled by the Stage, which opens the reset scene
  };

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

  void setCurrentLineMessage(const String& message, bool restartTyping = false);
  bool resolvePendingStatusLine(const String& resolvedText);
  const stickui::ClickPattern* soundFor(Gesture gesture) const;
  void playGestureSound(Gesture gesture);
  bool setGesture(Gesture gesture);
  void showReadyState();
  bool readyCursorEligible() const;

  Gesture gesture_ = Gesture::Ready;
  stickui::Typewriter currentLineTyping_{"ready"};
  bool faceLongPressActive_ = false;
  bool sideLongPressActive_ = false;
  TapTracker faceTap_;
  TapTracker sideTap_;
  uint32_t lastStatusChangeMs_ = 0;
  bool statusNeedsReset_ = false;
  bool statusCountdownStarted_ = false;
  uint32_t lastDisplayedDotCount_ = UINT32_MAX;
};
