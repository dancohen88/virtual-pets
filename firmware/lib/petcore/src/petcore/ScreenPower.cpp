#include "petcore/ScreenPower.h"

#include "petcore/Tuning.h"

namespace petcore {

bool ScreenPower::update(bool activity) {
  if (activity) {
    const bool wasOff = level_ == ScreenLevel::Off;
    level_ = ScreenLevel::On;
    idleFrames_ = 0;
    return wasOff;
  }
  ++idleFrames_;
  if (idleFrames_ >= tuning::kScreenOffAfterFrames) {
    level_ = ScreenLevel::Off;
  } else if (idleFrames_ >= tuning::kScreenDimAfterFrames) {
    level_ = ScreenLevel::Dim;
  }
  return false;
}

}  // namespace petcore
