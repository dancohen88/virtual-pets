// Incubation, the first crack, hatch day, attention minutes and hatching, plus
// deterministic catch-up across all of it.
#include <unity.h>

#include <cstring>

#include "petcore/Life.h"
#include "petcore/TimeUtil.h"

using namespace petcore;
using timeutil::kMsPerHour;
using timeutil::kMsPerMinute;
using timeutil::kMsPerSecond;
using timeutil::localMsFromCivil;

void setUp() {}
void tearDown() {}

namespace {
// Monday 21:30, bedtime: warmth drains slowly enough to stay warm all night.
const int64_t kMondayNight = localMsFromCivil(2026, 11, 23, 21, 30);
// Saturday 08:30, weekend wake-up.
const int64_t kSaturdayWake = localMsFromCivil(2026, 11, 28, 8, 30);

// Move game time forward to a wall-clock moment. (advanceTo() measures from the last clock
// reading, so tests that also use advanceBy() go through this instead.)
void advanceUntil(Life& life, int64_t localMs) { life.advanceBy(localMs - life.localTimeMs()); }

// A life whose egg cracked overnight and whose hatch day has just opened (Tue 07:00).
Life lifeOnHatchDay() {
  Life life(Schedule{}, kMondayNight);
  advanceUntil(life, localMsFromCivil(2026, 11, 24, 7, 0, 1));
  return life;
}

// Keep handling the egg: a stroke every 5 s for `seconds` seconds.
void playWith(Life& life, int seconds) {
  for (int s = 0; s < seconds; s += 5) {
    life.strokeEgg();
    life.advanceBy(5 * kMsPerSecond);
  }
}

bool sameEgg(const EggState& a, const EggState& b) {
  return std::memcmp(&a.warmth, &b.warmth, sizeof(float)) == 0 && a.warmMs == b.warmMs &&
         a.cracks == b.cracks && a.crackedAtMs == b.crackedAtMs &&
         a.hatchDayOpensMs == b.hatchDayOpensMs && a.attentionMs == b.attentionMs;
}

bool samePet(const PetState& a, const PetState& b) {
  return std::memcmp(a.needs.level.data(), b.needs.level.data(), sizeof(float) * kNeedCount) ==
             0 &&
         a.health == b.health && a.plumpness == b.plumpness;
}
}  // namespace

void test_eight_warm_hours_crack_the_egg() {
  Life life(Schedule{}, kMondayNight);
  life.advanceBy(8 * kMsPerHour - kMsPerSecond);
  TEST_ASSERT_EQUAL_INT(0, life.egg().state().cracks);
  life.advanceBy(kMsPerSecond);
  TEST_ASSERT_EQUAL_INT(1, life.egg().state().cracks);
  TEST_ASSERT_EQUAL_INT64(kMondayNight + 8 * kMsPerHour, life.egg().state().crackedAtMs);
  TEST_ASSERT_TRUE(life.egg().phase() == EggPhase::Cracked);
}

void test_a_cold_egg_never_cracks() {
  Life life(Schedule{}, kSaturdayWake);  // around all day, nobody warms it
  life.advanceBy(24 * kMsPerHour);
  TEST_ASSERT_EQUAL_INT(0, life.egg().state().cracks);
  TEST_ASSERT_TRUE(life.egg().state().warmMs < tuning::kEggWarmMsToCrack);
}

void test_a_crack_in_the_small_hours_opens_hatch_day_that_morning() {
  Life life(Schedule{}, kMondayNight);
  life.advanceBy(8 * kMsPerHour);  // cracks Tue 05:30
  TEST_ASSERT_EQUAL_INT64(localMsFromCivil(2026, 11, 24, 7, 0),
                          life.egg().state().hatchDayOpensMs);
  advanceUntil(life, localMsFromCivil(2026, 11, 24, 6, 59));
  TEST_ASSERT_TRUE(life.egg().phase() == EggPhase::Cracked);
  advanceUntil(life, localMsFromCivil(2026, 11, 24, 7, 0, 1));
  TEST_ASSERT_TRUE(life.egg().phase() == EggPhase::HatchDay);
}

void test_a_crack_in_the_afternoon_waits_for_the_next_morning() {
  Life life(Schedule{}, kSaturdayWake);
  // Keep it warm through the day: 5 strokes every half hour.
  for (int i = 0; i < 16; ++i) {
    for (int k = 0; k < 5; ++k) life.strokeEgg();
    life.advanceBy(30 * kMsPerMinute);
  }
  TEST_ASSERT_EQUAL_INT(1, life.egg().state().cracks);
  TEST_ASSERT_EQUAL_INT64(kSaturdayWake + 8 * kMsPerHour, life.egg().state().crackedAtMs);
  // Saturday 16:30 crack -> Sunday's (weekend) wake-up at 08:30.
  TEST_ASSERT_EQUAL_INT64(localMsFromCivil(2026, 11, 29, 8, 30),
                          life.egg().state().hatchDayOpensMs);
  advanceUntil(life, localMsFromCivil(2026, 11, 29, 8, 29));
  TEST_ASSERT_TRUE(life.egg().phase() == EggPhase::Cracked);
}

