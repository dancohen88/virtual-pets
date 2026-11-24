// The egg's warmth and balance: decay, stroking, toppling, righting, shaking.
#include <unity.h>

#include "petcore/Life.h"
#include "petcore/TimeUtil.h"

using namespace petcore;
using timeutil::kMsPerHour;
using timeutil::localMsFromCivil;

void setUp() {}
void tearDown() {}

namespace {
// Saturday 10:00: the owner is simply around, so warmth drains at the full rate.
const int64_t kSaturdayMorning = localMsFromCivil(2026, 11, 28, 10, 0);
// Monday 10:00: the owner is at school.
const int64_t kMondaySchool = localMsFromCivil(2026, 11, 23, 10, 0);

float warmth(const Life& life) { return life.egg().state().warmth; }
}  // namespace

void test_life_starts_as_a_warm_egg() {
  Life life(Schedule{}, kSaturdayMorning);
  TEST_ASSERT_TRUE(life.stage() == LifeStage::Egg);
  TEST_ASSERT_EQUAL_FLOAT(tuning::kEggWarmthStart, warmth(life));
  TEST_ASSERT_TRUE(life.egg().phase() == EggPhase::Incubating);
}

void test_warmth_drains_at_the_tuned_rate_while_around() {
  Life life(Schedule{}, kSaturdayMorning);
  life.advanceBy(kMsPerHour);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, tuning::kEggWarmthStart - tuning::kEggWarmthDecayPerHour,
                           warmth(life));
}

void test_warmth_drains_slowly_while_the_owner_is_at_school() {
  Life life(Schedule{}, kMondaySchool);
  life.advanceBy(kMsPerHour);
  TEST_ASSERT_FLOAT_WITHIN(
      0.01f, tuning::kEggWarmthStart - tuning::kEggWarmthDecayPerHour * tuning::kAwayMultiplier,
      warmth(life));
}

void test_stroking_warms_the_egg_up_to_the_maximum() {
  Life life(Schedule{}, kSaturdayMorning);
  life.strokeEgg();
  TEST_ASSERT_EQUAL_FLOAT(tuning::kEggWarmthStart + tuning::kEggStrokeWarmth, warmth(life));
  for (int i = 0; i < 100; ++i) life.strokeEgg();
  TEST_ASSERT_EQUAL_FLOAT(tuning::kLevelMax, warmth(life));
}

void test_egg_leans_but_only_topples_past_the_threshold() {
  Life life(Schedule{}, kSaturdayMorning);
  life.tiltEgg(25.0f);
  TEST_ASSERT_FALSE(life.egg().state().toppled);
  life.tiltEgg(31.0f);
  TEST_ASSERT_TRUE(life.egg().state().toppled);
  TEST_ASSERT_EQUAL_INT(1, life.egg().state().toppleSide);
}

void test_tilting_the_other_way_rights_a_toppled_egg() {
  Life life(Schedule{}, kSaturdayMorning);
  life.tiltEgg(-40.0f);  // falls to the left
  TEST_ASSERT_EQUAL_INT(-1, life.egg().state().toppleSide);
  life.tiltEgg(0.0f);  // level again: it's still lying down
  TEST_ASSERT_TRUE(life.egg().state().toppled);
  life.tiltEgg(16.0f);  // tilted the other way: rolls back and stands up
  TEST_ASSERT_FALSE(life.egg().state().toppled);
}

void test_tapping_rights_it_unless_still_tilted_too_far() {
  Life life(Schedule{}, kSaturdayMorning);
  life.tiltEgg(40.0f);
  life.tapEgg();  // device still tilted: it would just fall again
  TEST_ASSERT_TRUE(life.egg().state().toppled);
  life.tiltEgg(5.0f);
  life.tapEgg();
  TEST_ASSERT_FALSE(life.egg().state().toppled);
}

void test_a_toppled_egg_cools_twice_as_fast() {
  Life upright(Schedule{}, kSaturdayMorning);
  Life fallen(Schedule{}, kSaturdayMorning);
  fallen.tiltEgg(45.0f);
  upright.advanceBy(kMsPerHour);
  fallen.advanceBy(kMsPerHour);
  const float uprightLoss = tuning::kEggWarmthStart - warmth(upright);
  const float fallenLoss = tuning::kEggWarmthStart - warmth(fallen);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, uprightLoss * tuning::kEggToppledCoolingFactor, fallenLoss);
}

void test_shaking_only_makes_it_wobble() {
  Life life(Schedule{}, kSaturdayMorning);
  life.shakeEgg();
  TEST_ASSERT_TRUE(life.egg().wobbling(life.localTimeMs()));
  TEST_ASSERT_EQUAL_FLOAT(tuning::kEggWarmthStart, warmth(life));
  TEST_ASSERT_FALSE(life.egg().state().toppled);
  life.advanceBy(tuning::kShakeWobbleMs);
  TEST_ASSERT_FALSE(life.egg().wobbling(life.localTimeMs()));
}

void test_warmth_never_goes_below_zero() {
  Life life(Schedule{}, kSaturdayMorning);
  life.tiltEgg(60.0f);
  life.advanceBy(48 * kMsPerHour);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, warmth(life));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_life_starts_as_a_warm_egg);
  RUN_TEST(test_warmth_drains_at_the_tuned_rate_while_around);
  RUN_TEST(test_warmth_drains_slowly_while_the_owner_is_at_school);
  RUN_TEST(test_stroking_warms_the_egg_up_to_the_maximum);
  RUN_TEST(test_egg_leans_but_only_topples_past_the_threshold);
  RUN_TEST(test_tilting_the_other_way_rights_a_toppled_egg);
  RUN_TEST(test_tapping_rights_it_unless_still_tilted_too_far);
  RUN_TEST(test_a_toppled_egg_cools_twice_as_fast);
  RUN_TEST(test_shaking_only_makes_it_wobble);
  RUN_TEST(test_warmth_never_goes_below_zero);
  return UNITY_END();
}
