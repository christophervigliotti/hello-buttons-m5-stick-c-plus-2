#include "Orientation.h"

#include <M5Unified.h>

#include "Theme.h"

namespace stickui {

namespace {

// Returns false while lying flat or mid-turn.
bool readOrientation(Orientation& orientation) {
  float ax = 0;
  float ay = 0;
  float az = 0;
  if (!M5.Imu.isEnabled() || !M5.Imu.getAccel(&ax, &ay, &az)) {
    orientation = Orientation::Up;
    return true;
  }
  if (fabsf(ax) >= kOrientationThresholdG && fabsf(ax) >= fabsf(ay)) {
    orientation = ax > 0 ? Orientation::Up : Orientation::UpsideDown;
    return true;
  }
  if (fabsf(ay) >= kOrientationThresholdG) {
    orientation = (ay > 0) == kLeftSideUpIsPositiveY ? Orientation::LeftSideUp
                                                     : Orientation::LeftSideDown;
    return true;
  }
  return false;
}

}  // namespace

uint8_t rotationFor(Orientation orientation) {
  switch (orientation) {
    case Orientation::Up:
      return kScreenRotation;
    case Orientation::UpsideDown:
      return kUpsideDownRotation;
    case Orientation::LeftSideUp:
      return kLeftSideUpIsPositiveY ? kPortraitRotationForPositiveY : kPortraitRotationForNegativeY;
    case Orientation::LeftSideDown:
      return kLeftSideUpIsPositiveY ? kPortraitRotationForNegativeY : kPortraitRotationForPositiveY;
  }
  return kScreenRotation;
}

bool OrientationTracker::update() {
  Orientation current = Orientation::Up;
  if (!readOrientation(current) || current == held_) {
    pendingSinceMs_ = 0;
    return false;
  }
  if (pendingSinceMs_ == 0 || pending_ != current) {
    pending_ = current;
    pendingSinceMs_ = millis();
    return false;
  }
  if (millis() - pendingSinceMs_ < kOrientationSettleMs) {
    return false;
  }
  pendingSinceMs_ = 0;
  held_ = current;
  return true;
}

}  // namespace stickui
