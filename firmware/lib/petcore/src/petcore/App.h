#pragma once
// Placeholder app for Stages 1-2: the egg (warmth, balance, cracks, hatch) and then the
// baby's needs bars, pet blob and action strip. Pure C++ on top of the Platform interfaces,
// so the same code runs in the simulator and on the Core2. Real art arrives in Stage 3.

#include "petcore/Clock.h"
#include "petcore/Life.h"
#include "petcore/Platform.h"
#include "petcore/ScreenPower.h"

namespace petcore {

enum class Action { Feed, Stroke, Sleep, Bath, Count };

class App {
 public:
  App(Life& life, AcceleratedClock& clock, Display& display, Input& input, Motion& motion,
      Sound& sound);

  // Call every frame (~30 fps): read input and motion, advance the game, draw.
  void tick();

  bool stripOpen() const { return stripOpen_; }
  int selected() const { return selected_; }

 private:
  void handleEggInput(const InputFrame& in, const MotionFrame& motion);
  void handleBabyInput(const InputFrame& in);
  void perform(Action action);
  void renderHeader();
  void renderEgg();
  void renderHatch();
  void renderBaby();
  bool hitPet(int x, int y) const;
  void applyScreenLevel();
  bool hitEgg(int x, int y) const;
  int hitStrip(int x, int y) const;  // action index or -1

  Life& life_;
  AcceleratedClock& clock_;
  Display& display_;
  Input& input_;
  Motion& motion_;
  Sound& sound_;
  bool stripOpen_ = false;
  int selected_ = 0;

  // Presentation only (frame-based, so animations run in real time at any game speed).
  unsigned frame_ = 0;
  float eggX_ = 160.0f;      // where the egg sits on the floor (rolls to an edge when toppled)
  float eggAngle_ = 0.0f;    // drawn angle in degrees, eased toward the target
  int lastTouchX_ = -1, lastTouchY_ = -1;
  unsigned lastStrokeFrame_ = 0;
  int shownCracks_ = 0;
  long hatchFrame_ = -1;     // frames since the hatch began; -1 = not hatching
  int64_t seenWobbleUntil_ = 0;  // egg's wobble deadline last seen, to spot a new shake
  int wobbleFrames_ = 0;          // dizzy wobble frames left
  float eggCx_ = 160.0f, eggCy_ = 140.0f;  // last drawn egg centre, for hit tests
  ScreenPower power_;
  float lastLean_ = 0.0f;  // previous frame's lean, to notice being handled
  float leanAtDark_ = 0.0f;  // lean when the screen went dark, to notice being picked up
  bool blanked_ = false;   // screen-off frame already drawn
};

}  // namespace petcore
