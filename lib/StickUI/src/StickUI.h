#pragma once

// StickUI: a two-button UI kit for the M5StickC Plus2.
//
// Typical app:
//   void setup() {
//     stickui::begin("myApp");
//     stickui::runSetup();              // loading + double-tap calibration
//     ...                               // draw your first screen
//   }
//   void loop() {
//     M5.update();
//     if (stickui::isResetPromptActive()) { ... stickui::updateResetPrompt() ...; return; }
//     if (stickui::handleHeldOrientation()) { ... redraw your screen ... }
//     ...
//   }

#include "Buttons.h"
#include "Choice.h"
#include "Header.h"
#include "LogView.h"
#include "Orientation.h"
#include "Reset.h"
#include "Setup.h"
#include "Sound.h"
#include "Text.h"
#include "Theme.h"
#include "Typewriter.h"

namespace stickui {

// Starts M5Unified, fixes the landscape rotation, sets sound levels and the long-press
// threshold, and sets the header title.
void begin(const char* appTitle);

}  // namespace stickui
