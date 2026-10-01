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
const uint16_t kSideIndicatorColor = 0xF800;  // red, left
const uint16_t kFaceIndicatorColor = 0x07E0;  // green, right
const uint16_t kGreyText = 0x7BEF;
const int kHeaderColor = 0x7A7A7A;

// Text roles (see TextRole in Text.h): text you read, text you could select, and the
// current selection. In left-justified lists the selection gets a green dot on its left
// once it has typed in; centered choice lines rely on color alone.
const uint16_t kReadingTextColor = 0xFFFF;
const uint16_t kSelectableTextColor = kGreyText;
const uint16_t kSelectedTextColor = 0xFFFF;
const uint16_t kSelectionDotColor = 0x07E0;  // green
const int kSelectionDotRadius = 3;
const int kSelectionDotGap = 8;              // between the dot and the text
const int kSelectableTextInset = 20;         // room for the dot in left-justified lists

// Scrollbar for lists with more items than fit: a narrow track at the right edge.
const int kScrollbarWidth = 2;
const int kScrollbarMargin = 3;  // from the right edge

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

// Orientation. "Up" is the normal landscape orientation (kScreenRotation). Scenes decide what
// to show in the other three (see Stage.h).
const uint8_t kScreenRotation = 1;
const uint8_t kUpsideDownRotation = 3;
const uint8_t kPortraitRotationForPositiveY = 0;  // swap these two if portrait views are upside down
const uint8_t kPortraitRotationForNegativeY = 2;
const bool kLeftSideUpIsPositiveY = false;  // checked on the device 2026-09-30
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

}  // namespace stickui
