#include "petcore/Life.h"

namespace petcore {

using namespace tuning;

Life::Life(const Schedule& schedule, int64_t startLocalMs)
    : schedule_(schedule), timeMs_(startLocalMs), tracker_(startLocalMs) {}

void Life::advanceTo(int64_t localNowMs) { advanceBy(tracker_.elapsedSince(localNowMs)); }

void Life::advanceBy(int64_t ms) {
  if (ms <= 0) return;
  if (stage_ == LifeStage::Baby) {
    pet_->advanceBy(ms);
    return;
  }
  carryMs_ += ms;
  while (carryMs_ >= kEggStepMs && stage_ != LifeStage::Baby) {
    if (stage_ == LifeStage::Egg && egg_.step(timeMs_, kEggStepMs, schedule_, faceDown_)) {
      stage_ = LifeStage::Hatching;
      hatchingStartMs_ = timeMs_ + kEggStepMs;
    }
    timeMs_ += kEggStepMs;
    carryMs_ -= kEggStepMs;
    if (stage_ == LifeStage::Hatching && timeMs_ - hatchingStartMs_ >= kHatchingMs) {
      stage_ = LifeStage::Baby;
      pet_.emplace(schedule_, timeMs_);
      pet_->setFaceDown(faceDown_);
    }
  }
  if (stage_ == LifeStage::Baby && carryMs_ > 0) {
    pet_->advanceBy(carryMs_);  // the baby's own fixed steps take the leftover
    carryMs_ = 0;
  }
}

int64_t Life::localTimeMs() const {
  return stage_ == LifeStage::Baby ? pet_->localTimeMs() : timeMs_ + carryMs_;
}

void Life::strokeEgg() {
  if (stage_ == LifeStage::Egg) egg_.stroke(localTimeMs());
}
void Life::tapEgg() {
  if (stage_ == LifeStage::Egg) egg_.tap(localTimeMs());
}
void Life::shakeEgg() {
  if (stage_ == LifeStage::Egg) egg_.shake(localTimeMs());
}
void Life::tiltEgg(float leanDeg) {
  if (stage_ == LifeStage::Egg) egg_.tilt(localTimeMs(), leanDeg);
}

void Life::setFaceDown(bool faceDown) {
  faceDown_ = faceDown;
  if (pet_) pet_->setFaceDown(faceDown);
}

void Life::setScreenOn(bool on) { egg_.setScreenOn(on); }

}  // namespace petcore
