// M5Stack Core2 entry point. Same game core as the simulator; real RTC, IMU and touch.

#include <M5Unified.h>
#include <esp_timer.h>

#include <cmath>
#include <memory>

#include "../m5/M5Platform.h"
#include "petcore/App.h"
#include "petcore/Life.h"
#include "petcore/TimeUtil.h"

namespace {

// Local time from the battery-backed RTC, read once at boot, then advanced with the
// 64-bit microsecond timer (millis() would wrap after ~49 days).
// Build timestamp from the compiler's __DATE__ ("Nov 24 2026") and __TIME__ ("16:40:12"),
// in the build machine's local time. Used as a floor: the clock can never be earlier than
// the firmware it's running. Until Wi-Fi time sync (Stage 6) this is how a new board gets
// a roughly right time; it's as fresh as the last compile of this file.
struct BuildTime {
  int year, month, day, hour, minute, second;
};
BuildTime buildTime() {
  static const char* kMonths = "JanFebMarAprMayJunJulAugSepOctNovDec";
  const char* d = __DATE__;
  const char* t = __TIME__;
  BuildTime b{};
  for (int i = 0; i < 12; ++i) {
    if (d[0] == kMonths[i * 3] && d[1] == kMonths[i * 3 + 1] && d[2] == kMonths[i * 3 + 2]) b.month = i + 1;
  }
  b.day = (d[4] == ' ' ? 0 : d[4] - '0') * 10 + (d[5] - '0');
  b.year = (d[7] - '0') * 1000 + (d[8] - '0') * 100 + (d[9] - '0') * 10 + (d[10] - '0');
  b.hour = (t[0] - '0') * 10 + (t[1] - '0');
  b.minute = (t[3] - '0') * 10 + (t[4] - '0');
  b.second = (t[6] - '0') * 10 + (t[7] - '0');
  return b;
}

class RtcClock : public petcore::Clock {
 public:
  void begin() {
    const BuildTime b = buildTime();
    const int64_t buildMs =
        petcore::timeutil::localMsFromCivil(b.year, b.month, b.day, b.hour, b.minute, b.second);
    int64_t rtcMs = buildMs;  // if the RTC can't be read at all
    if (M5.Rtc.isEnabled()) {
      const auto dt = M5.Rtc.getDateTime();
      rtcMs = petcore::timeutil::localMsFromCivil(dt.date.year, dt.date.month, dt.date.date,
                                                  dt.time.hours, dt.time.minutes,
                                                  dt.time.seconds);
      if (rtcMs < buildMs) {  // never set, or reset by a flat backup battery
        m5::rtc_datetime_t fix;
        fix.date.year = b.year;
        fix.date.month = b.month;
        fix.date.date = b.day;
        fix.time.hours = b.hour;
        fix.time.minutes = b.minute;
        fix.time.seconds = b.second;
        M5.Rtc.setDateTime(fix);
        rtcMs = buildMs;
      }
    }
    offsetMs_ = rtcMs - esp_timer_get_time() / 1000;
  }
  int64_t nowMs() override { return offsetMs_ + esp_timer_get_time() / 1000; }

 private:
  int64_t offsetMs_ = 0;
};

// Gravity, shake and face-down from the accelerometer (in g).
// Still to confirm on real hardware: which IMU axis is the screen's right/down, and the
// face-down sign. Flip kSignX / kSignY / the az test if the egg leans the wrong way.
class ImuMotion : public petcore::Motion {
 public:
  petcore::MotionFrame read() override {
    petcore::MotionFrame m;
    float ax = 0, ay = 0, az = 0;
    if (!M5.Imu.isEnabled() || !M5.Imu.getAccel(&ax, &ay, &az)) return m;
    constexpr float kSignX = -1.0f, kSignY = -1.0f;  // X confirmed on a Core2 v1.3: +1 leaned the egg the wrong way
    m.gx = kSignX * ax;
    m.gy = kSignY * ay;
    m.gz = az;
    m.faceDown = az < -0.7f;
    // Shake: the total acceleration strays well away from 1 g. A short cooldown stops
    // one shake registering as several. 0.45 g tuned on a Core2 v1.3: a firm, ordinary
    // shake triggers it; picking it up or tilting stays under ~0.3 g. (1 g needed a hard shake.)
    constexpr float kShakeG = 0.45f;
    const float mag = std::sqrt(ax * ax + ay * ay + az * az);
    if (cooldown_ > 0) {
      --cooldown_;
    } else if (std::fabs(mag - 1.0f) > kShakeG) {
      m.shake = true;
      cooldown_ = 15;  // ~0.5 s at 30 fps
    }
    return m;
  }

 private:
  int cooldown_ = 0;
};

RtcClock rtcClock;
M5CanvasDisplay display;
M5Input input;
M5Sound sound;
ImuMotion motion;
std::unique_ptr<petcore::AcceleratedClock> gameClock;
std::unique_ptr<petcore::Life> life;
std::unique_ptr<petcore::App> app;

}  // namespace

void setup() {
  M5.begin();
  rtcClock.begin();
  display.begin();
  gameClock = std::make_unique<petcore::AcceleratedClock>(rtcClock);  // runs at x1 on the device
  life = std::make_unique<petcore::Life>(petcore::Schedule{}, gameClock->nowMs());
  app = std::make_unique<petcore::App>(*life, *gameClock, display, input, motion, sound);
}

void loop() {
  app->tick();
  M5.delay(33);
}
