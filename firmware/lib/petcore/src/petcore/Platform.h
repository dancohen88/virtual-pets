#pragma once
// The hardware abstraction boundary. The game core only ever talks to these small
// interfaces; the simulator and the Core2 each provide their own implementations.

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
  virtual void drawText(int x, int y, const char* text, uint16_t color, int size = 1) = 0;
  virtual void endFrame() = 0;  // push the finished frame to the screen
};

// One frame's worth of input. Buttons are the Core2's three touch buttons under the screen.
struct InputFrame {
  bool a = false, b = false, c = false;  // pressed this frame
  bool tapped = false;                   // screen tapped this frame
  int tapX = 0, tapY = 0;
};

class Input {
 public:
  virtual ~Input() = default;
  virtual InputFrame poll() = 0;
};

class Motion {
 public:
  virtual ~Motion() = default;
  virtual bool faceDown() = 0;
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
