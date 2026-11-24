// Desktop simulator: the same game core and M5Unified drawing code as the Core2,
// rendered into an SDL2 window.
//
// Keys: arrows Left/Down/Right = buttons A/B/C, mouse click/drag = touch,
//       ] faster, [ slower (0 = paused), J jump one hour, F toggle face-down,
//       A / D tilt left / right 10 degrees, S level, X shake.
// (M5GFX's SDL port already uses keys 1-6 for window zoom and R/L for rotation.)

#include <M5Unified.h>

#include <chrono>
#include <cmath>
#include <ctime>
#include <memory>

#include "../m5/M5Platform.h"
#include "petcore/App.h"
#include "petcore/Life.h"
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

// Stand-in for the IMU: keys set a tilt angle, a shake and "lying face-down".
// The device is imagined held upright, so tilting rotates gravity in the screen plane.
class KeyboardMotion : public petcore::Motion {
 public:
  bool down = false;
  float tiltDeg = 0.0f;
  bool shakeNext = false;
  petcore::MotionFrame read() override {
    petcore::MotionFrame m;
    const float rad = tiltDeg * 3.14159265f / 180.0f;
    m.gx = std::sin(rad);
    m.gy = std::cos(rad);
    m.gz = 0.0f;
    m.shake = shakeNext;
    shakeNext = false;
    m.faceDown = down;
    return m;
  }
};

// M5GFX's SDL port turns mapped keys into emulated GPIO pins (pressed = low).
enum : uint8_t {
  kPinFaster = 100,
  kPinSlower = 101,
  kPinJump = 102,
  kPinFaceDown = 103,
  kPinTiltLeft = 104,
  kPinTiltRight = 105,
  kPinLevel = 106,
  kPinShake = 107,
};

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
std::unique_ptr<petcore::Life> life;
std::unique_ptr<petcore::App> app;
KeyEdge faster(kPinFaster), slower(kPinSlower), jump(kPinJump), faceDown(kPinFaceDown);
KeyEdge tiltLeft(kPinTiltLeft), tiltRight(kPinTiltRight), level(kPinLevel), shake(kPinShake);

}  // namespace

void setup() {
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_RIGHTBRACKET, kPinFaster);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_LEFTBRACKET, kPinSlower);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_j, kPinJump);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_f, kPinFaceDown);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_a, kPinTiltLeft);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_d, kPinTiltRight);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_s, kPinLevel);
  lgfx::Panel_sdl::addKeyCodeMapping(SDLK_x, kPinShake);

  M5.begin();
  if (auto* panel = static_cast<lgfx::Panel_sdl*>(M5.Display.getPanel())) {
    panel->setWindowTitle("Virtual Pets simulator");
  }
  display.begin();

  gameClock = std::make_unique<petcore::AcceleratedClock>(systemClock);
  life = std::make_unique<petcore::Life>(petcore::Schedule{}, gameClock->nowMs());
  app = std::make_unique<petcore::App>(*life, *gameClock, display, input, motion, sound);
}

void loop() {
  if (faster.pressed() && speedIndex < kSpeedCount - 1) gameClock->setSpeed(kSpeeds[++speedIndex]);
  if (slower.pressed() && speedIndex > 0) gameClock->setSpeed(kSpeeds[--speedIndex]);
  if (jump.pressed()) gameClock->jump(petcore::timeutil::kMsPerHour);
  if (faceDown.pressed()) motion.down = !motion.down;
  if (tiltLeft.pressed() && motion.tiltDeg > -90.0f) motion.tiltDeg -= 10.0f;
  if (tiltRight.pressed() && motion.tiltDeg < 90.0f) motion.tiltDeg += 10.0f;
  if (level.pressed()) motion.tiltDeg = 0.0f;
  if (shake.pressed()) motion.shakeNext = true;
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
