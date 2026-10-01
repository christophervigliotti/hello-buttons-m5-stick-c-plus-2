#pragma once

#include <Arduino.h>

namespace stickui {

// Picks items from a list of N in random order without repeats: each next() returns an index
// not returned since the last reset; once every index has been returned, the bag refills and
// starts over.
template <size_t N>
class RandomBag {
 public:
  size_t next() {
    if (seenCount_ == N) {
      reset();
    }
    size_t remaining = random(N - seenCount_);
    for (size_t index = 0; index < N; ++index) {
      if (seen_[index]) {
        continue;
      }
      if (remaining == 0) {
        seen_[index] = true;
        ++seenCount_;
        return index;
      }
      --remaining;
    }
    return 0;  // unreachable
  }

  void reset() {
    for (size_t index = 0; index < N; ++index) {
      seen_[index] = false;
    }
    seenCount_ = 0;
  }

 private:
  bool seen_[N] = {};
  size_t seenCount_ = 0;
};

// Picks from a fixed array with a bag that lives as long as the array.
template <size_t N>
const char* randomUnseen(const char* const (&choices)[N]) {
  static RandomBag<N> bag;
  return choices[bag.next()];
}

}  // namespace stickui
