#pragma once
// The hardware abstraction boundary. The game core only ever talks to these small
// interfaces; the simulator and the Core2 each provide their own implementations.

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace petcore {

// 16-bit colour as the display stores it: 5 bits red, 6 green, 5 blue.
constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

class Display {
 public:
  virtual ~Display() = default;
  virtual int width() const = 0;
  virtual int height() const = 0;
  virtual void beginFrame(uint16_t background) = 0;
  virtual void fillRect(int x, int y, int w, int h, uint16_t color) = 0;
  virtual void drawRect(int x, int y, int w, int h, uint16_t color) = 0;
  virtual void fillEllipse(int cx, int cy, int rx, int ry, uint16_t color) = 0;
  virtual void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) = 0;
  virtual void drawLine(int x0, int y0, int x1, int y1, uint16_t color) = 0;
  virtual void drawText(int x, int y, const char* text, uint16_t color, int size = 1) = 0;
  virtual void endFrame() = 0;  // push the finished frame to the screen
};

// One frame's worth of input. Buttons are the Core2's three touch buttons under the screen.
struct InputFrame {
  bool a = false, b = false, c = false;  // pressed this frame
  bool tapped = false;                   // screen tapped this frame
  int tapX = 0, tapY = 0;
  bool touching = false;                 // a finger (or the mouse button) is down right now
  int touchX = 0, touchY = 0;
};

class Input {
 public:
  virtual ~Input() = default;
  virtual InputFrame poll() = 0;
};

// One reading of the motion sensor, in screen terms.
// Gravity is in g along the screen's right (gx), down (gy) and out-of-the-glass (gz) axes:
// held upright gx = 0, gy = 1; lying flat face-up gz = -1.
struct MotionFrame {
  float gx = 0.0f, gy = 1.0f, gz = 0.0f;
  bool shake = false;     // a shake was detected this frame
  bool faceDown = false;  // lying face-down (the pet naps)
};

// Sideways lean in degrees: the angle of gravity away from the plane through the screen's
// vertical axis. Works the same whether the device is held upright or lying flat, so
// tilting it left/right (rolling it) always leans the egg. Positive = leaning right.
inline float leanDegrees(const MotionFrame& m) {
  const float mag = std::sqrt(m.gx * m.gx + m.gy * m.gy + m.gz * m.gz);
  if (mag < 0.2f) return 0.0f;  // free fall or no data: treat as upright
  float s = m.gx / mag;
  if (s > 1.0f) s = 1.0f;
  if (s < -1.0f) s = -1.0f;
  return std::asin(s) * 57.29578f;
}

class Motion {
 public:
  virtual ~Motion() = default;
  virtual MotionFrame read() = 0;
};

class Sound {
 public:
  virtual ~Sound() = default;
  virtual void tone(int frequencyHz, int durationMs) = 0;  // placeholder until clips exist
};

// Save slots for the pet's state (implemented in a later stage).
class Storage {
 public:
  virtual ~Storage() = default;
  virtual bool save(const char* key, const void* data, size_t length) = 0;
  virtual bool load(const char* key, void* data, size_t length) = 0;
};

}  // namespace petcore
