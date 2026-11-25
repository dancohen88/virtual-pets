// Hidden hardware self-test for the Core2: tap touch buttons A, C, A, C within 3 s.
// Single taps on purpose: the Core2's virtual buttons register one touch at a time
// (holding two together only ever reports one), and fingers resting on the panel at
// power-on become its "untouched" baseline, so neither combos nor boot holds work.
// Shows live IMU, battery, clock and touch readings, lights each button as it's pressed,
// and exercises the speaker and vibration motor. Power off to leave.
// Device-only diagnostics, so it talks to M5Unified directly (it isn't game code).
#pragma once

#include <M5Unified.h>

#include <cmath>

#include "HardwareTuning.h"

namespace selftest {

// Call once per frame after M5.update(); true once the knock A, C, A, C is completed in time.
class SecretKnock {
 public:
  bool update() {
    int pressed = -1;
    if (M5.BtnA.wasPressed()) pressed = 0;
    if (M5.BtnB.wasPressed()) pressed = 1;
    if (M5.BtnC.wasPressed()) pressed = 2;
    ++frame_;
    if (step_ > 0 && frame_ - startFrame_ > kWindowFrames) step_ = 0;  // too slow: start over
    if (pressed < 0) return false;
    if (pressed == kPattern[step_]) {
      if (step_ == 0) startFrame_ = frame_;
      if (++step_ == kLength) return true;
    } else {
      step_ = (pressed == kPattern[0]) ? 1 : 0;  // a wrong tap restarts the knock
      if (step_ == 1) startFrame_ = frame_;
    }
    return false;
  }

 private:
  static constexpr int kLength = 4;
  static constexpr int kPattern[kLength] = {0, 2, 0, 2};  // A, C, A, C
  static constexpr int kWindowFrames = 90;                // ~3 s at 30 fps
  int step_ = 0, frame_ = 0, startFrame_ = 0;
};

inline void run() {  // never returns
  M5Canvas c(&M5.Display);
  c.setColorDepth(16);
  c.createSprite(M5.Display.width(), M5.Display.height());
  M5.Speaker.setVolume(160);

  int frame = 0, shakeFlash = 0, vibOffAt = -1, chirpAt = -1;
  while (true) {
    M5.update();

    // --- actions ---
    if (M5.BtnA.wasPressed()) { M5.Power.setVibration(180); vibOffAt = frame + 8; }
    if (frame == vibOffAt) M5.Power.setVibration(0);
    if (M5.BtnB.wasPressed()) M5.Speaker.tone(880, 150);
    if (M5.BtnC.wasPressed()) { M5.Speaker.tone(660, 80); chirpAt = frame + 3; }
    if (frame == chirpAt) M5.Speaker.tone(1320, 120);

    // --- readings ---
    float ax = 0, ay = 0, az = 0;
    const bool imuOk = M5.Imu.isEnabled() && M5.Imu.getAccel(&ax, &ay, &az);
    const float gx = hw::kImuSignX * ax;
    const float mag = std::sqrt(ax * ax + ay * ay + az * az);
    const float lean = mag > 0.1f ? std::asin(std::fmax(-1.0f, std::fmin(1.0f, gx / mag))) * 57.2958f : 0.0f;
    if (imuOk && std::fabs(mag - 1.0f) > hw::kShakeG) shakeFlash = 15;
    const auto dt = M5.Rtc.getDateTime();

    // --- draw ---
    c.fillSprite(TFT_BLACK);
    c.setTextSize(1);
    c.setTextColor(TFT_YELLOW);
    c.setCursor(6, 6);
    c.print("HARDWARE SELF-TEST   (power off to exit)");
    c.setTextColor(TFT_WHITE);
    c.setCursor(6, 24);
    static const char* kImuNames[] = {"none", "unknown", "SH200Q", "MPU6050", "MPU6886", "MPU9250", "BMI270"};
    const int imuType = static_cast<int>(M5.Imu.getType());
    c.printf("IMU   %s (type %d)  %s\n", (imuType >= 0 && imuType <= 6) ? kImuNames[imuType] : "?", imuType,
             imuOk ? "OK" : "NOT FOUND");
    c.setCursor(6, 38);
    c.printf("accel x %+.2f  y %+.2f  z %+.2f g", ax, ay, az);
    c.setCursor(6, 52);
    c.printf("lean  %+4.0f deg (tilt right = +)   %s", lean, az < hw::kFaceDownZ ? "FACE-DOWN" : "");
    c.setCursor(6, 66);
    c.printf("shake |g-1| = %.2f  (trigger > %.2f)", std::fabs(mag - 1.0f), hw::kShakeG);
    if (shakeFlash > 0) {
      c.fillRoundRect(230, 62, 80, 16, 4, TFT_ORANGE);
      c.setTextColor(TFT_BLACK);
      c.setCursor(250, 66);
      c.print("SHAKE!");
      c.setTextColor(TFT_WHITE);
      --shakeFlash;
    }
    c.setCursor(6, 84);
    c.printf("power %d%%  %d mV  %s", static_cast<int>(M5.Power.getBatteryLevel()),
             static_cast<int>(M5.Power.getBatteryVoltage()),
             M5.Power.isCharging() == m5::Power_Class::is_charging ? "charging" : "on battery");
    c.setCursor(6, 98);
    c.printf("clock %04d-%02d-%02d %02d:%02d:%02d", dt.date.year, dt.date.month, dt.date.date,
             dt.time.hours, dt.time.minutes, dt.time.seconds);
    c.setCursor(6, 116);
    c.setTextColor(TFT_DARKGREY);
    c.print("Touch the screen: a dot follows your finger.");

    // Lean gauge: a needle that should point the way you tilt.
    const int gx0 = 160, gy0 = 175;
    c.drawCircle(gx0, gy0, 30, TFT_DARKGREY);
    c.drawLine(gx0, gy0, gx0 + static_cast<int>(28 * std::sin(lean / 57.2958f)),
               gy0 - static_cast<int>(28 * std::cos(lean / 57.2958f)), TFT_GREEN);

    if (M5.Touch.getCount() > 0) {
      const auto t = M5.Touch.getDetail();
      if (t.y < c.height()) c.fillCircle(t.x, t.y, 8, TFT_CYAN);
    }

    // Button boxes along the bottom, above the physical touch buttons.
    const char* labels[3] = {"A: buzz", "B: beep", "C: chirp"};
    const bool down[3] = {M5.BtnA.isPressed(), M5.BtnB.isPressed(), M5.BtnC.isPressed()};
    for (int i = 0; i < 3; ++i) {
      const int x = 8 + i * 104;
      if (down[i]) c.fillRoundRect(x, 214, 96, 22, 5, TFT_GREEN);
      else c.drawRoundRect(x, 214, 96, 22, 5, TFT_WHITE);
      c.setTextColor(down[i] ? TFT_BLACK : TFT_WHITE);
      c.setCursor(x + 24, 221);
      c.print(labels[i]);
    }

    c.pushSprite(0, 0);
    M5.delay(33);
    ++frame;
  }
}

}  // namespace selftest
