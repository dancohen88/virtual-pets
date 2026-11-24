#pragma once
// Platform implementations built on M5Unified. M5Unified also runs on SDL, so the
// simulator and the Core2 share these; only the clock and motion differ.

#include <M5Unified.h>

#include "petcore/Platform.h"

// Draws into an off-screen canvas, then pushes the whole frame at once (double
// buffering), so the screen never shows a half-drawn frame.
class M5CanvasDisplay : public petcore::Display {
 public:
  void begin() {
    canvas_.setColorDepth(16);
    canvas_.createSprite(M5.Display.width(), M5.Display.height());
    canvas_.setFont(&fonts::Font0);
  }
  int width() const override { return canvas_.width(); }
  int height() const override { return canvas_.height(); }
  void beginFrame(uint16_t background) override { canvas_.fillScreen(background); }
  void fillRect(int x, int y, int w, int h, uint16_t color) override {
    canvas_.fillRect(x, y, w, h, color);
  }
  void drawRect(int x, int y, int w, int h, uint16_t color) override {
    canvas_.drawRect(x, y, w, h, color);
  }
  void fillEllipse(int cx, int cy, int rx, int ry, uint16_t color) override {
    canvas_.fillEllipse(cx, cy, rx, ry, color);
  }
  void drawText(int x, int y, const char* text, uint16_t color, int size) override {
    canvas_.setTextColor(color);
    canvas_.setTextSize(size);
    canvas_.drawString(text, x, y);
  }
  void endFrame() override { canvas_.pushSprite(0, 0); }

 private:
  M5Canvas canvas_{&M5.Display};
};

// Buttons A/B/C are the Core2's touch buttons under the screen (arrow keys in the simulator).
class M5Input : public petcore::Input {
 public:
  petcore::InputFrame poll() override {
    M5.update();
    petcore::InputFrame f;
    f.a = M5.BtnA.wasPressed();
    f.b = M5.BtnB.wasPressed();
    f.c = M5.BtnC.wasPressed();
    const auto touch = M5.Touch.getDetail();
    if (touch.wasPressed() && touch.y < M5.Display.height()) {  // below the screen = buttons
      f.tapped = true;
      f.tapX = touch.x;
      f.tapY = touch.y;
    }
    return f;
  }
};

class M5Sound : public petcore::Sound {
 public:
  void tone(int frequencyHz, int durationMs) override {
    M5.Speaker.tone(frequencyHz, durationMs);
  }
};
