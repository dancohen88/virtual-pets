#pragma once
// The pet's whole life on one time line: Egg -> Hatching -> Baby.
// Owns the clock bookkeeping, so catch-up across the hatch is still deterministic:
// the egg advances in fixed kEggStepMs steps, and once the baby exists the needs
// simulation takes over with its own fixed steps and any leftover time.

#include <cstdint>
#include <optional>

#include "petcore/Clock.h"
#include "petcore/Egg.h"
#include "petcore/Schedule.h"
#include "petcore/Simulation.h"

namespace petcore {

enum class LifeStage { Egg, Hatching, Baby };

class Life {
 public:
  Life(const Schedule& schedule, int64_t startLocalMs);

  // Advance to a clock reading (a reading earlier than the last counts as no time).
  void advanceTo(int64_t localNowMs);
  // Advance by game time. advanceBy(a); advanceBy(b) == advanceBy(a + b).
  void advanceBy(int64_t ms);

  LifeStage stage() const { return stage_; }
  int64_t localTimeMs() const;
  const Schedule& schedule() const { return schedule_; }
  const Egg& egg() const { return egg_; }
  int64_t hatchingStartedMs() const { return hatchingStartMs_; }

  // The baby's needs simulation; only valid once stage() == Baby.
  Simulation& pet() { return *pet_; }
  const Simulation& pet() const { return *pet_; }

  // Egg actions, at the current game time (ignored after hatching).
  void strokeEgg();
  void tapEgg();
  void shakeEgg();
  void tiltEgg(float leanDeg);

  void setFaceDown(bool faceDown);
  void setScreenOn(bool on);

 private:
  Schedule schedule_;
  Egg egg_;
  std::optional<Simulation> pet_;
  LifeStage stage_ = LifeStage::Egg;
  int64_t timeMs_;      // start of the next egg step
  int64_t carryMs_ = 0;  // game time not yet stepped
  int64_t hatchingStartMs_ = -1;
  bool faceDown_ = false;
  ElapsedTracker tracker_;
};

}  // namespace petcore
