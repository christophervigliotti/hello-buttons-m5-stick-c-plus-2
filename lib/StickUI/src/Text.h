#pragma once

#include <Arduino.h>

namespace stickui {

const char kChoiceSpacer[] = "   ";

// Y of content row 0, 1 or 2 under the header.
int contentLineY(size_t row);

int centeredX(const String& text);

// Consumes up to `length` characters from a screen-wide typing budget.
size_t takeVisible(size_t& remaining, size_t length);

// Centers on the full text so partially typed lines don't shift as they grow.
void drawCenteredLine(const String& text, size_t visibleCharacters, int y, uint16_t color);

// "first   second", centered, with the selected option white and the other grey.
void drawChoiceLine(const String& first, const String& second, bool firstSelected,
                    size_t visibleCharacters, int y);

// Word-wraps `text` to the screen width; returns the number of lines written.
size_t wrapToScreenWidth(const String& text, String lines[], size_t maxLines);

template <size_t N>
const char* randomChoice(const char* const (&choices)[N]) {
  return choices[random(N)];
}

// Full screens (no header). These block while they type.

// Clears the screen and types `text` centered, leaving room for `reservedSuffix` after it.
// Returns the x just past the typed text.
int typeFullScreenText(const String& text, const String& reservedSuffix = "");

// Types lines centered on screen. The first `alignedLineCount` lines are centered as a block
// with a shared left edge (for ASCII art); the rest are centered one by one.
void typeFullScreenLines(const String lines[], size_t lineCount, size_t alignedLineCount = 0);

// Types `text`, then animates dots after it for `durationMs` (e.g. "loading...").
void showFullScreenMessage(const String& text, uint32_t durationMs);

}  // namespace stickui
