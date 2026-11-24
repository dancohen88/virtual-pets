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
class RtcClock : public petcore::Clock {
 public:
  void begin() {
    int64_t rtcMs = petcore::timeutil::localMsFromCivil(2026, 1, 1);  // if the RTC isn't set
    if (M5.Rtc.isEnabled()) {
      const auto dt = M5.Rtc.getDateTime();
      rtcMs = petcore::timeutil::localMsFromCivil(dt.date.year, dt.date.month, dt.date.date,
                                                  dt.time.hours, dt.time.minutes,
                                                  dt.time.seconds);
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
    constexpr float kSignX = 1.0f, kSignY = -1.0f;
    m.gx = kSignX * ax;
    m.gy = kSignY * ay;
    m.gz = az;
    m.faceDown = az < -0.7f;
    // Shake: the total acceleration strays well away from 1 g. A short cooldown stops
    // one shake registering as several.
    const float mag = std::sqrt(ax * ax + ay * ay + az * az);
    if (cooldown_ > 0) {
      --cooldown_;
    } else if (std::fabs(mag - 1.0f) > 1.0f) {
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
