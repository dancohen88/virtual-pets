#include "petcore/Schedule.h"

#include "petcore/TimeUtil.h"

namespace petcore {

const DayProfile& Schedule::profileFor(int weekdayIndex) const {
  const bool isWeekend = (weekdayIndex == 0 || weekdayIndex == 6);
  return isWeekend ? weekend : weekday;
}

Activity Schedule::activityAt(int64_t localMs) const {
  const int wd = timeutil::weekday(localMs);
  const int minute = timeutil::minuteOfDay(localMs);
  const DayProfile& today = profileFor(wd);
  // An evening's bedtime belongs to the *next* day: Sunday night is a school night,
  // Friday night is not.
  const DayProfile& tomorrow = profileFor((wd + 1) % 7);

  if (minute < today.wakeMin || minute >= tomorrow.bedtimeMin) return Activity::Asleep;
  if (today.school && !holiday && minute >= today.schoolStartMin && minute < today.schoolEndMin) {
    return Activity::AtSchool;
  }
  return Activity::Around;
}

}  // namespace petcore
