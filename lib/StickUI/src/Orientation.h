#pragma once

#include <Arduino.h>

namespace stickui {

// How the device is being held. Up is the normal landscape orientation.
enum class Orientation { Up, UpsideDown, LeftSideUp, LeftSideDown };

const size_t kOrientationCount = 4;

// Screen rotation that makes text readable in `orientation`.
uint8_t rotationFor(Orientation orientation);

// Reads the accelerometer and reports a new orientation once it has been held for
// kOrientationSettleMs. Lying flat or mid-turn never changes it.
class OrientationTracker {
 public:
  // Returns true (and updates held()) when the held orientation changed.
  bool update();
  Orientation held() const { return held_; }

 private:
  Orientation held_ = Orientation::Up;
  Orientation pending_ = Orientation::Up;
  uint32_t pendingSinceMs_ = 0;
};

}  // namespace stickui
