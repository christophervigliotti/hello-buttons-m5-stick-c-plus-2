#pragma once

#include <StickUI.h>

// Full-screen alternative views for the main scene. Each types a random pick with clicks
// when it appears; the App has already rotated the screen for the held orientation.

// "cat-random": an ASCII cat with a random cat sound under it.
class CatRandomView : public stickui::View {
 public:
  CatRandomView() : View("cat-random") {}
  void onEnter() override;
};

// "dog-random": an ASCII dog with a random dog sound under it.
class DogRandomView : public stickui::View {
 public:
  DogRandomView() : View("dog-random") {}
  void onEnter() override;
};

// "sarcastic-random": a random remark about being upside down, word-wrapped.
class SarcasticRandomView : public stickui::View {
 public:
  SarcasticRandomView() : View("sarcastic-random") {}
  void onEnter() override;
};
