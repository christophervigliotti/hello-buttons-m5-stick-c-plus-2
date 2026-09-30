#pragma once

#include "Scenes.h"

namespace stickui {

// The "reset-app" scene: one view asking "reset app?" with "yes   cancel". Yes, or a new
// hold of both buttons, shows "restarting..." and restarts the device; cancel returns to
// the scene that was showing. Register it with app.setResetScene().
Scene& resetAppScene();

}  // namespace stickui
