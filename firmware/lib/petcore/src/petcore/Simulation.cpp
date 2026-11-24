#include "petcore/Simulation.h"

#include "petcore/TimeUtil.h"

namespace petcore {

using namespace tuning;

Simulation::Simulation(const Schedule& schedule, int64_t startLocalMs)
    : schedule_(schedule), simTimeMs_(startLocalMs), tracker_(startLocalMs) {}

void Simulation::advanceTo(int64_t localNowMs) { advanceBy(tracker_.elapsedSince(localNowMs)); }

void Simulation::advanceBy(int64_t ms) {
  if (ms <= 0) return;
  carryMs_ += ms;
  while (carryMs_ >= kStepMs) {
    step(kStepMs);
    carryMs_ -= kStepMs;
  }
}

Mode Simulation::mode() const {
  const Activity activity = schedule_.activityAt(simTimeMs_);
  if (pet_.sleeping || activity == Activity::Asleep) return Mode::Asleep;
  if (pet_.faceDown) return Mode::Napping;
  if (activity == Activity::AtSchool) return Mode::Away;
  return Mode::Around;
}

void Simulation::step(int64_t ms) {
  const float hours = static_cast<float>(ms) / static_cast<float>(timeutil::kMsPerHour);
  const Mode m = mode();
  Needs& n = pet_.needs;

  float drain = 1.0f;
  switch (m) {
    case Mode::Around: drain = 1.0f; break;
    case Mode::Away: drain = kAwayMultiplier; break;
    case Mode::Napping: drain = kNapMultiplier; break;
    case Mode::Asleep: drain = kSleepMultiplier; break;
  }

  n[Need::Hunger] -= kHungerDrainPerHour * drain * hours;
  n[Need::Affection] -= kAffectionDrainPerHour * drain * hours;
  n[Need::Hygiene] -= kHygieneDrainPerHour * drain * hours;
  if (m == Mode::Asleep) {
    n[Need::Energy] += kEnergyRegenAsleepPerHour * hours;
  } else if (m == Mode::Napping) {
    n[Need::Energy] += kEnergyRegenNapPerHour * hours;
  } else {
    n[Need::Energy] -= kEnergyDrainPerHour * drain * hours;
  }
  n.clampAll();

  pet_.plumpness -= kPlumpnessLossPerHour * hours;
  if (pet_.plumpness < 0.0f) pet_.plumpness = 0.0f;

  int critical = 0;
  for (float level : n.level) {
    if (level < kCriticalLevel) ++critical;
  }
  if (critical > 0) {
    pet_.health -= kHealthLossPerCriticalNeedPerHour * static_cast<float>(critical) * hours;
  } else {
    pet_.health += kHealthRecoveryPerHour * hours;
  }
  if (pet_.health < 0.0f) pet_.health = 0.0f;
  if (pet_.health > kLevelMax) pet_.health = kLevelMax;

  simTimeMs_ += ms;

  if (pet_.sleeping && n[Need::Energy] >= kAutoWakeEnergy &&
      schedule_.activityAt(simTimeMs_) != Activity::Asleep) {
    pet_.sleeping = false;
  }
}

void Simulation::feed() {
  float& hunger = pet_.needs[Need::Hunger];
  hunger += kFeedAmount;
  if (hunger > kLevelMax) {
    pet_.plumpness += (hunger - kLevelMax) * kPlumpnessPerOverflowPoint;
    if (pet_.plumpness > 1.0f) pet_.plumpness = 1.0f;
    hunger = kLevelMax;
  }
}

void Simulation::stroke() {
  pet_.needs[Need::Affection] += kStrokeAmount;
  pet_.needs.clampAll();
}

void Simulation::putToBed() { pet_.sleeping = true; }
void Simulation::wake() { pet_.sleeping = false; }
void Simulation::bathe() { pet_.needs[Need::Hygiene] = kLevelMax; }
void Simulation::setFaceDown(bool faceDown) { pet_.faceDown = faceDown; }

}  // namespace petcore
