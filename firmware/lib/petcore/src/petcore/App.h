#pragma once
// Stage 1 placeholder app: needs bars, a pet blob and an action strip.
// Pure C++ on top of the Platform interfaces, so the same code runs in the
// simulator and on the Core2.

#include "petcore/Clock.h"
#include "petcore/Platform.h"
#include "petcore/Simulation.h"

namespace petcore {

enum class Action { Feed, Stroke, Sleep, Bath, Count };

class App {
 public:
  App(Simulation& sim, AcceleratedClock& clock, Display& display, Input& input, Motion& motion,
      Sound& sound);

  // Call every frame: read input, apply actions, advance the simulation, draw.
  void tick();

  bool stripOpen() const { return stripOpen_; }
  int selected() const { return selected_; }

 private:
  void handleInput(const InputFrame& in);
  void perform(Action action);
  void render();
  bool hitPet(int x, int y) const;
  int hitStrip(int x, int y) const;  // action index or -1

  Simulation& sim_;
  AcceleratedClock& clock_;
  Display& display_;
  Input& input_;
  Motion& motion_;
  Sound& sound_;
  bool stripOpen_ = false;
  int selected_ = 0;
};

}  // namespace petcore
