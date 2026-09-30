#pragma once

// StickUI: a two-button UI kit for the M5StickC Plus2.
//
// A stage hosts shows; a show is a group of scenes; a scene is a group of views (see
// Stage.h). Typical main.cpp:
//
//   MyView myView;
//   Scene mainScene("main");
//   Show myShow("myShow");
//
//   void setup() {
//     mainScene.addView(myView);
//     myShow.addScene(mainScene);
//     stage.addShow(myShow).setConfigureScene(configureAppScene());
//     stage.start("stickUI");
//   }
//   void loop() { stage.update(); }

#include "Buttons.h"
#include "Choice.h"
#include "ConfigViews.h"
#include "Header.h"
#include "LogView.h"
#include "Orientation.h"
#include "Playbill.h"
#include "ResetScene.h"
#include "Sound.h"
#include "Stage.h"
#include "Text.h"
#include "Theme.h"
#include "Typewriter.h"

namespace stickui {

// Starts M5Unified, fixes the landscape rotation, and sets sound levels and the long-press
// threshold. Stage::start() calls this for you.
void begin();

}  // namespace stickui
