#pragma once

#include "Stage.h"

namespace stickui {

// The "playbill" scene: lists the stage's shows, one per row. Tap moves the highlight,
// double tap opens the show. The stage opens it itself when there is more than one show.
Scene& playbillScene();

}  // namespace stickui
