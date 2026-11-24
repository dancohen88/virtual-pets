// Monotonic elapsed time and the debug accelerated clock.
#include <unity.h>

#include "petcore/Clock.h"

using namespace petcore;

void setUp() {}
void tearDown() {}

namespace {
class FakeClock : public Clock {
 public:
  int64_t now = 1'000'000;
  int64_t nowMs() override { return now; }
};
}  // namespace

void test_elapsed_tracker_ignores_backwards_jumps() {
  ElapsedTracker t(1000);
  TEST_ASSERT_EQUAL_INT64(500, t.elapsedSince(1500));
  TEST_ASSERT_EQUAL_INT64(0, t.elapsedSince(200));  // backwards
  TEST_ASSERT_EQUAL_INT64(100, t.elapsedSince(300));  // counts from the new baseline
}

void test_accelerated_clock_runs_faster() {
  FakeClock base;
  AcceleratedClock fast(base);
  const int64_t start = fast.nowMs();
  fast.setSpeed(60.0);
  base.now += 1000;  // one real second
  TEST_ASSERT_EQUAL_INT64(start + 60'000, fast.nowMs());
}

void test_changing_speed_does_not_jump_time() {
  FakeClock base;
  AcceleratedClock clock(base);
  clock.setSpeed(10.0);
  base.now += 1000;
  const int64_t before = clock.nowMs();
  clock.setSpeed(3600.0);
  TEST_ASSERT_EQUAL_INT64(before, clock.nowMs());
}

void test_speed_zero_pauses() {
  FakeClock base;
  AcceleratedClock clock(base);
  clock.setSpeed(0.0);
  const int64_t paused = clock.nowMs();
  base.now += 50'000;
  TEST_ASSERT_EQUAL_INT64(paused, clock.nowMs());
}

void test_jump_skips_ahead() {
  FakeClock base;
  AcceleratedClock clock(base);
  const int64_t start = clock.nowMs();
  clock.jump(3'600'000);
  TEST_ASSERT_EQUAL_INT64(start + 3'600'000, clock.nowMs());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_elapsed_tracker_ignores_backwards_jumps);
  RUN_TEST(test_accelerated_clock_runs_faster);
  RUN_TEST(test_changing_speed_does_not_jump_time);
  RUN_TEST(test_speed_zero_pauses);
  RUN_TEST(test_jump_skips_ahead);
  return UNITY_END();
}
