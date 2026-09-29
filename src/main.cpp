#include <Arduino.h>
#include <M5Unified.h>

namespace {

// For this project we use the two primary user buttons on the device.
const char* kButtonLabels[] = {"front", "top"};
const uint16_t kPressFrequency = 2600;
const uint32_t kStatusResetDelayMs = 2000;

void drawScreenFrame() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(0x7A7A7A, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.drawString("helloButtons", M5.Lcd.width() / 2, 18);

  M5.Lcd.drawFastHLine(0, 48, M5.Lcd.width(), 0x7A7A7A);
}

void showStatus(const String& message) {
  drawScreenFrame();
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.drawString(message, M5.Lcd.width() / 2, 68);
}

void showReadyState() {
  showStatus("ready for input");
}

void showButtonState(const char* label, bool longPress) {
  String message = String(label);
  message += longPress ? " long pressed" : " pressed";
  showStatus(message);
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
  showReadyState();

  M5.Speaker.setVolume(255);
  M5.BtnA.setHoldThresh(900);
  M5.BtnB.setHoldThresh(900);
  playStartupTone();
}

void loop() {
  static uint32_t lastStatusChangeMs = 0;
  static bool statusNeedsReset = false;

  M5.update();

  if (M5.BtnA.wasPressed()) {
    showButtonState(kButtonLabels[0], false);
    playPressTone(0);
    lastStatusChangeMs = millis();
    statusNeedsReset = true;
  }
  if (M5.BtnA.wasHold()) {
    showButtonState(kButtonLabels[0], true);
    playLongPressTone(0);
    lastStatusChangeMs = millis();
    statusNeedsReset = true;
  }

  if (M5.BtnB.wasPressed()) {
    showButtonState(kButtonLabels[1], false);
    playPressTone(1);
    lastStatusChangeMs = millis();
    statusNeedsReset = true;
  }
  if (M5.BtnB.wasHold()) {
    showButtonState(kButtonLabels[1], true);
    playLongPressTone(1);
    lastStatusChangeMs = millis();
    statusNeedsReset = true;
  }

  if (statusNeedsReset && (millis() - lastStatusChangeMs >= kStatusResetDelayMs)) {
    showReadyState();
    statusNeedsReset = false;
  }

  delay(20);
}
