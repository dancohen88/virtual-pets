#pragma once
#include <cstdint>

#include "petcore/Clock.h"
#include "petcore/Needs.h"
#include "petcore/Schedule.h"

namespace petcore {

// What the pet is doing right now, which decides how fast needs change.
enum class Mode { Around, Away, Napping, Asleep };

struct PetState {
  Needs needs;
  float health = tuning::kLevelMax;  // hidden; derived from how long needs stay critical
  float plumpness = 0.0f;            // 0..1, from overfeeding
  bool sleeping = false;             // put to bed by the owner
  bool faceDown = false;             // device lying face-down -> nap
};

class Simulation {
 public:
  Simulation(const Schedule& schedule, int64_t startLocalMs);

  // Advance to a clock reading. A reading earlier than the last one counts as no time.
  void advanceTo(int64_t localNowMs);
  // Advance by an amount of game time. Runs in fixed steps; leftovers carry over,
  // so advanceBy(a); advanceBy(b) gives exactly the same result as advanceBy(a + b).
  void advanceBy(int64_t ms);

  Mode mode() const;
  const PetState& state() const { return pet_; }
  int64_t localTimeMs() const { return simTimeMs_ + carryMs_; }
  Schedule& schedule() { return schedule_; }

  // Owner actions (placeholders for Stage 1).
  void feed();
  void stroke();
  void putToBed();
  void wake();
  void bathe();
  void setFaceDown(bool faceDown);

 private:
  void step(int64_t ms);

  Schedule schedule_;
  PetState pet_;
  int64_t simTimeMs_;
  int64_t carryMs_ = 0;
  ElapsedTracker tracker_;
};

}  // namespace petcore
