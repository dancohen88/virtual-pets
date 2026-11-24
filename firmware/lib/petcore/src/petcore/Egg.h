#pragma once
// The egg: warmth, balance, incubation and the hatch-day attention counter.
// Time-driven parts advance in fixed kEggStepMs steps (see Life), so catch-up is
// deterministic; player actions apply instantly at the current time.

#include <cstdint>

#include "petcore/Schedule.h"
#include "petcore/Tuning.h"

namespace petcore {

// Where incubation is up to, in the order the player sees it.
enum class EggPhase { Incubating, Cracked, HatchDay, Hatched };

struct EggState {
  float warmth = tuning::kEggWarmthStart;  // 0..100
  int64_t warmMs = 0;                      // incubation: time spent at or above the threshold
  int cracks = 0;                          // 0 = none, 1 = first crack, 2, 3; hatched after that
  bool toppled = false;
  int toppleSide = 0;            // -1 fell to the left, +1 to the right
  float leanDeg = 0.0f;          // last tilt reading
  int64_t crackedAtMs = -1;      // when the first crack appeared
  int64_t hatchDayOpensMs = -1;  // the next morning's wake time after the crack
  int64_t attentionMs = 0;       // hatch-day attention, counted only once hatch day opens
  int64_t wobbleUntilMs = 0;     // dizzy wobble after a shake (cosmetic)
};

class Egg {
 public:
  // One fixed step covering [startMs, startMs + ms). Returns true if the egg hatched.
  bool step(int64_t startMs, int64_t ms, const Schedule& schedule, bool faceDown);

  // Player actions at time nowMs. Each counts as handling the egg (attention).
  void stroke(int64_t nowMs);
  void tap(int64_t nowMs);  // rights a toppled egg, unless the device is still tilted too far
  void shake(int64_t nowMs);
  void tilt(int64_t nowMs, float leanDeg);  // call every frame with the current lean

  // Attention only counts with the screen on; turning it off ends the current engagement.
  void setScreenOn(bool on);

  EggPhase phase() const;
  const EggState& state() const { return s_; }
  bool wobbling(int64_t nowMs) const { return nowMs < s_.wobbleUntilMs; }

 private:
  void handled(int64_t nowMs);
  float decayMultiplier(int64_t atMs, const Schedule& schedule, bool faceDown) const;

  EggState s_;
  bool screenOn_ = true;
  bool hatchDay_ = false;
  bool hatched_ = false;
  float lastHandledLean_ = 0.0f;
  // The current engagement: interactions keep extending attentionUntil_ while they are
  // less than kAttentionWindowMs apart; a longer gap starts a new engagement.
  int64_t attentionSince_ = 0;
  int64_t attentionUntil_ = 0;
};

}  // namespace petcore
