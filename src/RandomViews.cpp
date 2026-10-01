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

struct Quote {
  const char* text;
  const char* author;
};

// Some of these are too long for the screen; QuoteRandomView skips those at render time.
const Quote kQuotes[] = {
    // Cats
    {"I have lived with several Zen masters - all of them cats.", "Eckhart Tolle"},
    {"A cat sits until it is done sitting, and then gets up, stretches, and walks away.", "Alan Watts"},
    {"When I was young I was like a tiger, but now I am like a cat!", "Shunryu Suzuki"},
    {"Of all animals, the cat alone attains to the contemplative life. He regards the wheel of "
     "existence from without, like the Buddha.", "Andrew Lang"},
    // Dogs
    {"Dogs and philosophers do the greatest good and get the fewest rewards.", "Diogenes of Sinope"},
    {"When a dog is tied to a cart, if it wants to follow, it is pulled and follows, making its "
     "spontaneous act coincide with necessity. But if the dog does not follow, it will be "
     "compelled in any case.", "Chrysippus of Soli"},
    {"Does a dog have Buddha-nature? Mu!", "Master Zhaozhou"},
    {"Everything is estimated by the standard of its own good. If a dog is to find the trail of "
     "a wild beast, keenness of scent is of value... So too in man, we must look to what is "
     "proper to human nature.", "Epictetus"},
    {"Learning to stay with ourselves in meditation is like training a dog.", "Pema Chodron"},
};
const size_t kQuoteCount = sizeof(kQuotes) / sizeof(kQuotes[0]);
const size_t kMaxQuoteLines = 8;

// Wraps the quote and its attribution; returns the line count, or 0 if it doesn't fit.
size_t layoutQuote(const Quote& quote, String lines[], size_t capacity) {
  M5.Lcd.setTextSize(kTextSize);
  const size_t quoteLines = wrapToScreenWidth(String("\"") + quote.text + "\"", lines, kMaxQuoteLines);
  const size_t attributionLines =
      wrapToScreenWidth(String("- ") + quote.author, lines + quoteLines, kMaxQuoteLines - quoteLines);
  const size_t total = quoteLines + attributionLines;
  const bool complete = lines[total - 1].length() > 0 &&
                        total < kMaxQuoteLines;  // wrap stops early when it runs out of lines
  return (complete && total <= capacity) ? total : 0;
}

}  // namespace

void CatRandomView::onEnter() {
  const String cat[] = {" /\\_/\\", "( o.o )", " > ^ <", "", randomUnseen(kCatSounds)};
  typeFullScreenLines(cat, 5, 3, /*underTitleBar=*/true);
}

void CatRandomView::update(const ButtonInput& input) {
  if (input.face.wasPressed) {
    onEnter();
  }
}

void DogRandomView::onEnter() {
  const String dog[] = {"     __", "(___()'`;", "/,    /`", "\\\\\"--\\\\", "", randomUnseen(kDogSounds)};
  typeFullScreenLines(dog, 6, 4, /*underTitleBar=*/true);
}

void DogRandomView::update(const ButtonInput& input) {
  if (input.face.wasPressed) {
    onEnter();
  }
}

void QuoteRandomView::onEnter() {
  static RandomBag<kQuoteCount> bag;
  const size_t capacity = fullScreenLineCapacity();
  String lines[kMaxQuoteLines];
  // Quotes that don't fit count as seen too, so they never block the rotation.
  for (size_t attempt = 0; attempt < 2 * kQuoteCount; ++attempt) {
    const size_t lineCount = layoutQuote(kQuotes[bag.next()], lines, capacity);
    if (lineCount > 0) {
      typeFullScreenLines(lines, lineCount);
      return;
    }
  }
  M5.Lcd.fillScreen(BLACK);  // no quote fits this screen
}

void QuoteRandomView::update(const ButtonInput& input) {
  if (input.face.wasPressed) {
    onEnter();
  }
}
