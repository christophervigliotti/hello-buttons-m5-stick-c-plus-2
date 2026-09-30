#include "Scenes.h"

#include <M5Unified.h>

#include "Header.h"
#include "LogView.h"
#include "Sound.h"
#include "StickUI.h"
#include "Text.h"

namespace stickui {

App app;

// Scene

Scene& Scene::addView(View& view) {
  if (viewCount_ < kMaxViews) {
    views_[viewCount_++] = &view;
    view.scene_ = this;
  }
  return *this;
}

Scene& Scene::showWhenHeld(Orientation orientation, View& view) {
  alternatives_[static_cast<size_t>(orientation)] = &view;
  view.scene_ = this;
  return *this;
}

Scene& Scene::then(Scene& nextScene) {
  next_ = &nextScene;
  return *this;
}

size_t Scene::indexOf(const View& view) const {
  for (size_t index = 0; index < viewCount_; ++index) {
    if (views_[index] == &view) {
      return index;
    }
  }
  return 0;
}

void Scene::goToView(const char* name) {
  for (size_t index = 0; index < viewCount_; ++index) {
    if (strcmp(views_[index]->name(), name) == 0) {
      app.requestView(*this, index);
      return;
    }
  }
}

void Scene::goToNextView() {
  if (currentIndex_ + 1 < viewCount_) {
    app.requestView(*this, currentIndex_ + 1);
  } else if (next_ != nullptr) {
    app.goToScene(*next_);
  }
}

// App

App& App::addScene(Scene& scene) {
  if (sceneCount_ < kMaxScenes) {
    scenes_[sceneCount_++] = &scene;
  }
  return *this;
}

App& App::setResetScene(Scene& scene) {
  resetScene_ = &scene;
  return *this;
}

void App::start(const char* title, Scene& firstScene) {
  begin(title);
  showFullScreenMessage("loading", kLoadingScreenMs);
  current_ = &firstScene;
  current_->currentIndex_ = 0;
  showCurrentView();
  applyOrientation();
}

void App::update() {
  M5.update();
  const ButtonInput input = readInput();

  if (orientation_.update()) {
    applyOrientation();
  }
  if (showingAlternative_) {
    delay(displayed_->frameDelayMs());  // alternative views ignore the buttons
    return;
  }

  if (input.bothHeldLong && resetScene_ != nullptr && current_ != resetScene_) {
    indicators.face = IndicatorState::LongPressed;
    indicators.side = IndicatorState::LongPressed;
    playPattern(buttonSounds.bothLong);
    goToScene(*resetScene_);
  } else {
    displayed_->update(input);
  }
  applyPending();
  delay(displayed_->frameDelayMs());
}

void App::goToScene(Scene& scene) {
  pending_ = Pending::EnterScene;
  pendingScene_ = &scene;
}

void App::goToScene(const char* name) {
  for (size_t index = 0; index < sceneCount_; ++index) {
    if (strcmp(scenes_[index]->name(), name) == 0) {
      goToScene(*scenes_[index]);
      return;
    }
  }
}

void App::returnToPreviousScene() {
  if (previous_ != nullptr) {
    pending_ = Pending::ResumeScene;
    pendingScene_ = previous_;
  }
}

void App::requestView(Scene& scene, size_t index) {
  pending_ = Pending::EnterView;
  pendingScene_ = &scene;
  pendingIndex_ = index;
}

ButtonInput App::readInput() {
  ButtonInput input;
  for (Button button : {Button::Face, Button::Side}) {
    m5::Button_Class& hardware = hardwareButton(button);
    ButtonState& state = button == Button::Face ? input.face : input.side;
    state.isPressed = hardware.isPressed();
    state.wasPressed = hardware.wasPressed();
    state.wasReleased = hardware.wasReleased();
    state.wasHeld = hardware.wasHold();

    bool& heldLong = heldLong_[static_cast<size_t>(button)];
    if (state.wasHeld) {
      heldLong = true;
    }
    if (!state.isPressed) {
      heldLong = false;
    }
  }
  if (heldLong_[0] && heldLong_[1]) {
    if (!bothHeldLongFired_) {
      bothHeldLongFired_ = true;
      input.bothHeldLong = true;
    }
  } else {
    bothHeldLongFired_ = false;
  }
  return input;
}

void App::applyOrientation() {
  const Orientation held = orientation_.held();
  View* alternative = held == Orientation::Up ? nullptr : current_->alternativeFor(held);
  if (alternative != nullptr) {
    showingAlternative_ = true;
    displayed_ = alternative;
    M5.Lcd.setRotation(rotationFor(held));
    alternative->onEnter();
  } else if (showingAlternative_) {
    resumeCurrentView(ResumeReason::OrientationRestored);
  }
}

void App::applyPending() {
  const Pending pending = pending_;
  pending_ = Pending::None;
  switch (pending) {
    case Pending::None:
      break;
    case Pending::EnterScene:
      previous_ = current_;
      current_ = pendingScene_;
      current_->currentIndex_ = 0;
      showCurrentView();
      applyOrientation();  // the new scene may show something for how the device is held now
      break;
    case Pending::ResumeScene:
      current_ = pendingScene_;
      previous_ = nullptr;
      resumeCurrentView(ResumeReason::SceneReturned);
      applyOrientation();
      break;
    case Pending::EnterView:
      if (pendingScene_ == current_) {
        current_->currentIndex_ = pendingIndex_;
        showCurrentView();
        applyOrientation();
      }
      break;
  }
}

void App::showCurrentView() {
  M5.Lcd.setRotation(kScreenRotation);
  showingAlternative_ = false;
  displayed_ = &current_->currentView();
  displayed_->onEnter();
}

void App::resumeCurrentView(ResumeReason reason) {
  M5.Lcd.setRotation(kScreenRotation);
  showingAlternative_ = false;
  displayed_ = &current_->currentView();
  M5.Lcd.fillScreen(BLACK);
  logView.markStale();
  displayed_->onResume(reason);
}

void restartApp() {
  showFullScreenMessage("restarting", kRestartingScreenMs);
  ESP.restart();
}

}  // namespace stickui
