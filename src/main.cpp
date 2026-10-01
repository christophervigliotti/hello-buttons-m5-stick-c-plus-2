// A StickUI stage with two shows:
//   helloButtons  main: button-demo; upside-down shows sarcastic-random,
//                 left-side-down shows cat-random, left-side-up shows dog-random
//   pomodoro      a placeholder until the real show lands (see PlaceholderView.h)
//
// The stage itself provides the "config" show (configure-app, once at boot) and the "main
// menu" show (pick a show, or reset). Holding both buttons in a show opens the main menu.
#include <Arduino.h>
#include <StickUI.h>

#include "ButtonDemoView.h"
#include "PlaceholderView.h"
#include "RandomViews.h"

using namespace stickui;

namespace {

// The sound on/off config view is hidden for now; "config n of N" counts only added views.
const bool kSoundConfigViewEnabled = false;

ButtonDemoView buttonDemo;
CatRandomView catRandom;
DogRandomView dogRandom;
SarcasticRandomView sarcasticRandom;
Scene mainScene("main");
Show helloButtons("helloButtons");

PlaceholderView pomodoroPlaceholder;
Scene pomodoroScene("placeholder");
Show pomodoro("pomodoro");

}  // namespace

void setup() {
  mainScene.addView(buttonDemo)
      .showWhenHeld(Orientation::UpsideDown, sarcasticRandom)
      .showWhenHeld(Orientation::LeftSideDown, catRandom)
      .showWhenHeld(Orientation::LeftSideUp, dogRandom);
  helloButtons.addScene(mainScene);

  pomodoroScene.addView(pomodoroPlaceholder);
  pomodoro.addScene(pomodoroScene);

  stage.addShow(helloButtons).addShow(pomodoro).setConfigureScene(configureAppScene(kSoundConfigViewEnabled));
  stage.start();
}

void loop() {
  stage.update();
}
