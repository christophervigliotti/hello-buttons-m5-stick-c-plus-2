#pragma once

#include <StickUI.h>

// Full-screen alternative views for helloPets' main scene. Each types a random pick with
// clicks when it appears (every item once before any repeats), and a press of the face
// button loads another. The Stage has already rotated the screen for the held orientation.

// "cat-random": an ASCII cat with a random cat sound under it, under the title bar.
class CatRandomView : public stickui::View {
 public:
  CatRandomView() : View("cat-random") {}
  void onEnter() override;
  void update(const stickui::ButtonInput& input) override;
  bool doDisplayTitleBarButtonHelpers() const override { return false; }
};

// "dog-random": an ASCII dog with a random dog sound under it, under the title bar.
class DogRandomView : public stickui::View {
 public:
  DogRandomView() : View("dog-random") {}
  void onEnter() override;
  void update(const stickui::ButtonInput& input) override;
  bool doDisplayTitleBarButtonHelpers() const override { return false; }
};

// "quote-random": a random quote about cats or dogs with its attribution, no title bar.
// Quotes too long to fit the screen are kept in the list but never rendered.
class QuoteRandomView : public stickui::View {
 public:
  QuoteRandomView() : View("quote-random") {}
  void onEnter() override;
  void update(const stickui::ButtonInput& input) override;
  bool doDisplayTitleBar() const override { return false; }
};
