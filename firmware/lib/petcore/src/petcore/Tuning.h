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

}  // namespace petcore::tuning
