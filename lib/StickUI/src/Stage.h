#pragma once

#include <Arduino.h>

#include "Buttons.h"
#include "Orientation.h"

// The stage hosts shows. A show is one self-contained app: a title and a group of scenes.
// A scene is a group of views. A view owns the screen and decides what the buttons do
// while it shows.
//
//   Scene mainScene("main");
//   mainScene.addView(buttonDemo)
//       .showWhenHeld(Orientation::UpsideDown, sarcasticRandom)
//       .showWhenHeld(Orientation::LeftSideDown, catRandom);
//   Show helloButtons("helloButtons");
//   helloButtons.addScene(mainScene);
//
//   void setup() {
//     stage.addShow(helloButtons).setConfigureScene(configureAppScene());
//     stage.start("stickUI");
//   }
//   void loop() { stage.update(); }
//
// Boot: "loading...", then the configure scene (once, if set), then the playbill to pick
// a show when there is more than one, otherwise straight into the only show.
//
// Each scene has one current view (moved with goToView / goToNextView). A scene can name
// an alternative view for any orientation other than Up; while the device is held that
// way the alternative shows instead (rotated to be readable, with no button input), and
// the current view comes back when the device returns to Up. Orientations a scene doesn't
// name are ignored: the current view stays put, unrotated.
//
// Holding both buttons long opens the reset scene from anywhere (not while an alternative
// view shows, since those ignore buttons). When a show's last scene finishes, the stage
// returns to the playbill.

namespace stickui {

class Scene;
class Show;

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

  // Scene to go to after goToNextView() on the last view. Without one, the scene is
  // finished: the configure scene continues booting, a show's scene returns to the playbill.
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
  friend class Stage;
  static const size_t kMaxViews = 8;

  const char* name_;
  View* views_[kMaxViews] = {};
  size_t viewCount_ = 0;
  size_t currentIndex_ = 0;
  View* alternatives_[kOrientationCount] = {};
  Scene* next_ = nullptr;
};

class Show {
 public:
  explicit Show(const char* title) : title_(title) {}

  const char* title() const { return title_; }

  // Scenes of the show; the first added one opens it unless opensWith() says otherwise.
  Show& addScene(Scene& scene);
  Show& opensWith(Scene& scene);

  size_t sceneCount() const { return sceneCount_; }
  Scene& openingScene() { return *opening_; }
  Scene* findScene(const char* name);

 private:
  static const size_t kMaxScenes = 8;

  const char* title_;
  Scene* scenes_[kMaxScenes] = {};
  size_t sceneCount_ = 0;
  Scene* opening_ = nullptr;
};

class Stage {
 public:
  Stage& addShow(Show& show);

  // Scene run once at boot before any show (e.g. configureAppScene()).
  Stage& setConfigureScene(Scene& scene);

  // Scene opened by holding both buttons long. Defaults to resetAppScene().
  Stage& setResetScene(Scene& scene);

  // Starts the hardware and boots. `title` is the header title outside any show (during
  // configuration and on the playbill). Call from setup().
  void start(const char* title);

  // Runs one frame. Call from loop().
  void update();

  void openShow(Show& show);
  void openPlaybill();

  // Within the current show first, then the stage's own scenes (configure, reset, playbill).
  void goToScene(Scene& scene);
  void goToScene(const char* name);
  void returnToPreviousScene();

  size_t showCount() const { return showCount_; }
  Show& show(size_t index) { return *shows_[index]; }
  Show* currentShow() { return currentShow_; }
  Scene& currentScene() { return *current_; }
  Orientation heldOrientation() const { return orientation_.held(); }

 private:
  friend class Scene;
  enum class Pending { None, EnterScene, ResumeScene, EnterView };
  static const size_t kMaxShows = 8;

  void requestView(Scene& scene, size_t index);
  void sceneFinished(Scene& scene);
  void afterConfigure();
  void setTitle(const char* title);
  ButtonInput readInput();
  void applyOrientation();
  void applyPending();
  void showCurrentView();
  void resumeCurrentView(ResumeReason reason);

  const char* title_ = "";
  Show* shows_[kMaxShows] = {};
  size_t showCount_ = 0;
  Show* currentShow_ = nullptr;
  Scene* configureScene_ = nullptr;
  Scene* resetScene_ = nullptr;
  Scene* current_ = nullptr;
  Scene* previous_ = nullptr;
  View* displayed_ = nullptr;
  bool showingAlternative_ = false;
  OrientationTracker orientation_;
  bool heldLong_[2] = {false, false};
  bool bothHeldLongFired_ = false;
  Pending pending_ = Pending::None;
  Scene* pendingScene_ = nullptr;
  size_t pendingIndex_ = 0;
};

extern Stage stage;

// Shows a full-screen "restarting..." then restarts the device.
void restartApp();

}  // namespace stickui
