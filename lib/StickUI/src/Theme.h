#pragma once

#include <Arduino.h>

// Shared look, timing and sound levels for every StickUI app.
namespace stickui {

// Layout (landscape, text size 2)
const uint8_t kTextSize = 2;
const int kTitleY = 10;
const int kHeaderLineY = 40;
const int kContentFirstLineY = 50;
const int kContentLeftX = 12;
const int kRowHeight = 24;
const int kIndicatorX = 22;  // distance of each header indicator from its screen edge

// Colors
const uint16_t kRedIndicator = 0xF800;
const uint16_t kGreenIndicator = 0x07E0;
const uint16_t kGreyText = 0x7BEF;
const int kHeaderColor = 0x7A7A7A;

// Timing
const uint32_t kLongPressThresholdMs = 900;
const uint16_t kTypingIntervalMs = 25;
const uint32_t kDotIntervalMs = 250;
const uint16_t kAfterUserInputDelayMs = 350;
const uint32_t kLoadingScreenMs = 1000;
const uint32_t kRestartingScreenMs = 1000;
const uint32_t kCursorBlinkMs = 600;
const uint32_t kCursorDelayMs = 1000;

// Double-tap calibration
const uint16_t kDefaultDoubleTapWindowMs = 300;
const uint16_t kDoubleTapAllowanceMs = 100;
const uint16_t kMaximumCalibrationGapMs = 1000;

// Held any way but the normal landscape orientation, the app pauses behind a full screen:
// an upside-down message in the other landscape orientation, a cat in either portrait one.
const uint8_t kScreenRotation = 1;
const uint8_t kUpsideDownRotation = 3;
const uint8_t kPortraitRotationForPositiveY = 0;  // swap these two if the cat is upside down
const uint8_t kPortraitRotationForNegativeY = 2;
const float kOrientationThresholdG = 0.5f;
const uint32_t kOrientationSettleMs = 300;

// Sound. Master volume stays at 100%; each kind of sound gets its own channel and level.
const uint8_t kSpeakerVolume = 255;
const uint8_t kTypingSoundChannel = 0;
const uint8_t kTypingSoundVolume = 128;  // 50%
const uint8_t kButtonSoundChannel = 1;
const uint8_t kButtonSoundVolume = 191;  // 75%

// Typing click: a short tick for each typed character (not spaces) and each new log line.
const bool kTypingClicksEnabled = true;
const uint16_t kTypingClickHz = 4000;
const uint16_t kButtonClickHz = 4490;  // two semitones above the typing click
const uint16_t kClickDurationMs = 4;

// Setup. The sound on/off screen is hidden for now; config labels count only active screens.
const bool kSoundConfigScreenEnabled = false;
const uint8_t kConfigScreenCount = kSoundConfigScreenEnabled ? 3 : 2;

}  // namespace stickui
