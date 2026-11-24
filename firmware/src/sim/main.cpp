// Desktop simulator: the same game core and M5Unified drawing code as the Core2,
// rendered into an SDL2 window.
//
// Keys: arrows Left/Down/Right = buttons A/B/C, mouse click = touch,
//       ] faster, [ slower (0 = paused), J jump one hour, F toggle face-down.
// (M5GFX's SDL port already uses keys 1-6 for window zoom and R/L for rotation.)

#include <M5Unified.h>

#include <chrono>
#include <ctime>
#include <memory>

#include "../m5/M5Platform.h"
#include "petcore/App.h"
#include "petcore/TimeUtil.h"

namespace {

// Local wall-clock time from the computer.
class SystemClock : public petcore::Clock {
 public:
  int64_t nowMs() override {
    using namespace std::chrono;
    const int64_t utcMs =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    std::time_t seconds = static_cast<std::time_t>(utcMs / 1000);
    std::tm local{};
    localtime_r(&seconds, &local);
    return utcMs + static_cast<int64_t>(local.tm_gmtoff) * 1000;
  }
};

// Stand-in for the IMU: F toggles "lying face-down".
class KeyboardMotion : public petcore::Motion {
 public:
  bool down = false;
  bool faceDown() override { return down; }
};

// M5GFX's SDL port turns mapped keys into emulated GPIO pins (pressed = low).
enum : uint8_t { kPinFaster = 100, kPinSlower = 101, kPinJump = 102, kPinFaceDown = 103 };

class KeyEdge {
 public:
  explicit KeyEdge(uint8_t pin) : pin_(pin) {}
  bool pressed() {
    const bool down = !lgfx::gpio_in(pin_);
    const bool edge = down && !wasDown_;
    wasDown_ = down;
    return edge;
  }

 private:
  uint8_t pin_;
  bool wasDown_ = false;
};

constexpr double kSpeeds[] = {0, 1, 60, 600, 3600, 86400};  // x1 real time .. 1 day per second
constexpr int kSpeedCount = sizeof(kSpeeds) / sizeof(kSpeeds[0]);
int speedIndex = 1;

SystemClock systemClock;
M5CanvasDisplay display;
M5Input input;
M5Sound sound;
KeyboardMotion motion;
std::unique_ptr<petcore::AcceleratedClock> gameClock;
std::unique_ptr<petcore::Simulation> sim;
std::unique_ptr<petcore::App> app;
KeyEdge faster(kPinFaster), slower(kPinSlower), jump(kPinJump), faceDown(kPinFaceDown);

}  // namespace

void setup() {
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_RIGHTBRACKET, kPinFaster);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_LEFTBRACKET, kPinSlower);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_j, kPinJump);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_f, kPinFaceDown);

  M5.begin();
  if (auto* panel = static_cast<lgfx::Panel_sdl*>(M5.Display.getPanel())) {
    panel->setWindowTitle("Virtual Pets simulator");
  }
  display.begin();

  gameClock = std::make_unique<petcore::AcceleratedClock>(systemClock);
  sim = std::make_unique<petcore::Simulation>(petcore::Schedule{}, gameClock->nowMs());
  app = std::make_unique<petcore::App>(*sim, *gameClock, display, input, motion, sound);
}

void loop() {
  if (faster.pressed() && speedIndex < kSpeedCount - 1) gameClock->setSpeed(kSpeeds[++speedIndex]);
  if (slower.pressed() && speedIndex > 0) gameClock->setSpeed(kSpeeds[--speedIndex]);
  if (jump.pressed()) gameClock->jump(petcore::timeutil::kMsPerHour);
  if (faceDown.pressed()) motion.down = !motion.down;
  app->tick();
  M5.delay(33);  // ~30 frames per second
}

// SDL needs its event loop on the main thread, so M5GFX runs setup()/loop() on a
// second thread (this is the pattern from M5Unified's PlatformIO_SDL example).
static int user_func(bool* running) {
  setup();
  do {
    loop();
  } while (*running);
  return 0;
}

int main(int, char**) { return lgfx::Panel_sdl::main(user_func, 128); }
