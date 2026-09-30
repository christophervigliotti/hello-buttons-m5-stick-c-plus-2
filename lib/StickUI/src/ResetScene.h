#pragma once

#include "Stage.h"

namespace stickui {

// The "reset-app" scene: one view asking "reset app?" with "yes   cancel". Yes, or a new
// hold of both buttons, shows "restarting..." and restarts the device; cancel returns to
// the scene that was showing. The stage uses it by default.
Scene& resetAppScene();

}  // namespace stickui
