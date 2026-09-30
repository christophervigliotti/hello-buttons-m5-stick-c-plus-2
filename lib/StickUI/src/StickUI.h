#pragma once

// StickUI: a two-button UI kit for the M5StickC Plus2.
//
// Apps are composed from scenes and views (see Scenes.h). Typical main.cpp:
//
//   ConfigureButtonView configureFace(Button::Face), configureSide(Button::Side);
//   Scene configureApp("configure-app");
//   MyView myView;
//   Scene mainScene("main");
//
//   void setup() {
//     configureApp.addView(configureFace).addView(configureSide).then(mainScene);
//     mainScene.addView(myView);
//     app.addScene(configureApp).addScene(mainScene).setResetScene(resetAppScene());
//     app.start("myApp", configureApp);
//   }
//   void loop() { app.update(); }

#include "Buttons.h"
#include "Choice.h"
#include "ConfigViews.h"
#include "Header.h"
#include "LogView.h"
#include "Orientation.h"
#include "ResetScene.h"
#include "Scenes.h"
#include "Sound.h"
#include "Text.h"
#include "Theme.h"
#include "Typewriter.h"

namespace stickui {

// Starts M5Unified, fixes the landscape rotation, sets sound levels and the long-press
// threshold, and sets the header title. App::start() calls this for you.
void begin(const char* appTitle);

}  // namespace stickui
