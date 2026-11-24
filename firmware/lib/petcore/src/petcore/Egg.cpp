#include "petcore/Egg.h"

#include <algorithm>
#include <cmath>

#include "petcore/TimeUtil.h"

namespace petcore {

using namespace tuning;

float Egg::decayMultiplier(int64_t atMs, const Schedule& schedule, bool faceDown) const {
  // Same asymmetric decay as the pet's needs: slower when the owner can't help.
  switch (schedule.activityAt(atMs)) {
    case Activity::Asleep: return kSleepMultiplier;
    case Activity::AtSchool: return faceDown ? std::min(kAwayMultiplier, kNapMultiplier)
                                             : kAwayMultiplier;
    case Activity::Around: return faceDown ? kNapMultiplier : 1.0f;
  }
  return 1.0f;
}

bool Egg::step(int64_t startMs, int64_t ms, const Schedule& schedule, bool faceDown) {
  if (hatched_) return false;
  const int64_t endMs = startMs + ms;
  const float hours = static_cast<float>(ms) / static_cast<float>(timeutil::kMsPerHour);

  // Incubation counts while warm enough at the start of the step.
  if (s_.cracks == 0 && s_.warmth >= kEggWarmThreshold) {
    s_.warmMs += ms;
    if (s_.warmMs >= kEggWarmMsToCrack) {
      s_.cracks = 1;
      s_.crackedAtMs = endMs;
      s_.hatchDayOpensMs = schedule.nextWakeAfter(endMs);
    }
  }

  float decay = kEggWarmthDecayPerHour * decayMultiplier(startMs, schedule, faceDown) * hours;
  if (s_.toppled) decay *= kEggToppledCoolingFactor;
  s_.warmth = std::max(0.0f, s_.warmth - decay);

  if (s_.cracks > 0 && !hatchDay_ && startMs >= s_.hatchDayOpensMs) hatchDay_ = true;

  // Attention: the part of this step inside the current engagement, after hatch day opens.
  if (hatchDay_ && screenOn_) {
    const int64_t lo = std::max({startMs, attentionSince_, s_.hatchDayOpensMs});
    const int64_t hi = std::min(endMs, attentionUntil_);
    if (hi > lo) s_.attentionMs += hi - lo;
    if (s_.attentionMs >= kAttentionThirdCrackMs) {
      s_.cracks = std::max(s_.cracks, 3);
    } else if (s_.attentionMs >= kAttentionSecondCrackMs) {
      s_.cracks = std::max(s_.cracks, 2);
    }
    if (s_.attentionMs >= kAttentionHatchMs) {
      hatched_ = true;
      return true;
    }
  }
  return false;
}

void Egg::handled(int64_t nowMs) {
  if (!screenOn_) return;
  if (nowMs > attentionUntil_) attentionSince_ = nowMs;  // a new engagement
  attentionUntil_ = std::max(attentionUntil_, nowMs + kAttentionWindowMs);
}

void Egg::stroke(int64_t nowMs) {
  s_.warmth = std::min(kLevelMax, s_.warmth + kEggStrokeWarmth);
  handled(nowMs);
}

void Egg::tap(int64_t nowMs) {
  if (s_.toppled && std::fabs(s_.leanDeg) <= kEggToppleDeg) {
    s_.toppled = false;
    s_.toppleSide = 0;
  }
  handled(nowMs);
}

void Egg::shake(int64_t nowMs) {
  s_.wobbleUntilMs = nowMs + kShakeWobbleMs;
  handled(nowMs);
}

void Egg::tilt(int64_t nowMs, float leanDeg) {
  s_.leanDeg = leanDeg;
  if (!s_.toppled && std::fabs(leanDeg) > kEggToppleDeg) {
    s_.toppled = true;
    s_.toppleSide = leanDeg > 0 ? 1 : -1;
  } else if (s_.toppled && leanDeg * static_cast<float>(s_.toppleSide) <= -kEggRightingDeg) {
    // Tilted the other way: it rolls back to the middle and stands up.
    s_.toppled = false;
    s_.toppleSide = 0;
  }
  if (std::fabs(leanDeg - lastHandledLean_) >= kEggHandledTiltDeg) {
    lastHandledLean_ = leanDeg;
    handled(nowMs);
  }
}

void Egg::setScreenOn(bool on) {
  screenOn_ = on;
  if (!on) attentionUntil_ = 0;
}

EggPhase Egg::phase() const {
  if (hatched_) return EggPhase::Hatched;
  if (hatchDay_) return EggPhase::HatchDay;
  if (s_.cracks > 0) return EggPhase::Cracked;
  return EggPhase::Incubating;
}

}  // namespace petcore
