// helloButtons: a StickUI demo that logs every button gesture.
//
// Scenes:
//   configure-app  configure-face, configure-side (configure-sound when enabled)
//   main           button-demo; upside-down shows sarcastic-random,
//                  left-side-down shows cat-random, left-side-up shows dog-random
//   reset-app      opened from any scene by holding both buttons
#include <Arduino.h>
#include <StickUI.h>

#include "ButtonDemoView.h"
#include "RandomViews.h"

using namespace stickui;

namespace {

// The sound on/off config view is hidden for now; "config n of N" counts only added views.
const bool kSoundConfigViewEnabled = false;

ConfigureButtonView configureFace(Button::Face);
ConfigureButtonView configureSide(Button::Side);
ConfigureSoundView configureSound;
Scene configureApp("configure-app");

ButtonDemoView buttonDemo;
CatRandomView catRandom;
DogRandomView dogRandom;
SarcasticRandomView sarcasticRandom;
Scene mainScene("main");

}  // namespace

void setup() {
  configureApp.addView(configureFace).addView(configureSide);
  if (kSoundConfigViewEnabled) {
    configureApp.addView(configureSound);
  }
  configureApp.then(mainScene);

  mainScene.addView(buttonDemo)
      .showWhenHeld(Orientation::UpsideDown, sarcasticRandom)
      .showWhenHeld(Orientation::LeftSideDown, catRandom)
      .showWhenHeld(Orientation::LeftSideUp, dogRandom);

  app.addScene(configureApp).addScene(mainScene).setResetScene(resetAppScene());
  app.start("helloButtons", configureApp);
}

void loop() {
  app.update();
}
