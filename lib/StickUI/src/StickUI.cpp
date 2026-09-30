#include "StickUI.h"

namespace stickui {

Settings settings;

void begin(const char* appTitle) {
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Lcd.setRotation(kScreenRotation);
  beginSound();
  M5.BtnA.setHoldThresh(kLongPressThresholdMs);
  M5.BtnB.setHoldThresh(kLongPressThresholdMs);
  setAppTitle(appTitle);
}

}  // namespace stickui
