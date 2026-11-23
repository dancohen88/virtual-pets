#include "petcore/App.h"

#include <cstdio>

#include "petcore/TimeUtil.h"

namespace petcore {

namespace {
constexpr int kStripY = 196;
constexpr int kStripH = 40;
constexpr int kPetCx = 160;
constexpr int kPetCy = 128;

const char* const kActionLabels[] = {"Feed", "Pet", "Sleep", "Bath"};
const char* const kNeedLabels[] = {"Food", "Love", "Energy", "Clean"};
const char* const kDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const uint16_t kBg = rgb565(250, 230, 214);
const uint16_t kInk = rgb565(60, 35, 20);
const uint16_t kBarBack = rgb565(225, 200, 180);
const uint16_t kBarGood = rgb565(110, 170, 90);
const uint16_t kBarLow = rgb565(220, 90, 70);
const uint16_t kFur = rgb565(214, 140, 84);
const uint16_t kFurSick = rgb565(170, 150, 130);
const uint16_t kHighlight = rgb565(255, 205, 120);
const uint16_t kWhite = rgb565(255, 255, 255);

const char* modeLabel(Mode m) {
  switch (m) {
    case Mode::Around: return "around";
    case Mode::Away: return "owner at school";
    case Mode::Napping: return "napping (face-down)";
    case Mode::Asleep: return "asleep";
  }
  return "";
}
}  // namespace

App::App(Simulation& sim, AcceleratedClock& clock, Display& display, Input& input, Motion& motion,
         Sound& sound)
    : sim_(sim), clock_(clock), display_(display), input_(input), motion_(motion), sound_(sound) {}

void App::tick() {
  handleInput(input_.poll());
  sim_.setFaceDown(motion_.faceDown());
  sim_.advanceTo(clock_.nowMs());
  render();
}

void App::handleInput(const InputFrame& in) {
  constexpr int count = static_cast<int>(Action::Count);
  if (stripOpen_) {
    if (in.a) selected_ = (selected_ + count - 1) % count;
    if (in.c) selected_ = (selected_ + 1) % count;
    if (in.b) {
      perform(static_cast<Action>(selected_));
      stripOpen_ = false;
    }
  } else if (in.b) {
    stripOpen_ = true;
  }

  if (in.tapped) {
    const int hit = stripOpen_ ? hitStrip(in.tapX, in.tapY) : -1;
    if (hit >= 0) {
      perform(static_cast<Action>(hit));
      stripOpen_ = false;
    } else if (hitPet(in.tapX, in.tapY)) {
      perform(Action::Stroke);  // touching the pet strokes it
    } else {
      stripOpen_ = false;
    }
  }
}

void App::perform(Action action) {
  switch (action) {
    case Action::Feed: sim_.feed(); sound_.tone(660, 80); break;
    case Action::Stroke: sim_.stroke(); sound_.tone(880, 40); break;
    case Action::Sleep:
      if (sim_.state().sleeping) {
        sim_.wake();
      } else {
        sim_.putToBed();
      }
      sound_.tone(330, 120);
      break;
    case Action::Bath: sim_.bathe(); sound_.tone(990, 100); break;
    case Action::Count: break;
  }
}

bool App::hitPet(int x, int y) const {
  const int dx = x - kPetCx, dy = y - kPetCy;
  return dx * dx + dy * dy < 60 * 60;
}

int App::hitStrip(int x, int y) const {
  if (y < kStripY || y >= kStripY + kStripH) return -1;
  const int w = display_.width() / static_cast<int>(Action::Count);
  const int i = x / w;
  return (i >= 0 && i < static_cast<int>(Action::Count)) ? i : -1;
}

void App::render() {
  const PetState& pet = sim_.state();
  display_.beginFrame(kBg);

  // Top line: game time, speed, mode.
  char line[64];
  const int64_t t = sim_.localTimeMs();
  const int minute = timeutil::minuteOfDay(t);
  const double speed = clock_.speed();
  if (speed == 0.0) {
    std::snprintf(line, sizeof line, "%s %02d:%02d  PAUSED", kDays[timeutil::weekday(t)],
                  minute / 60, minute % 60);
  } else {
    std::snprintf(line, sizeof line, "%s %02d:%02d  x%.0f", kDays[timeutil::weekday(t)],
                  minute / 60, minute % 60, speed);
  }
  display_.drawText(4, 4, line, kInk);
  display_.drawText(4, 16, modeLabel(sim_.mode()), kInk);

  // Needs bars (placeholder for the tap-to-show icon strip).
  for (int i = 0; i < kNeedCount; ++i) {
    const int x = 4 + i * 79;
    const float level = pet.needs.level[i];
    display_.drawText(x, 30, kNeedLabels[i], kInk);
    display_.fillRect(x, 40, 72, 8, kBarBack);
    display_.fillRect(x, 40, static_cast<int>(72 * level / tuning::kLevelMax), 8,
                      level < tuning::kCriticalLevel ? kBarLow : kBarGood);
  }

  // Placeholder pet: a blob that gets wider when plump and greyer when unwell.
  const uint16_t fur = pet.health < 50.0f ? kFurSick : kFur;
  const int rx = 50 + static_cast<int>(30 * pet.plumpness);
  display_.fillEllipse(kPetCx, kPetCy, rx, 38, fur);
  if (sim_.mode() == Mode::Asleep) {
    display_.fillRect(kPetCx - 22, kPetCy - 8, 12, 3, kInk);
    display_.fillRect(kPetCx + 10, kPetCy - 8, 12, 3, kInk);
    display_.drawText(kPetCx + rx - 10, kPetCy - 50, "z z", kInk, 2);
  } else {
    display_.fillEllipse(kPetCx - 16, kPetCy - 8, 5, 6, kInk);
    display_.fillEllipse(kPetCx + 16, kPetCy - 8, 5, 6, kInk);
  }

  // Action strip, opened with button B.
  if (stripOpen_) {
    const int count = static_cast<int>(Action::Count);
    const int w = display_.width() / count;
    for (int i = 0; i < count; ++i) {
      display_.fillRect(i * w + 2, kStripY, w - 4, kStripH, i == selected_ ? kHighlight : kWhite);
      display_.drawRect(i * w + 2, kStripY, w - 4, kStripH, kInk);
      display_.drawText(i * w + 12, kStripY + 14, kActionLabels[i], kInk);
    }
  } else {
    display_.drawText(4, 226, "B: actions   tap pet: stroke", kInk);
  }

  display_.endFrame();
}

}  // namespace petcore
