#pragma once
// Every gameplay number lives here, so balancing never means hunting through logic.
// Units: need levels are 0..100 (100 = fully met); rates are per hour of game time.

#include <cstdint>

namespace petcore::tuning {

constexpr float kLevelMax = 100.0f;

// --- Drain while the pet is "around" (awake and the owner is nearby) ---
constexpr float kHungerDrainPerHour = 10.0f;    // full -> starving in ~10 h
constexpr float kAffectionDrainPerHour = 8.0f;
constexpr float kEnergyDrainPerHour = 6.0f;
constexpr float kHygieneDrainPerHour = 4.0f;

// --- Asymmetric decay: needs fall much more slowly when the owner can't help ---
constexpr float kAwayMultiplier = 0.25f;   // owner at school
constexpr float kSleepMultiplier = 0.15f;  // night-time / put to bed
constexpr float kNapMultiplier = 0.30f;    // device left face-down

// --- Rest ---
constexpr float kEnergyRegenAsleepPerHour = 14.0f;
constexpr float kEnergyRegenNapPerHour = 6.0f;
constexpr float kAutoWakeEnergy = 95.0f;  // a pet put to bed wakes once rested (outside bedtime)

// --- Hidden health: derived from how long needs stay critical ---
constexpr float kCriticalLevel = 20.0f;
constexpr float kHealthLossPerCriticalNeedPerHour = 4.0f;
constexpr float kHealthRecoveryPerHour = 2.0f;

// --- Actions ---
constexpr float kFeedAmount = 35.0f;
constexpr float kStrokeAmount = 8.0f;
// Food beyond "full" turns into plumpness (0..1), which slowly wears off.
constexpr float kPlumpnessPerOverflowPoint = 0.004f;  // 25 points over = +0.1
constexpr float kPlumpnessLossPerHour = 0.005f;

// --- Simulation step ---
// Catch-up runs in fixed chunks of this size, so 3 days off and back on gives exactly
// the same result as 3 days of continuous play (deterministic fixed-step simulation).
constexpr int64_t kStepMs = 60 * 1000;

// --- Egg (Stage 2) ---
// Warmth is 0..100 like the needs. It drains with the same asymmetric multipliers
// (school, night, face-down), so being away doesn't freeze the egg.
constexpr float kEggWarmthStart = 80.0f;          // a new egg starts comfortably warm
constexpr float kEggWarmthDecayPerHour = 12.0f;   // with the owner around: 80 -> 50 in 2.5 h
constexpr float kEggToppledCoolingFactor = 2.0f;  // lying on its side it cools twice as fast
constexpr float kEggStrokeWarmth = 2.0f;          // per stroke event (~5 a second while rubbing)
constexpr float kEggWarmThreshold = 50.0f;        // incubation only counts while this warm
constexpr int64_t kEggWarmMsToCrack = 8LL * 60 * 60 * 1000;  // 8 h of warm time -> first crack
// The egg steps once a second (not once a minute like the pet), so a crack or the hatch
// lands while someone is still there to see it.
constexpr int64_t kEggStepMs = 1000;

// Balance. Lean is the angle of gravity away from the screen's vertical axis.
constexpr float kEggToppleDeg = 30.0f;      // lean further than this and it falls over
constexpr float kEggRightingDeg = 15.0f;    // tilt this far the other way and it rolls back up
constexpr float kEggHandledTiltDeg = 5.0f;  // a tilt change this big counts as handling it
constexpr int64_t kShakeWobbleMs = 2000;    // a shake makes it wobble dizzily, nothing more

// Hatch day: attention minutes = time with the screen on and the egg touched in the last
// kAttentionWindowMs. Cracks at 5 and 10, hatch at 15, so it hatches in front of the player.
constexpr int64_t kAttentionWindowMs = 10 * 1000;
constexpr int64_t kAttentionSecondCrackMs = 5LL * 60 * 1000;
constexpr int64_t kAttentionThirdCrackMs = 10LL * 60 * 1000;
constexpr int64_t kAttentionHatchMs = 15LL * 60 * 1000;
constexpr int64_t kHatchingMs = 5000;  // the hatching moment, before the baby's needs start

}  // namespace petcore::tuning
