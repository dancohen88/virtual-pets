#pragma once
#include <array>

#include "petcore/Tuning.h"

namespace petcore {

// The four needs. Each level is 0..100: 100 = fully met, 0 = desperate.
// So a high Hunger level means a full, contented pet.
enum class Need : int { Hunger = 0, Affection, Energy, Hygiene, Count };

constexpr int kNeedCount = static_cast<int>(Need::Count);

struct Needs {
  std::array<float, kNeedCount> level{tuning::kLevelMax, tuning::kLevelMax, tuning::kLevelMax,
                                      tuning::kLevelMax};

  float& operator[](Need n) { return level[static_cast<int>(n)]; }
  float operator[](Need n) const { return level[static_cast<int>(n)]; }

  void clampAll() {
    for (float& v : level) {
      if (v < 0.0f) v = 0.0f;
      if (v > tuning::kLevelMax) v = tuning::kLevelMax;
    }
  }
};

}  // namespace petcore