void test_an_idle_screen_earns_no_attention() {
  Life life = lifeOnHatchDay();
  life.advanceBy(20 * kMsPerMinute);
  TEST_ASSERT_EQUAL_INT64(0, life.egg().state().attentionMs);
  TEST_ASSERT_TRUE(life.stage() == LifeStage::Egg);
}

void test_one_touch_earns_ten_seconds_of_attention() {
  Life life = lifeOnHatchDay();
  life.strokeEgg();
  life.advanceBy(30 * kMsPerSecond);
  TEST_ASSERT_EQUAL_INT64(tuning::kAttentionWindowMs, life.egg().state().attentionMs);
}

void test_attention_does_not_count_before_hatch_day() {
  Life life(Schedule{}, kMondayNight);
  life.advanceBy(8 * kMsPerHour);  // cracked at 05:30, hatch day opens at 07:00
  playWith(life, 120);
  TEST_ASSERT_EQUAL_INT64(0, life.egg().state().attentionMs);
}

void test_attention_does_not_count_with_the_screen_off() {
  Life life = lifeOnHatchDay();
  life.setScreenOn(false);
  playWith(life, 60);
  TEST_ASSERT_EQUAL_INT64(0, life.egg().state().attentionMs);
}

void test_cracks_at_five_and_ten_minutes_then_hatches_at_fifteen() {
  Life life = lifeOnHatchDay();
  playWith(life, 5 * 60);
  TEST_ASSERT_EQUAL_INT(2, life.egg().state().cracks);
  playWith(life, 5 * 60);
  TEST_ASSERT_EQUAL_INT(3, life.egg().state().cracks);
  TEST_ASSERT_TRUE(life.stage() == LifeStage::Egg);
  playWith(life, 5 * 60);
  TEST_ASSERT_TRUE(life.stage() == LifeStage::Hatching);
  life.advanceBy(tuning::kHatchingMs);
  TEST_ASSERT_TRUE(life.stage() == LifeStage::Baby);
}

void test_the_babys_needs_start_full_at_hatching_and_then_drain() {
  Life life = lifeOnHatchDay();
  playWith(life, 15 * 60);
  life.advanceBy(tuning::kHatchingMs);
  TEST_ASSERT_TRUE(life.stage() == LifeStage::Baby);
  for (float level : life.pet().state().needs.level) TEST_ASSERT_EQUAL_FLOAT(100.0f, level);
  life.advanceBy(kMsPerHour);
  TEST_ASSERT_TRUE(life.pet().state().needs[Need::Hunger] < 100.0f);
}

void test_incubation_catch_up_is_split_invariant() {
  Life whole(Schedule{}, kMondayNight);
  Life chunked(Schedule{}, kMondayNight);
  const int64_t chunks[] = {1, 999, 37 * kMsPerSecond + 3, 3 * kMsPerHour, 42,
                            5 * kMsPerHour + 17, 26 * kMsPerHour};
  int64_t total = 0;
  for (int64_t c : chunks) {
    chunked.advanceBy(c);
    total += c;
  }
  whole.advanceBy(total);
  TEST_ASSERT_TRUE(sameEgg(whole.egg().state(), chunked.egg().state()));
  TEST_ASSERT_EQUAL_INT64(whole.localTimeMs(), chunked.localTimeMs());
}

void test_catch_up_across_the_hatch_is_split_invariant() {
  Life whole = lifeOnHatchDay();
  Life chunked = lifeOnHatchDay();
  // The same play session, then 30 h off; one advances in whole steps, one in odd chunks.
  for (int s = 0; s < 15 * 60; s += 5) {
    whole.strokeEgg();
    chunked.strokeEgg();
    whole.advanceBy(5 * kMsPerSecond);
    chunked.advanceBy(2 * kMsPerSecond);
    chunked.advanceBy(3 * kMsPerSecond);
  }
  whole.advanceBy(30 * kMsPerHour);
  chunked.advanceBy(7 * kMsPerHour + 333);
  chunked.advanceBy(23 * kMsPerHour - 333);
  TEST_ASSERT_TRUE(whole.stage() == LifeStage::Baby);
  TEST_ASSERT_TRUE(chunked.stage() == LifeStage::Baby);
  TEST_ASSERT_TRUE(samePet(whole.pet().state(), chunked.pet().state()));
  TEST_ASSERT_EQUAL_INT64(whole.localTimeMs(), chunked.localTimeMs());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_eight_warm_hours_crack_the_egg);
  RUN_TEST(test_a_cold_egg_never_cracks);
  RUN_TEST(test_a_crack_in_the_small_hours_opens_hatch_day_that_morning);
  RUN_TEST(test_a_crack_in_the_afternoon_waits_for_the_next_morning);
  RUN_TEST(test_an_idle_screen_earns_no_attention);
  RUN_TEST(test_one_touch_earns_ten_seconds_of_attention);
  RUN_TEST(test_attention_does_not_count_before_hatch_day);
  RUN_TEST(test_attention_does_not_count_with_the_screen_off);
  RUN_TEST(test_cracks_at_five_and_ten_minutes_then_hatches_at_fifteen);
  RUN_TEST(test_the_babys_needs_start_full_at_hatching_and_then_drain);
  RUN_TEST(test_incubation_catch_up_is_split_invariant);
  RUN_TEST(test_catch_up_across_the_hatch_is_split_invariant);
  return UNITY_END();
}
