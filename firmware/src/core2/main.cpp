// M5Stack Core2 entry point. Same game core as the simulator; real RTC, IMU and touch.

#include <M5Unified.h>
#include <esp_timer.h>

#include <memory>

#include "../m5/M5Platform.h"
#include "petcore/App.h"
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

// Face-down from the accelerometer. The axis sign still needs checking on real hardware.
class ImuMotion : public petcore::Motion {
 public:
  bool faceDown() override {
    float ax = 0, ay = 0, az = 0;
    if (!M5.Imu.isEnabled() || !M5.Imu.getAccel(&ax, &ay, &az)) return false;
    return az < -0.7f;
  }
};

RtcClock rtcClock;
M5CanvasDisplay display;
M5Input input;
M5Sound sound;
ImuMotion motion;
std::unique_ptr<petcore::AcceleratedClock> gameClock;
std::unique_ptr<petcore::Simulation> sim;
std::unique_ptr<petcore::App> app;

}  // namespace

void setup() {
  M5.begin();
  rtcClock.begin();
  display.begin();
  gameClock = std::make_unique<petcore::AcceleratedClock>(rtcClock);  // runs at x1 on the device
  sim = std::make_unique<petcore::Simulation>(petcore::Schedule{}, gameClock->nowMs());
  app = std::make_unique<petcore::App>(*sim, *gameClock, display, input, motion, sound);
}

void loop() {
  app->tick();
  M5.delay(33);
}
