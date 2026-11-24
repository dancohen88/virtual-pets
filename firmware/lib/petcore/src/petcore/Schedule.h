#pragma once
#include <cstdint>

namespace petcore {

// One day's routine, in minutes after midnight.
struct DayProfile {
  bool school = false;
  int schoolStartMin = 8 * 60 + 30;
  int schoolEndMin = 15 * 60 + 30;
  int bedtimeMin = 21 * 60;
  int wakeMin = 7 * 60;
};

enum class Activity { Around, AtSchool, Asleep };

// The parent-set routine: weekday and weekend profiles plus a holiday switch.
struct Schedule {
  DayProfile weekday{true, 8 * 60 + 30, 15 * 60 + 30, 21 * 60, 7 * 60};
  DayProfile weekend{false, 0, 0, 22 * 60, 8 * 60 + 30};
  bool holiday = false;  // turns school hours off; bedtimes still apply

  const DayProfile& profileFor(int weekdayIndex) const;  // 0 = Sunday
  Activity activityAt(int64_t localMs) const;
  // The first wake-up time strictly after localMs (today's if still ahead, else tomorrow's).
  int64_t nextWakeAfter(int64_t localMs) const;
};

}  // namespace petcore
