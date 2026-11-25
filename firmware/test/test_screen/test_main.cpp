// Screen power-saving: dims, then turns off, after idle frames; activity wakes it, and the
// activity that wakes a dark screen is consumed rather than acted on.
#include <unity.h>

#include "petcore/ScreenPower.h"
#include "petcore/Tuning.h"

using namespace petcore;
using tuning::kScreenDimAfterFrames;
using tuning::kScreenOffAfterFrames;

void setUp() {}
void tearDown() {}

namespace {
void idle(ScreenPower& p, int frames) {
  for (int i = 0; i < frames; ++i) p.update(false);
}
}  // namespace

void test_starts_on() {
  ScreenPower p;
  TEST_ASSERT_TRUE(p.level() == ScreenLevel::On);
}

void test_dims_then_turns_off_when_idle() {
  ScreenPower p;
  idle(p, kScreenDimAfterFrames - 1);
  TEST_ASSERT_TRUE(p.level() == ScreenLevel::On);
  idle(p, 1);
  TEST_ASSERT_TRUE(p.level() == ScreenLevel::Dim);
  idle(p, kScreenOffAfterFrames - kScreenDimAfterFrames);
  TEST_ASSERT_TRUE(p.level() == ScreenLevel::Off);
}

void test_activity_resets_the_idle_count() {
  ScreenPower p;
  idle(p, kScreenDimAfterFrames - 5);
  p.update(true);
  idle(p, kScreenDimAfterFrames - 1);
  TEST_ASSERT_TRUE(p.level() == ScreenLevel::On);
}

void test_activity_while_dim_wakes_without_consuming() {
  ScreenPower p;
  idle(p, kScreenDimAfterFrames);
  TEST_ASSERT_TRUE(p.level() == ScreenLevel::Dim);
  TEST_ASSERT_FALSE(p.update(true));  // dim screen is still readable: the tap counts
  TEST_ASSERT_TRUE(p.level() == ScreenLevel::On);
}

void test_waking_from_off_consumes_the_activity() {
  ScreenPower p;
  idle(p, kScreenOffAfterFrames);
  TEST_ASSERT_TRUE(p.level() == ScreenLevel::Off);
  TEST_ASSERT_TRUE(p.update(true));   // the wake-up tap is swallowed
  TEST_ASSERT_TRUE(p.level() == ScreenLevel::On);
  TEST_ASSERT_FALSE(p.update(true));  // the next one acts normally
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_starts_on);
  RUN_TEST(test_dims_then_turns_off_when_idle);
  RUN_TEST(test_activity_resets_the_idle_count);
  RUN_TEST(test_activity_while_dim_wakes_without_consuming);
  RUN_TEST(test_waking_from_off_consumes_the_activity);
  return UNITY_END();
}
