#include "Stage.h"

#include <M5Unified.h>

#include "ConfigViews.h"
#include "Header.h"
#include "LogView.h"
#include "Playbill.h"
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

void Stage::start() {
  begin();
  mainMenuShow().addScene(playbillScene());

  showFullScreenMessage("loading", kLoadingScreenMs);
  if (configureScene_ != nullptr) {
    configShow().addScene(*configureScene_);
    openShow(configShow());  // when it finishes, sceneFinished() opens the main menu
  } else {
    openMainMenu();
  }
  applyPending();
}

void Stage::update() {
  M5.update();
  const ButtonInput input = readInput();
  if (displayed_ == nullptr) {
    delay(20);  // nothing to show: no shows were added
    return;
  }

  if (orientation_.update()) {
    applyOrientation();
  }
  if (showingAlternative_) {
    displayed_->update(input);  // alternative views may react to buttons, but can't navigate
    delay(displayed_->frameDelayMs());
    return;
  }

  if (input.bothHeldLong && currentShow_ != &mainMenuShow()) {
    indicators.face = IndicatorState::LongPressed;
    indicators.side = IndicatorState::LongPressed;
    playPattern(buttonSounds.bothLong);
    if (currentShow_ == &configShow()) {
      openResetPrompt();  // configuration can't be skipped, but it can be abandoned
    } else {
      openMainMenu();
    }
  } else {
    displayed_->update(input);
  }
  applyPending();
  if (displayed_->doDisplayTitleBar() && !showingAlternative_ && !isHeaderTypingComplete()) {
    // Keep the title typing in between the view's own redraws.
    drawScreenFrame(false, displayed_->doDisplayTitleBarButtonHelpers());
  }
  delay(displayed_->frameDelayMs());
}

void Stage::openShow(Show& show) {
  previousShow_ = currentShow_;
  currentShow_ = &show;
  setTitle(show.title());
  goToScene(show.openingScene());
}

void Stage::openMainMenu() {
  previousShow_ = currentShow_;
  currentShow_ = &mainMenuShow();
  setTitle(currentShow_->title());
  goToScene(playbillScene());
}

void Stage::openResetPrompt() {
  previousShow_ = currentShow_;
  currentShow_ = &mainMenuShow();
  setTitle(currentShow_->title());
  resetReturnsToPrevious_ = true;
  pending_ = Pending::EnterScene;
  pendingScene_ = &playbillScene();
  pendingIndex_ = 1;  // the reset-prompt view
}

void Stage::resetPromptCancelled() {
  if (resetReturnsToPrevious_) {
    resetReturnsToPrevious_ = false;
    returnToPreviousScene();
  } else {
    playbillScene().goToView("show-list");
  }
}

void Stage::goToScene(Scene& scene) {
  pending_ = Pending::EnterScene;
  pendingScene_ = &scene;
  pendingIndex_ = 0;
}

void Stage::goToScene(const char* name) {
  if (currentShow_ != nullptr) {
    Scene* scene = currentShow_->findScene(name);
    if (scene != nullptr) {
      goToScene(*scene);
      return;
    }
  }
  for (Scene* scene : {configureScene_, &playbillScene()}) {
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
  (void)scene;
  openMainMenu();
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
      current_->currentIndex_ = pendingIndex_;
      showCurrentView();
      applyOrientation();  // the new scene may show something for how the device is held now
      break;
    case Pending::ResumeScene:
      current_ = pendingScene_;
      previous_ = nullptr;
      if (previousShow_ != nullptr) {
        currentShow_ = previousShow_;
        setTitle(currentShow_->title());
      }
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
