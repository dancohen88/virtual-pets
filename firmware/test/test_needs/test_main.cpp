// Needs, actions and hidden health.
#include <unity.h>

#include "petcore/Simulation.h"
#include "petcore/TimeUtil.h"

using namespace petcore;
using timeutil::kMsPerHour;

void setUp() {}
void tearDown() {}

namespace {
// A Saturday at 10:00 with a holiday schedule: the pet is simply "around".
Simulation aroundSim() {
  Schedule s;
  s.holiday = true;
  return Simulation(s, timeutil::localMsFromCivil(2026, 11, 28, 10, 0));
}
}  // namespace

void test_needs_start_full() {
  Simulation sim = aroundSim();
  for (float level : sim.state().needs.level) TEST_ASSERT_EQUAL_FLOAT(100.0f, level);
  TEST_ASSERT_EQUAL_FLOAT(100.0f, sim.state().health);
}

void test_needs_drain_at_tuned_rates_while_around() {
  Simulation sim = aroundSim();
  sim.advanceBy(kMsPerHour);
  TEST_ASSERT_TRUE(sim.mode() == Mode::Around);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f - tuning::kHungerDrainPerHour,
                           sim.state().needs[Need::Hunger]);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f - tuning::kHygieneDrainPerHour,
                           sim.state().needs[Need::Hygiene]);
}

void test_needs_never_go_below_zero() {
  Simulation sim = aroundSim();
  sim.advanceBy(48 * kMsPerHour);
  for (float level : sim.state().needs.level) TEST_ASSERT_TRUE(level >= 0.0f);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, sim.state().needs[Need::Hunger]);
}

void test_feeding_past_full_makes_the_pet_plump() {
  Simulation sim = aroundSim();
  sim.feed();  // already full: all 35 points overflow
  TEST_ASSERT_EQUAL_FLOAT(100.0f, sim.state().needs[Need::Hunger]);
  TEST_ASSERT_FLOAT_WITHIN(1e-5f, 35.0f * tuning::kPlumpnessPerOverflowPoint,
                           sim.state().plumpness);
}

void test_plumpness_wears_off() {
  Simulation sim = aroundSim();
  sim.feed();
  const float before = sim.state().plumpness;
  sim.advanceBy(10 * kMsPerHour);
  TEST_ASSERT_TRUE(sim.state().plumpness < before);
}

void test_bath_restores_hygiene() {
  Simulation sim = aroundSim();
  sim.advanceBy(5 * kMsPerHour);
  TEST_ASSERT_TRUE(sim.state().needs[Need::Hygiene] < 100.0f);
  sim.bathe();
  TEST_ASSERT_EQUAL_FLOAT(100.0f, sim.state().needs[Need::Hygiene]);
}

void test_health_falls_when_needs_are_critical_and_recovers_later() {
  Simulation sim = aroundSim();
  sim.advanceBy(24 * kMsPerHour);  // hunger and affection hit zero
  const float sick = sim.state().health;
  TEST_ASSERT_TRUE(sick < 100.0f);
  for (int i = 0; i < 4; ++i) sim.feed();
  for (int i = 0; i < 13; ++i) sim.stroke();
  sim.bathe();
  sim.putToBed();
  sim.advanceBy(6 * kMsPerHour);
  TEST_ASSERT_TRUE(sim.state().health > sick);
}

void test_sleeping_restores_energy() {
  Simulation sim = aroundSim();
  sim.advanceBy(5 * kMsPerHour);
  const float tired = sim.state().needs[Need::Energy];
  sim.putToBed();
  TEST_ASSERT_TRUE(sim.mode() == Mode::Asleep);
  sim.advanceBy(kMsPerHour);
  TEST_ASSERT_TRUE(sim.state().needs[Need::Energy] > tired);
}

void test_pet_put_to_bed_wakes_when_rested() {
  Simulation sim = aroundSim();
  sim.advanceBy(2 * kMsPerHour);
  sim.putToBed();
  sim.advanceBy(3 * kMsPerHour);
  TEST_ASSERT_FALSE(sim.state().sleeping);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_needs_start_full);
  RUN_TEST(test_needs_drain_at_tuned_rates_while_around);
  RUN_TEST(test_needs_never_go_below_zero);
  RUN_TEST(test_feeding_past_full_makes_the_pet_plump);
  RUN_TEST(test_plumpness_wears_off);
  RUN_TEST(test_bath_restores_hygiene);
  RUN_TEST(test_health_falls_when_needs_are_critical_and_recovers_later);
  RUN_TEST(test_sleeping_restores_energy);
  RUN_TEST(test_pet_put_to_bed_wakes_when_rested);
  return UNITY_END();
}
