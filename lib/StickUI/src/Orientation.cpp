#include "Orientation.h"

#include <M5Unified.h>

#include "LogView.h"
#include "Text.h"
#include "Theme.h"

namespace stickui {

namespace {

const char* const kUpsideDownMessages[] = {
    "I am positioned upside down.",   "My world is inverted right now.",
    "I am hanging by my feet.",       "My head is near the ground.",
    "Everything looks flipped from here.", "I am currently upside down.",
    "My feet are above my head.",     "I am turned bottom to top.",
    "I am suspended upside down.",    "Up and down have swapped places.",
};
const char* const kCatSounds[] = {
    "Meow", "Miaow", "Miau", "Mew", "Meowth", "Maw", "Miaou", "Meoww", "Nya", "Meowf",
};

enum class HeldOrientation { Unknown, Normal, UpsideDown, PortraitPositiveY, PortraitNegativeY };

HeldOrientation readHeldOrientation() {
  float ax = 0;
  float ay = 0;
  float az = 0;
  if (!M5.Imu.isEnabled() || !M5.Imu.getAccel(&ax, &ay, &az)) {
    return HeldOrientation::Normal;
  }
  if (fabsf(ax) >= kOrientationThresholdG && fabsf(ax) >= fabsf(ay)) {
    return ax > 0 ? HeldOrientation::Normal : HeldOrientation::UpsideDown;
  }
  if (fabsf(ay) >= kOrientationThresholdG) {
    return ay > 0 ? HeldOrientation::PortraitPositiveY : HeldOrientation::PortraitNegativeY;
  }
  return HeldOrientation::Unknown;  // lying flat or mid-turn
}

void showHeldOrientationScreen(HeldOrientation orientation) {
  if (orientation == HeldOrientation::UpsideDown) {
    M5.Lcd.setRotation(kUpsideDownRotation);
    M5.Lcd.setTextSize(kTextSize);
    String lines[4];
    const size_t lineCount = wrapToScreenWidth(randomChoice(kUpsideDownMessages), lines, 4);
    typeFullScreenLines(lines, lineCount);
    return;
  }
  M5.Lcd.setRotation(orientation == HeldOrientation::PortraitPositiveY ? kPortraitRotationForPositiveY
                                                                       : kPortraitRotationForNegativeY);
  const String cat[] = {" /\\_/\\", "( o.o )", " > ^ <", "", randomChoice(kCatSounds)};
  typeFullScreenLines(cat, 5, 3);
}

}  // namespace

bool handleHeldOrientation(bool immediate) {
  static HeldOrientation pendingOrientation = HeldOrientation::Normal;
  static uint32_t pendingSinceMs = 0;
  const HeldOrientation orientation = readHeldOrientation();
  if (orientation == HeldOrientation::Unknown || orientation == HeldOrientation::Normal) {
    pendingSinceMs = 0;
    return false;
  }
  if (!immediate) {
    if (pendingSinceMs == 0 || pendingOrientation != orientation) {
      pendingOrientation = orientation;
      pendingSinceMs = millis();
      return false;
    }
    if (millis() - pendingSinceMs < kOrientationSettleMs) {
      return false;
    }
  }
  pendingSinceMs = 0;

  HeldOrientation shownOrientation = orientation;
  showHeldOrientationScreen(shownOrientation);
  HeldOrientation candidateOrientation = shownOrientation;
  uint32_t candidateSinceMs = 0;
  while (true) {
    M5.update();  // buttons are ignored on these screens
    const HeldOrientation current = readHeldOrientation();
    if (current == HeldOrientation::Unknown || current == shownOrientation) {
      candidateSinceMs = 0;
    } else if (candidateSinceMs == 0 || candidateOrientation != current) {
      candidateOrientation = current;
      candidateSinceMs = millis();
    } else if (millis() - candidateSinceMs >= kOrientationSettleMs) {
      if (current == HeldOrientation::Normal) {
        break;
      }
      shownOrientation = current;
      showHeldOrientationScreen(shownOrientation);
      candidateSinceMs = 0;
    }
    delay(20);
  }
  M5.Lcd.setRotation(kScreenRotation);
  M5.Lcd.fillScreen(BLACK);
  logView.markStale();
  return true;
}

}  // namespace stickui
