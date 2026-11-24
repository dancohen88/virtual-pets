// Calendar maths, the parent-set schedule, and asymmetric decay.
#include <unity.h>

#include "petcore/Schedule.h"
#include "petcore/Simulation.h"
#include "petcore/TimeUtil.h"

using namespace petcore;
using timeutil::kMsPerHour;
using timeutil::localMsFromCivil;

void setUp() {}
void tearDown() {}

void test_weekday_of_known_dates() {
  TEST_ASSERT_EQUAL_INT(4, timeutil::weekday(localMsFromCivil(1970, 1, 1)));   // Thursday
  TEST_ASSERT_EQUAL_INT(6, timeutil::weekday(localMsFromCivil(2000, 1, 1)));   // Saturday
  TEST_ASSERT_EQUAL_INT(4, timeutil::weekday(localMsFromCivil(2024, 2, 29)));  // Thursday
  TEST_ASSERT_EQUAL_INT(1, timeutil::weekday(localMsFromCivil(2026, 11, 23)));  // Monday
  TEST_ASSERT_EQUAL_INT(5, timeutil::weekday(localMsFromCivil(2026, 12, 25)));  // Friday
}

void test_minute_of_day() {
  TEST_ASSERT_EQUAL_INT(8 * 60 + 31, timeutil::minuteOfDay(localMsFromCivil(2026, 11, 23, 8, 31)));
}

void test_school_hours_on_a_weekday() {
  Schedule s;
  TEST_ASSERT_TRUE(s.activityAt(localMsFromCivil(2026, 11, 23, 10, 0)) == Activity::AtSchool);
  TEST_ASSERT_TRUE(s.activityAt(localMsFromCivil(2026, 11, 23, 16, 0)) == Activity::Around);
}

void test_no_school_at_weekends_or_on_holiday() {
  Schedule s;
  TEST_ASSERT_TRUE(s.activityAt(localMsFromCivil(2026, 11, 28, 10, 0)) == Activity::Around);
  s.holiday = true;
  TEST_ASSERT_TRUE(s.activityAt(localMsFromCivil(2026, 11, 23, 10, 0)) == Activity::Around);
}

void test_bedtime_crosses_midnight() {
  Schedule s;  // weekday bedtime 21:00, wake 07:00
  TEST_ASSERT_TRUE(s.activityAt(localMsFromCivil(2026, 11, 23, 22, 0)) == Activity::Asleep);
  TEST_ASSERT_TRUE(s.activityAt(localMsFromCivil(2026, 11, 24, 3, 0)) == Activity::Asleep);
  TEST_ASSERT_TRUE(s.activityAt(localMsFromCivil(2026, 11, 24, 7, 30)) == Activity::Around);
}

void test_sunday_night_is_a_school_night_friday_is_not() {
  Schedule s;  // weekday bedtime 21:00, weekend bedtime 22:00
  TEST_ASSERT_TRUE(s.activityAt(localMsFromCivil(2026, 11, 29, 21, 30)) == Activity::Asleep);  // Sun
  TEST_ASSERT_TRUE(s.activityAt(localMsFromCivil(2026, 11, 27, 21, 30)) == Activity::Around);  // Fri
}

void test_needs_drain_slower_while_owner_is_at_school() {
  Schedule s;
  Simulation home(s, localMsFromCivil(2026, 11, 28, 10, 0));    // Saturday: around
  Simulation school(s, localMsFromCivil(2026, 11, 23, 10, 0));  // Monday: at school
  home.advanceBy(kMsPerHour);
  school.advanceBy(kMsPerHour);
  TEST_ASSERT_TRUE(school.mode() == Mode::Away);
  const float homeDrop = 100.0f - home.state().needs[Need::Hunger];
  const float schoolDrop = 100.0f - school.state().needs[Need::Hunger];
  TEST_ASSERT_FLOAT_WITHIN(0.01f, homeDrop * tuning::kAwayMultiplier, schoolDrop);
}

void test_face_down_means_a_nap() {
  Schedule s;
  s.holiday = true;
  Simulation sim(s, localMsFromCivil(2026, 11, 28, 10, 0));
  sim.advanceBy(3 * kMsPerHour);
  const float energy = sim.state().needs[Need::Energy];
  sim.setFaceDown(true);
  TEST_ASSERT_TRUE(sim.mode() == Mode::Napping);
  sim.advanceBy(kMsPerHour);
  TEST_ASSERT_TRUE(sim.state().needs[Need::Energy] > energy);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_weekday_of_known_dates);
  RUN_TEST(test_minute_of_day);
  RUN_TEST(test_school_hours_on_a_weekday);
  RUN_TEST(test_no_school_at_weekends_or_on_holiday);
  RUN_TEST(test_bedtime_crosses_midnight);
  RUN_TEST(test_sunday_night_is_a_school_night_friday_is_not);
  RUN_TEST(test_needs_drain_slower_while_owner_is_at_school);
  RUN_TEST(test_face_down_means_a_nap);
  return UNITY_END();
}
