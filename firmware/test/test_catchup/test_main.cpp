// Catch-up after the device has been off must be deterministic: however the elapsed
// time is chopped up, the pet ends in exactly the same state.
#include <unity.h>

#include <cstring>

#include "petcore/Simulation.h"
#include "petcore/TimeUtil.h"

using namespace petcore;
using timeutil::kMsPerHour;
using timeutil::kMsPerMinute;

void setUp() {}
void tearDown() {}

namespace {
const int64_t kMondayMorning = timeutil::localMsFromCivil(2026, 11, 23, 6, 0);

Simulation makeSim() { return Simulation(Schedule{}, kMondayMorning); }

bool sameState(const PetState& a, const PetState& b) {
  return std::memcmp(a.needs.level.data(), b.needs.level.data(), sizeof(float) * kNeedCount) ==
             0 &&
         a.health == b.health && a.plumpness == b.plumpness && a.sleeping == b.sleeping;
}
}  // namespace

void test_one_big_jump_equals_many_small_steps() {
  Simulation big = makeSim();
  Simulation small = makeSim();
  big.advanceBy(30 * kMsPerHour);
  for (int i = 0; i < 30 * 60; ++i) small.advanceBy(kMsPerMinute);
  TEST_ASSERT_TRUE(sameState(big.state(), small.state()));
  TEST_ASSERT_EQUAL_INT64(big.localTimeMs(), small.localTimeMs());
}

void test_uneven_chunks_give_identical_results() {
  Simulation whole = makeSim();
  Simulation chunked = makeSim();
  const int64_t chunks[] = {1, 59999, 17 * kMsPerMinute + 3, 3 * kMsPerHour, 42, 26 * kMsPerHour};
  int64_t total = 0;
  for (int64_t c : chunks) {
    chunked.advanceBy(c);
    total += c;
  }
  whole.advanceBy(total);
  TEST_ASSERT_TRUE(sameState(whole.state(), chunked.state()));
  TEST_ASSERT_EQUAL_INT64(whole.localTimeMs(), chunked.localTimeMs());
}

void test_partial_steps_carry_over() {
  Simulation a = makeSim();
  Simulation b = makeSim();
  a.advanceBy(30 * 1000);
  a.advanceBy(30 * 1000);
  b.advanceBy(60 * 1000);
  TEST_ASSERT_TRUE(sameState(a.state(), b.state()));
}

void test_three_days_off_catches_up_through_the_schedule() {
  Simulation sim = makeSim();
  sim.advanceTo(kMondayMorning + 72 * kMsPerHour);
  TEST_ASSERT_EQUAL_INT64(kMondayMorning + 72 * kMsPerHour, sim.localTimeMs());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, sim.state().needs[Need::Hunger]);
}

void test_clock_going_backwards_counts_as_no_time() {
  Simulation sim = makeSim();
  sim.advanceTo(kMondayMorning + kMsPerHour);
  const PetState before = sim.state();
  const int64_t timeBefore = sim.localTimeMs();
  sim.advanceTo(kMondayMorning - 5 * kMsPerHour);  // RTC reset to an earlier time
  TEST_ASSERT_TRUE(sameState(before, sim.state()));
  TEST_ASSERT_EQUAL_INT64(timeBefore, sim.localTimeMs());
  sim.advanceTo(kMondayMorning - 4 * kMsPerHour);  // then runs forward again: +1 h counts
  TEST_ASSERT_EQUAL_INT64(timeBefore + kMsPerHour, sim.localTimeMs());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_one_big_jump_equals_many_small_steps);
  RUN_TEST(test_uneven_chunks_give_identical_results);
  RUN_TEST(test_partial_steps_carry_over);
  RUN_TEST(test_three_days_off_catches_up_through_the_schedule);
  RUN_TEST(test_clock_going_backwards_counts_as_no_time);
  return UNITY_END();
}
