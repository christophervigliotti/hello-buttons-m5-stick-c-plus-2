#include "RandomViews.h"

#include <M5Unified.h>

using namespace stickui;

namespace {

const char* const kCatSounds[] = {
    "Meow", "Miaow", "Miau", "Mew", "Meowth", "Maw", "Miaou", "Meoww", "Nya", "Meowf",
};

const char* const kDogSounds[] = {
    "Woof", "Bark", "Ruff", "Arf", "Wuff", "Boof", "Yap", "Yip", "Baff", "Bow",
};

const char* const kUpsideDownMessages[] = {
    "I am positioned upside down.",   "My world is inverted right now.",
    "I am hanging by my feet.",       "My head is near the ground.",
    "Everything looks flipped from here.", "I am currently upside down.",
    "My feet are above my head.",     "I am turned bottom to top.",
    "I am suspended upside down.",    "Up and down have swapped places.",
};

}  // namespace

void CatRandomView::onEnter() {
  const String cat[] = {" /\\_/\\", "( o.o )", " > ^ <", "", randomChoice(kCatSounds)};
  typeFullScreenLines(cat, 5, 3);
}

void DogRandomView::onEnter() {
  const String dog[] = {"     __", "(___()'`;", "/,    /`", "\\\\\"--\\\\", "", randomChoice(kDogSounds)};
  typeFullScreenLines(dog, 6, 4);
}

void SarcasticRandomView::onEnter() {
  M5.Lcd.setTextSize(kTextSize);
  String lines[4];
  const size_t lineCount = wrapToScreenWidth(randomChoice(kUpsideDownMessages), lines, 4);
  typeFullScreenLines(lines, lineCount);
}
