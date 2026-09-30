#pragma once

namespace stickui {

// Call once per frame from any screen. If the device has been held upside down or in
// portrait for a moment, shows the matching full screen (readable in that orientation:
// a random upside-down message, or a cat with a random cat sound) and waits, ignoring
// buttons, until it is back in the normal orientation. Returns true when the caller must
// redraw its screen. immediate=true skips the settle time (used at boot).
bool handleHeldOrientation(bool immediate = false);

}  // namespace stickui
