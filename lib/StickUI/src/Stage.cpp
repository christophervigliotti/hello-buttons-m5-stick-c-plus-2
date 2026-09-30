#include "Stage.h"

#include <M5Unified.h>

#include "Header.h"
#include "LogView.h"
#include "Playbill.h"
#include "ResetScene.h"
#include "Sound.h"
#include "StickUI.h"
#include "Text.h"

namespace stickui {

Stage stage;

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
      stage.requestView(*this, index);
      return;
    }
  }
}

void Scene::goToNextView() {
  if (currentIndex_ + 1 < viewCount_) {
    stage.requestView(*this, currentIndex_ + 1);
  } else if (next_ != nullptr) {
    stage.goToScene(*next_);
  } else {
    stage.sceneFinished(*this);
  }
}

// Show

Show& Show::addScene(Scene& scene) {
  if (sceneCount_ < kMaxScenes) {
    scenes_[sceneCount_++] = &scene;
    if (opening_ == nullptr) {
      opening_ = &scene;
    }
  }
  return *this;
}

Show& Show::opensWith(Scene& scene) {
  opening_ = &scene;
  return *this;
}

Scene* Show::findScene(const char* name) {
  for (size_t index = 0; index < sceneCount_; ++index) {
    if (strcmp(scenes_[index]->name(), name) == 0) {
      return scenes_[index];
    }
  }
  return nullptr;
}

// Stage

Stage& Stage::addShow(Show& show) {
  if (showCount_ < kMaxShows) {
    shows_[showCount_++] = &show;
  }
  return *this;
}

Stage& Stage::setConfigureScene(Scene& scene) {
  configureScene_ = &scene;
  return *this;
}

Stage& Stage::setResetScene(Scene& scene) {
  resetScene_ = &scene;
  return *this;
}

void Stage::start(const char* title) {
  begin();
  title_ = title;
  setAppTitle(title_);
  if (resetScene_ == nullptr) {
    resetScene_ = &resetAppScene();
  }
  showFullScreenMessage("loading", kLoadingScreenMs);

  if (configureScene_ != nullptr) {
    goToScene(*configureScene_);
  } else {
    afterConfigure();
  }
  applyPending();
}

void Stage::update() {
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

void Stage::openShow(Show& show) {
  currentShow_ = &show;
  setTitle(show.title());
  goToScene(show.openingScene());
}

void Stage::openPlaybill() {
  currentShow_ = nullptr;
  setTitle(title_);
  goToScene(playbillScene());
}

void Stage::goToScene(Scene& scene) {
  pending_ = Pending::EnterScene;
  pendingScene_ = &scene;
}

void Stage::goToScene(const char* name) {
  if (currentShow_ != nullptr) {
    Scene* scene = currentShow_->findScene(name);
    if (scene != nullptr) {
      goToScene(*scene);
      return;
    }
  }
  for (Scene* scene : {configureScene_, resetScene_, &playbillScene()}) {
    if (scene != nullptr && strcmp(scene->name(), name) == 0) {
      goToScene(*scene);
      return;
    }
  }
}

void Stage::returnToPreviousScene() {
  if (previous_ != nullptr) {
    pending_ = Pending::ResumeScene;
    pendingScene_ = previous_;
  }
}

void Stage::requestView(Scene& scene, size_t index) {
  pending_ = Pending::EnterView;
  pendingScene_ = &scene;
  pendingIndex_ = index;
}

void Stage::sceneFinished(Scene& scene) {
  if (&scene == configureScene_) {
    afterConfigure();
  } else if (showCount_ > 1) {
    openPlaybill();
  }
}

void Stage::afterConfigure() {
  if (showCount_ > 1) {
    openPlaybill();
  } else if (showCount_ == 1) {
    openShow(*shows_[0]);
  }
}

// Changes the header title; a different title types in again when the next screen draws.
void Stage::setTitle(const char* title) {
  if (strcmp(appTitle(), title) == 0) {
    return;
  }
  setAppTitle(title);
  restartHeaderTyping();
}

ButtonInput Stage::readInput() {
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

void Stage::applyOrientation() {
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

void Stage::applyPending() {
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

void Stage::showCurrentView() {
  M5.Lcd.setRotation(kScreenRotation);
  showingAlternative_ = false;
  displayed_ = &current_->currentView();
  displayed_->onEnter();
}

void Stage::resumeCurrentView(ResumeReason reason) {
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
