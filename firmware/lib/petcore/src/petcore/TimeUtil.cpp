#include "petcore/TimeUtil.h"

namespace petcore::timeutil {

namespace {
// Division that rounds toward negative infinity (C++ rounds toward zero).
int64_t floorDiv(int64_t a, int64_t b) {
  int64_t q = a / b;
  if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
  return q;
}
}  // namespace

int64_t daysFromCivil(int year, unsigned month, unsigned day) {
  const int64_t y = static_cast<int64_t>(year) - (month <= 2 ? 1 : 0);
  const int64_t era = floorDiv(y, 400);
  const int64_t yoe = y - era * 400;                                        // [0, 399]
  const int64_t mp = (static_cast<int64_t>(month) + 9) % 12;                // March = 0
  const int64_t doy = (153 * mp + 2) / 5 + static_cast<int64_t>(day) - 1;  // [0, 365]
  const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                // [0, 146096]
  return era * 146097 + doe - 719468;
}

int64_t localMsFromCivil(int year, unsigned month, unsigned day, unsigned hour, unsigned minute,
                         unsigned second) {
  return daysFromCivil(year, month, day) * kMsPerDay + hour * kMsPerHour + minute * kMsPerMinute +
         second * kMsPerSecond;
}

int weekday(int64_t localMs) {
  const int64_t days = floorDiv(localMs, kMsPerDay);
  // 1970-01-01 was a Thursday (4).
  int64_t wd = (days + 4) % 7;
  if (wd < 0) wd += 7;
  return static_cast<int>(wd);
}

int minuteOfDay(int64_t localMs) {
  const int64_t msOfDay = localMs - floorDiv(localMs, kMsPerDay) * kMsPerDay;
  return static_cast<int>(msOfDay / kMsPerMinute);
}

}  // namespace petcore::timeutil
