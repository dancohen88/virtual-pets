#include "petcore/App.h"

#include <cmath>
#include <cstdio>

#include "petcore/TimeUtil.h"

namespace petcore {

namespace {
constexpr int kStripY = 196;
constexpr int kStripH = 40;
constexpr int kPetCx = 160;
constexpr int kPetCy = 128;

// Egg stage layout.
constexpr int kFloorY = 200;
constexpr float kEggRx = 38.0f;
constexpr float kEggRy = 50.0f;
constexpr int kEggPts = 40;
constexpr int kHatchFrames = 90;  // ~3 s at 30 fps
constexpr float kPi = 3.14159265f;

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
const uint16_t kFloor = rgb565(232, 206, 186);
const uint16_t kWarmRed = rgb565(220, 80, 70);
const uint16_t kColdBlue = rgb565(90, 140, 220);
const uint16_t kFrost = rgb565(240, 248, 255);
const uint16_t kSpeckle = rgb565(226, 196, 156);

struct Rgb {
  float r, g, b;
};
constexpr Rgb kBgRgb{250, 230, 214};
constexpr Rgb kShell{250, 243, 226};
constexpr Rgb kShellCold{214, 232, 248};
constexpr Rgb kShellWarm{255, 222, 214};
constexpr Rgb kRose{255, 170, 170};

Rgb mix(Rgb a, Rgb b, float t) {
  return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}
uint16_t to565(Rgb c) {
  return rgb565(static_cast<uint8_t>(c.r), static_cast<uint8_t>(c.g), static_cast<uint8_t>(c.b));
}
float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

const char* modeLabel(Mode m) {
  switch (m) {
    case Mode::Around: return "around";
    case Mode::Away: return "owner at school";
    case Mode::Napping: return "napping (face-down)";
    case Mode::Asleep: return "asleep";
  }
  return "";
}

// An egg-shaped outline: an ellipse whose top half is a little narrower.
// Local coordinates have y pointing down; angleDeg rotates clockwise on screen.
struct Pt {
  float x, y;
};
struct EggShape {
  float cx, cy, scale, angleRad;
  Pt toScreen(float lx, float ly) const {
    const float c = std::cos(angleRad), s = std::sin(angleRad);
    return {cx + scale * (lx * c - ly * s), cy + scale * (lx * s + ly * c)};
  }
  // Point i of n around the outline, in local coordinates.
  static Pt local(int i, int n) {
    const float t = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(n);
    return {kEggRx * std::sin(t) * (1.0f - 0.12f * std::cos(t)), -kEggRy * std::cos(t)};
  }
};

void fillPolygon(Display& d, const Pt* pts, int n, uint16_t color) {
  float mx = 0, my = 0;
  for (int i = 0; i < n; ++i) {
    mx += pts[i].x;
    my += pts[i].y;
  }
  mx /= static_cast<float>(n);
  my /= static_cast<float>(n);
  for (int i = 0; i < n; ++i) {  // triangle fan from the centre (fine for convex shapes)
    const Pt& a = pts[i];
    const Pt& b = pts[(i + 1) % n];
    d.fillTriangle(static_cast<int>(mx), static_cast<int>(my), static_cast<int>(a.x),
                   static_cast<int>(a.y), static_cast<int>(b.x), static_cast<int>(b.y), color);
  }
}

void outlinePolygon(Display& d, const Pt* pts, int n, uint16_t color, bool closed = true) {
  const int segments = closed ? n : n - 1;
  for (int i = 0; i < segments; ++i) {
    const Pt& a = pts[i];
    const Pt& b = pts[(i + 1) % n];
    // Two passes, one pixel apart, for a 2 px ink line.
    d.drawLine(static_cast<int>(a.x), static_cast<int>(a.y), static_cast<int>(b.x),
               static_cast<int>(b.y), color);
    d.drawLine(static_cast<int>(a.x) + 1, static_cast<int>(a.y), static_cast<int>(b.x) + 1,
               static_cast<int>(b.y), color);
  }
}

// Crack zig-zags in units of the egg's radii (x across, y down; the top of the egg is -1).
const Pt kCrack1[] = {{-0.55f, -0.30f}, {-0.30f, -0.16f}, {-0.08f, -0.36f}, {0.14f, -0.18f},
                      {0.40f, -0.34f}};
const Pt kCrack2[] = {{-0.70f, 0.02f}, {-0.45f, 0.16f}, {-0.22f, -0.02f}, {0.02f, 0.12f}};
const Pt kCrack3[] = {{0.08f, 0.04f}, {0.30f, 0.18f}, {0.52f, 0.00f}, {0.72f, 0.14f}};

void drawCrack(Display& d, const EggShape& e, const Pt* pts, int n, uint16_t color) {
  Pt screen[8];
  for (int i = 0; i < n; ++i) screen[i] = e.toScreen(pts[i].x * kEggRx, pts[i].y * kEggRy);
  outlinePolygon(d, screen, n, color, false);
}

void sparkle(Display& d, int x, int y, int r, uint16_t color) {
  d.drawLine(x - r, y, x + r, y, color);
  d.drawLine(x, y - r, x, y + r, color);
  d.drawLine(x - r / 2, y - r / 2, x + r / 2, y + r / 2, color);
  d.drawLine(x - r / 2, y + r / 2, x + r / 2, y - r / 2, color);
}
}  // namespace

App::App(Life& life, AcceleratedClock& clock, Display& display, Input& input, Motion& motion,
         Sound& sound)
    : life_(life), clock_(clock), display_(display), input_(input), motion_(motion),
      sound_(sound) {}

void App::tick() {
  const InputFrame in = input_.poll();
  const MotionFrame motion = motion_.read();
  life_.setFaceDown(motion.faceDown);

  if (life_.stage() == LifeStage::Egg) {
    handleEggInput(in, motion);
  } else if (life_.stage() == LifeStage::Baby && hatchFrame_ >= kHatchFrames) {
    handleBabyInput(in);
  }

  life_.advanceTo(clock_.nowMs());

  if (life_.stage() != LifeStage::Egg && hatchFrame_ < 0) {
    hatchFrame_ = 0;  // the hatch just happened: play the moment
    sound_.tone(1320, 200);
  }

  if (life_.stage() == LifeStage::Egg) {
    renderEgg();
  } else if (hatchFrame_ < kHatchFrames || life_.stage() == LifeStage::Hatching) {
    renderHatch();
  } else {
    renderBaby();
  }
  if (hatchFrame_ >= 0 && hatchFrame_ < kHatchFrames) ++hatchFrame_;
  ++frame_;
}

// ---------------------------------------------------------------- egg
void App::handleEggInput(const InputFrame& in, const MotionFrame& motion) {
  life_.tiltEgg(leanDegrees(motion));
  if (motion.shake) life_.shakeEgg();

  // Rubbing the egg (moving a finger across it) warms it: at most one stroke every 5 frames.
  if (in.touching && hitEgg(in.touchX, in.touchY)) {
    if (lastTouchX_ >= 0) {
      const int dx = in.touchX - lastTouchX_, dy = in.touchY - lastTouchY_;
      if (dx * dx + dy * dy >= 9 && frame_ - lastStrokeFrame_ >= 5) {
        life_.strokeEgg();
        lastStrokeFrame_ = frame_;
      }
    }
    lastTouchX_ = in.touchX;
    lastTouchY_ = in.touchY;
  } else {
    lastTouchX_ = lastTouchY_ = -1;
  }

  if (in.tapped && hitEgg(in.tapX, in.tapY)) {
    const bool wasToppled = life_.egg().state().toppled;
    life_.tapEgg();
    if (wasToppled && !life_.egg().state().toppled) sound_.tone(520, 80);
  }
}

bool App::hitEgg(int x, int y) const {
  const float dx = static_cast<float>(x) - eggCx_, dy = static_cast<float>(y) - eggCy_;
  const float r = kEggRy + 12.0f;
  return dx * dx + dy * dy < r * r;
}

void App::renderHeader() {
  char line[64];
  const int64_t t = life_.localTimeMs();
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
}

void App::renderEgg() {
  const Egg& egg = life_.egg();
  const EggState& s = egg.state();
  display_.beginFrame(kBg);
  display_.fillRect(0, kFloorY, display_.width(), display_.height() - kFloorY, kFloor);
  renderHeader();

  // Status line: what the egg is waiting for.
  char status[80];
  switch (egg.phase()) {
    case EggPhase::Incubating: {
      const int64_t m = s.warmMs / timeutil::kMsPerMinute;
      std::snprintf(status, sizeof status, "egg  warm time %lldh%02lldm / 8h",
                    static_cast<long long>(m / 60), static_cast<long long>(m % 60));
      break;
    }
    case EggPhase::Cracked: {
      const int mod = timeutil::minuteOfDay(s.hatchDayOpensMs);
      std::snprintf(status, sizeof status, "cracked! hatch day starts %s %02d:%02d",
                    kDays[timeutil::weekday(s.hatchDayOpensMs)], mod / 60, mod % 60);
      break;
    }
    case EggPhase::HatchDay:
    case EggPhase::Hatched: {
      const int64_t sec = s.attentionMs / timeutil::kMsPerSecond;
      std::snprintf(status, sizeof status, "hatch day  attention %lldm%02llds / 15m",
                    static_cast<long long>(sec / 60), static_cast<long long>(sec % 60));
      break;
    }
  }
  display_.drawText(4, 16, status, kInk);

  // New crack: a little click.
  if (s.cracks > shownCracks_) {
    sound_.tone(1200, 60);
    shownCracks_ = s.cracks;
  }

  // Where it sits and leans. Toppled: lying on its side, rolled to an edge.
  const float targetAngle =
      s.toppled ? 90.0f * static_cast<float>(s.toppleSide)
                : (s.leanDeg > tuning::kEggToppleDeg
                       ? tuning::kEggToppleDeg
                       : (s.leanDeg < -tuning::kEggToppleDeg ? -tuning::kEggToppleDeg : s.leanDeg));
  const float targetX = s.toppled ? (s.toppleSide > 0 ? 240.0f : 70.0f) : 160.0f;
  eggAngle_ += (targetAngle - eggAngle_) * 0.25f;
  eggX_ += (targetX - eggX_) * 0.08f;

  // Dizzy wobble after a shake, timed in frames so it is visible at any game speed.
  constexpr int kWobbleFrames = 75;  // ~2.5 s at 30 fps
  if (s.wobbleUntilMs != seenWobbleUntil_) {
    seenWobbleUntil_ = s.wobbleUntilMs;
    wobbleFrames_ = kWobbleFrames;
  }
  float angle = eggAngle_;
  if (wobbleFrames_ > 0) {
    // Big, slowish, decaying swing so it reads as dizzy (tuned on hardware: 14° looked like a twitch).
    angle += std::sin(static_cast<float>(frame_) * 0.6f) * 26.0f *
             static_cast<float>(wobbleFrames_) / static_cast<float>(kWobbleFrames);
    --wobbleFrames_;
  }

  // Temperature: warm 0..1 above the threshold, cold 0..1 below it.
  const float warm = clamp01((s.warmth - tuning::kEggWarmThreshold) /
                             (tuning::kLevelMax - tuning::kEggWarmThreshold));
  const float cold = clamp01((tuning::kEggWarmThreshold - s.warmth) / tuning::kEggWarmThreshold);

  // Heartbeat: a gentle double pulse every ~1.2 s while warm.
  float scale = 1.0f;
  if (s.warmth >= tuning::kEggWarmThreshold) {
    const unsigned beat = frame_ % 36;
    const float bump = beat < 3 ? 1.0f : (beat >= 6 && beat < 9 ? 0.6f : 0.0f);
    scale += 0.03f * bump;
  }

  const float rad = angle * kPi / 180.0f;
  const float halfH = std::sqrt(std::pow(kEggRy * std::cos(rad), 2.0f) +
                                std::pow(kEggRx * std::sin(rad), 2.0f));
  EggShape e{eggX_, static_cast<float>(kFloorY) - halfH * scale, scale, rad};
  eggCx_ = e.cx;
  eggCy_ = e.cy;

  Pt outline[kEggPts];
  for (int i = 0; i < kEggPts; ++i) {
    const Pt l = EggShape::local(i, kEggPts);
    outline[i] = e.toScreen(l.x, l.y);
  }

  // Rosy glow behind a warm egg: three faint, slightly larger eggs.
  if (warm > 0.0f) {
    for (int k = 3; k >= 1; --k) {
      EggShape halo = e;
      halo.scale = scale * (1.0f + 0.07f * static_cast<float>(k));
      Pt ring[kEggPts];
      for (int i = 0; i < kEggPts; ++i) {
        const Pt l = EggShape::local(i, kEggPts);
        ring[i] = halo.toScreen(l.x, l.y);
      }
      fillPolygon(display_, ring, kEggPts,
                  to565(mix(kBgRgb, kRose, warm * (0.55f - 0.15f * static_cast<float>(k)))));
    }
  }

  // The shell, tinted by temperature.
  Rgb shell = kShell;
  if (warm > 0.0f) shell = mix(kShell, kShellWarm, warm);
  if (cold > 0.0f) shell = mix(kShell, kShellCold, cold);
  fillPolygon(display_, outline, kEggPts, to565(shell));

  // Speckles.
  const Pt spots[] = {{0.30f, -0.10f}, {-0.36f, 0.26f}, {0.12f, 0.46f}, {-0.16f, -0.52f}};
  for (const Pt& p : spots) {
    const Pt q = e.toScreen(p.x * kEggRx, p.y * kEggRy);
    display_.fillEllipse(static_cast<int>(q.x), static_cast<int>(q.y), 3, 3, kSpeckle);
  }

  // Frost creeping in from the edge as it cools: wedges and little star crystals.
  const int crystals = static_cast<int>(cold * 20.0f + 0.5f);
  for (int c = 0; c < crystals; ++c) {
    const int i = (c * 7) % kEggPts;
    const Pt& p = outline[i];
    const Pt& p2 = outline[(i + 1) % kEggPts];
    const float depth = 0.10f + 0.12f * cold;
    const Pt q{p.x + (e.cx - p.x) * depth, p.y + (e.cy - p.y) * depth};
    display_.fillTriangle(static_cast<int>(p.x), static_cast<int>(p.y), static_cast<int>(p2.x),
                          static_cast<int>(p2.y), static_cast<int>(q.x), static_cast<int>(q.y),
                          kFrost);
    sparkle(display_, static_cast<int>(q.x), static_cast<int>(q.y), 2 + static_cast<int>(3 * cold),
            kFrost);
  }

  outlinePolygon(display_, outline, kEggPts, kInk);
  if (s.cracks >= 1) drawCrack(display_, e, kCrack1, 5, kInk);
  if (s.cracks >= 2) drawCrack(display_, e, kCrack2, 4, kInk);
  if (s.cracks >= 3) drawCrack(display_, e, kCrack3, 4, kInk);

  // Thermometer at the side, with a tick at the incubation threshold.
  const int tx = 294, ty = 64, tw = 12, th = 112;
  const uint16_t mercury = s.warmth >= tuning::kEggWarmThreshold ? kWarmRed : kColdBlue;
  display_.fillRect(tx, ty, tw, th, kWhite);
  const int fill = static_cast<int>(static_cast<float>(th) * s.warmth / tuning::kLevelMax);
  display_.fillRect(tx, ty + th - fill, tw, fill, mercury);
  display_.drawRect(tx, ty, tw, th, kInk);
  display_.fillEllipse(tx + tw / 2, ty + th + 8, 10, 10, mercury);
  const int tick =
      ty + th - static_cast<int>(static_cast<float>(th) * tuning::kEggWarmThreshold / tuning::kLevelMax);
  display_.drawLine(tx - 5, tick, tx + tw + 4, tick, kInk);

  display_.drawText(4, 226, s.toppled ? "it fell over! tap it or tilt back" : "rub the egg to warm it",
                    kInk);
  display_.endFrame();
}

void App::renderHatch() {
  display_.beginFrame(kBg);
  display_.fillRect(0, kFloorY, display_.width(), display_.height() - kFloorY, kFloor);
  renderHeader();
  display_.drawText(4, 16, "hatching...", kInk);

  const long f = hatchFrame_ < 0 ? 0 : hatchFrame_;
  const float shakeDeg = f < 30 ? std::sin(static_cast<float>(f) * 1.3f) * 10.0f : 0.0f;
  const float rad = shakeDeg * kPi / 180.0f;
  EggShape e{160.0f, static_cast<float>(kFloorY) - kEggRy, 1.0f, rad};

  // Split the outline into the cap (top) and the base, along a line just above the middle.
  Pt cap[kEggPts], base[kEggPts];
  int nc = 0, nb = 0;
  for (int i = 0; i < kEggPts; ++i) {
    const Pt l = EggShape::local(i, kEggPts);
    if (l.y < -0.15f * kEggRy) {
      cap[nc++] = l;
    } else {
      base[nb++] = l;
    }
  }
  const float lift = f > 30 ? static_cast<float>(f - 30) * 2.5f : 0.0f;
  const float capTurn = f > 30 ? static_cast<float>(f - 30) * 3.0f * kPi / 180.0f : 0.0f;

  // The baby blob rising out of the shell.
  if (f > 30) {
    const float grow = clamp01(static_cast<float>(f - 30) / 40.0f);
    display_.fillEllipse(160, static_cast<int>(static_cast<float>(kFloorY) - kEggRy - 10.0f * grow),
                         static_cast<int>(10.0f + 26.0f * grow), static_cast<int>(8.0f + 20.0f * grow),
                         kFur);
    display_.fillEllipse(150, static_cast<int>(static_cast<float>(kFloorY) - kEggRy - 14.0f * grow), 3,
                         4, kInk);
    display_.fillEllipse(170, static_cast<int>(static_cast<float>(kFloorY) - kEggRy - 14.0f * grow), 3,
                         4, kInk);
  }

  Pt sb[kEggPts], sc[kEggPts];
  for (int i = 0; i < nb; ++i) sb[i] = e.toScreen(base[i].x, base[i].y);
  EggShape capShape{160.0f + lift * 0.4f, static_cast<float>(kFloorY) - kEggRy - lift, 1.0f,
                    rad + capTurn};
  for (int i = 0; i < nc; ++i) sc[i] = capShape.toScreen(cap[i].x, cap[i].y);
  fillPolygon(display_, sb, nb, to565(kShellWarm));
  outlinePolygon(display_, sb, nb, kInk);
  fillPolygon(display_, sc, nc, to565(kShellWarm));
  outlinePolygon(display_, sc, nc, kInk);
  if (f < 30) {
    drawCrack(display_, e, kCrack1, 5, kInk);
    drawCrack(display_, e, kCrack2, 4, kInk);
    drawCrack(display_, e, kCrack3, 4, kInk);
  }

  if (f > 45) {
    for (int k = 0; k < 6; ++k) {
      const float a = static_cast<float>(k) * kPi / 3.0f + static_cast<float>(f) * 0.05f;
      sparkle(display_, 160 + static_cast<int>(std::cos(a) * 70.0f),
              110 + static_cast<int>(std::sin(a) * 50.0f), 5, kHighlight);
    }
    display_.drawText(118, 40, "Hatched!", kInk, 2);
  }
  display_.endFrame();
}

// ---------------------------------------------------------------- baby
void App::handleBabyInput(const InputFrame& in) {
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
  Simulation& sim = life_.pet();
  switch (action) {
    case Action::Feed: sim.feed(); sound_.tone(660, 80); break;
    case Action::Stroke: sim.stroke(); sound_.tone(880, 40); break;
    case Action::Sleep:
      if (sim.state().sleeping) {
        sim.wake();
      } else {
        sim.putToBed();
      }
      sound_.tone(330, 120);
      break;
    case Action::Bath: sim.bathe(); sound_.tone(990, 100); break;
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

void App::renderBaby() {
  const Simulation& sim = life_.pet();
  const PetState& pet = sim.state();
  display_.beginFrame(kBg);
  renderHeader();
  display_.drawText(4, 16, modeLabel(sim.mode()), kInk);

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
  if (sim.mode() == Mode::Asleep) {
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
