#pragma once

namespace stickui {

// Boot flow shared by every app: "loading...", then "config n of N" screens that calibrate
// each button's double-tap window (and, when enabled, ask about sound). Results land in
// stickui::settings. Holding both buttons on any config screen opens the reset prompt.
void runSetup();

}  // namespace stickui
