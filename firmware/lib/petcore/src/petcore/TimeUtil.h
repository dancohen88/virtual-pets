#pragma once
// Calendar maths on "local milliseconds": ms since 1970-01-01 00:00 *local* time.
// The device's RTC is set to local time from the parent menu, so the game never needs
// time zones — a reading of 08:30 means 08:30 where the pet lives.

#include <cstdint>

namespace petcore::timeutil {

constexpr int64_t kMsPerSecond = 1000;
constexpr int64_t kMsPerMinute = 60 * kMsPerSecond;
constexpr int64_t kMsPerHour = 60 * kMsPerMinute;
constexpr int64_t kMsPerDay = 24 * kMsPerHour;

// Days since 1970-01-01 for a Gregorian date (Howard Hinnant's days_from_civil algorithm).
int64_t daysFromCivil(int year, unsigned month, unsigned day);

int64_t localMsFromCivil(int year, unsigned month, unsigned day, unsigned hour = 0,
                         unsigned minute = 0, unsigned second = 0);

// The inverse: local ms back to a calendar date and time (Hinnant's civil_from_days).
void civilFromLocalMs(int64_t localMs, int& year, int& month, int& day, int& hour, int& minute,
                      int& second);

// 0 = Sunday ... 6 = Saturday.
int weekday(int64_t localMs);

// 0..1439
int minuteOfDay(int64_t localMs);

}  // namespace petcore::timeutil
