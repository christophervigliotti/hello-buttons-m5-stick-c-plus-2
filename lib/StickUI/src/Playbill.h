#pragma once

#include "Stage.h"

namespace stickui {

// The built-in "main menu" show; the stage puts the playbill in it at start().
Show& mainMenuShow();

// The "playbill" scene. View "show-list": "pick one" over the stage's shows and a final
// "reset?" item, left-justified; tap moves the selection, double tap opens it. View
// "reset-prompt": "reset?  yes   cancel"; yes (or a new hold of both buttons) restarts the
// device, cancel goes back.
Scene& playbillScene();

}  // namespace stickui
