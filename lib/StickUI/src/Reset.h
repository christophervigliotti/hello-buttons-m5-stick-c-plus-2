#pragma once

#include <Arduino.h>

namespace stickui {

// "reset app?  yes   cancel". Yes restarts the device (after a full-screen "restarting...").

void askResetApp();
bool isResetPromptActive();

enum class ResetPromptOutcome { Open, Cancelled };

// Call once per frame after M5.update() while isResetPromptActive(). Never returns on yes.
// On Cancelled the prompt is closed and the caller should redraw its own screen.
ResetPromptOutcome updateResetPrompt();

// Hold-both detector for screens that don't track gestures themselves. Returns true once
// both buttons have been down together for kLongPressThresholdMs.
bool bothButtonsHeldLong(uint32_t& bothButtonsDownAtMs);

// Plays the both-long sound, then runs the reset prompt until the user cancels.
void runResetPromptModally();

void restartApp();

}  // namespace stickui
