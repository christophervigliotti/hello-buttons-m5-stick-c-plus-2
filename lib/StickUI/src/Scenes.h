#pragma once

#include <Arduino.h>

#include "Buttons.h"
#include "Orientation.h"

// An app is a set of scenes; a scene is a group of views; a view owns the screen and
// decides what the buttons do while it shows.
//
//   Scene configureApp("configure-app");
//   configureApp.addView(configureFace).addView(configureSide).then(mainScene);
//
//   Scene mainScene("main");
//   mainScene.addView(buttonDemo)
//       .showWhenHeld(Orientation::UpsideDown, sarcasticRandom)
//       .showWhenHeld(Orientation::LeftSideUp, catRandom);
//
//   void setup() {
//     app.addScene(configureApp).addScene(mainScene).setResetScene(resetAppScene());
//     app.start("helloButtons", configureApp);
//   }
//   void loop() { app.update(); }
//
// Each scene has one current view (moved with goToView / goToNextView). A scene can also
// name an alternative view for any orientation other than Up; while the device is held
// that way the alternative shows instead (rotated to be readable, with no button input),
// and the current view comes back when the device returns to Up. Orientations a scene
// doesn't name are ignored: the current view stays put, unrotated.
//
// Holding both buttons long opens the reset scene from any other scene (not while an
// alternative view shows, since those ignore buttons).

namespace stickui {

class Scene;

enum class ResumeReason {
  SceneReturned,        // another scene (e.g. reset-app) was shown on top and was left
  OrientationRestored,  // an alternative view was shown and the device is back to Up
};

class View {
 public:
  explicit View(const char* name) : name_(name) {}
  virtual ~View() {}

  const char* name() const { return name_; }
  Scene& scene() { return *scene_; }

  // The view becomes current: start from scratch and draw the whole screen.
  virtual void onEnter() {}

  // The view shows again after being covered; the screen has been cleared.
  virtual void onResume(ResumeReason reason) {
    (void)reason;
    onEnter();
  }

  // Once per frame while the view shows. After navigating (goToNextView, goToScene, ...)
  // return right away; the change happens after update() returns.
  virtual void update(const ButtonInput& input) { (void)input; }

  // Pause between frames while this view shows.
  virtual uint32_t frameDelayMs() const { return 20; }

 private:
  friend class Scene;
  const char* name_;
  Scene* scene_ = nullptr;
};

class Scene {
 public:
  explicit Scene(const char* name) : name_(name) {}

  const char* name() const { return name_; }

  // Views in order; the scene starts at the first one.
  Scene& addView(View& view);

  // Shows `view` instead of the current view while the device is held in `orientation`
  // (not Up). The view shows rotated for that orientation and gets no button input.
  Scene& showWhenHeld(Orientation orientation, View& view);

  // Scene to go to after goToNextView() on the last view.
  Scene& then(Scene& nextScene);

  size_t viewCount() const { return viewCount_; }
  size_t indexOf(const View& view) const;
  View& currentView() { return *views_[currentIndex_]; }
  View* alternativeFor(Orientation orientation) const {
    return alternatives_[static_cast<size_t>(orientation)];
  }

  void goToView(const char* name);
  void goToNextView();

 private:
  friend class App;
  static const size_t kMaxViews = 8;

  const char* name_;
  View* views_[kMaxViews] = {};
  size_t viewCount_ = 0;
  size_t currentIndex_ = 0;
  View* alternatives_[kOrientationCount] = {};
  Scene* next_ = nullptr;
};

class App {
 public:
  App& addScene(Scene& scene);

  // Scene opened by holding both buttons long. It leaves with returnToPreviousScene().
  App& setResetScene(Scene& scene);

  // Starts the hardware, shows "loading...", then enters `firstScene`. Call from setup().
  void start(const char* title, Scene& firstScene);

  // Runs one frame. Call from loop().
  void update();

  void goToScene(Scene& scene);
  void goToScene(const char* name);
  void returnToPreviousScene();

  Scene& currentScene() { return *current_; }
  Orientation heldOrientation() const { return orientation_.held(); }

 private:
  friend class Scene;
  enum class Pending { None, EnterScene, ResumeScene, EnterView };
  static const size_t kMaxScenes = 8;

  void requestView(Scene& scene, size_t index);
  ButtonInput readInput();
  void applyOrientation();
  void applyPending();
  void showCurrentView();
  void resumeCurrentView(ResumeReason reason);

  Scene* scenes_[kMaxScenes] = {};
  size_t sceneCount_ = 0;
  Scene* current_ = nullptr;
  Scene* previous_ = nullptr;
  Scene* resetScene_ = nullptr;
  View* displayed_ = nullptr;
  bool showingAlternative_ = false;
  OrientationTracker orientation_;
  bool heldLong_[2] = {false, false};
  bool bothHeldLongFired_ = false;
  Pending pending_ = Pending::None;
  Scene* pendingScene_ = nullptr;
  size_t pendingIndex_ = 0;
};

extern App app;

// Shows a full-screen "restarting..." then restarts the device.
void restartApp();

}  // namespace stickui
