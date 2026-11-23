#pragma once
#include <cstdint>

namespace petcore {

// A source of local wall-clock time (see TimeUtil.h for "local milliseconds").
// Device: RTC + a microsecond timer. Simulator: the computer's clock. Tests: a fake.
class Clock {
 public:
  virtual ~Clock() = default;
  virtual int64_t nowMs() = 0;
};

// Turns clock readings into elapsed time, treating a clock that goes backwards
// (flat RTC battery, someone changing the time) as "no time passed".
// This is monotonic time: game time only ever moves forward.
class ElapsedTracker {
 public:
  explicit ElapsedTracker(int64_t startMs) : last_(startMs) {}

  int64_t elapsedSince(int64_t nowMs) {
    if (nowMs < last_) {  // went backwards: re-baseline, no time passes
      last_ = nowMs;
      return 0;
    }
    const int64_t elapsed = nowMs - last_;
    last_ = nowMs;
    return elapsed;
  }

 private:
  int64_t last_;
};

// Debug time acceleration: wraps a real clock and runs it N times faster.
// Speed 0 pauses. Changing speed re-anchors, so time never jumps when you change it.
class AcceleratedClock : public Clock {
 public:
  explicit AcceleratedClock(Clock& base) : base_(base) {
    anchorBase_ = base_.nowMs();
    anchorVirtual_ = anchorBase_;
  }

  int64_t nowMs() override {
    const int64_t realElapsed = base_.nowMs() - anchorBase_;
    return anchorVirtual_ + static_cast<int64_t>(static_cast<double>(realElapsed) * speed_);
  }

  void setSpeed(double speed) {
    anchorVirtual_ = nowMs();
    anchorBase_ = base_.nowMs();
    speed_ = speed < 0.0 ? 0.0 : speed;
  }

  double speed() const { return speed_; }

  // Skip ahead instantly (e.g. "one hour later") without changing speed.
  void jump(int64_t ms) { anchorVirtual_ += ms; }

 private:
  Clock& base_;
  int64_t anchorBase_ = 0;
  int64_t anchorVirtual_ = 0;
  double speed_ = 1.0;
};

}  // namespace petcore
