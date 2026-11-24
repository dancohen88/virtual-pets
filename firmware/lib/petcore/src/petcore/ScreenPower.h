#pragma once
// Screen power-saving: On -> Dim -> Off after idle frames; any activity wakes it.
// Frame-based (like the animations), so it behaves the same at any game speed.
// The activity that wakes a dark screen is reported as consumed, so a wake-up tap never
// also strokes the egg or presses a button.

namespace petcore {

enum class ScreenLevel { On, Dim, Off };

class ScreenPower {
 public:
  // Call once per frame. Returns true when this frame's activity only woke the screen
  // from Off, so the caller should ignore that input.
  bool update(bool activity);
  ScreenLevel level() const { return level_; }

 private:
  ScreenLevel level_ = ScreenLevel::On;
  int idleFrames_ = 0;
};

}  // namespace petcore
