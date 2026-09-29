#include <Arduino.h>
#include <M5Unified.h>

namespace {

// For this project we use the two primary user buttons on the device.
const char* kButtonLabels[] = {"front", "top"};
const uint16_t kPressFrequency = 2600;

void showButtonState(const char* label, bool longPress) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextDatum(MC_DATUM);

  String message = String(label);
  message += longPress ? " long pressed" : " pressed";
  M5.Lcd.drawString(message, M5.Lcd.width() / 2, M5.Lcd.height() / 2);
}

void playStartupTone() {
  M5.Speaker.tone(3100, 90);
  delay(110);
  M5.Speaker.stop();
  delay(90);
  M5.Speaker.tone(4200, 110);
  delay(140);
  M5.Speaker.stop();
}

void playPressTone(uint8_t buttonIndex) {
  (void)buttonIndex;
  M5.Speaker.tone(kPressFrequency, 120);
  delay(150);
  M5.Speaker.stop();
}

void playLongPressTone(uint8_t buttonIndex) {
  (void)buttonIndex;
  M5.Speaker.tone(kPressFrequency, 110);
  delay(90);
  M5.Speaker.stop();
  delay(30);
  M5.Speaker.tone(kPressFrequency, 110);
  delay(140);
  M5.Speaker.stop();
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.drawString("Hello Buttons", M5.Lcd.width() / 2, 12);

  M5.Speaker.setVolume(255);
  M5.BtnA.setHoldThresh(900);
  M5.BtnB.setHoldThresh(900);
  playStartupTone();
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    showButtonState(kButtonLabels[0], false);
    playPressTone(0);
  }
  if (M5.BtnA.wasHold()) {
    showButtonState(kButtonLabels[0], true);
    playLongPressTone(0);
  }

  if (M5.BtnB.wasPressed()) {
    showButtonState(kButtonLabels[1], false);
    playPressTone(1);
  }
  if (M5.BtnB.wasHold()) {
    showButtonState(kButtonLabels[1], true);
    playLongPressTone(1);
  }

  delay(20);
}
