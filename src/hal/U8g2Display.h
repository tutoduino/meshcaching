#pragma once
#include <U8g2lib.h>

#include "Display.h"

// Adaptateur Display -> U8g2, pour les OLED monochromes 128x64
// (SH1106, SSD1306) des cartes actuelles.
class U8g2Display : public Display {
public:
  explicit U8g2Display(U8G2 &u8g2) : _d(u8g2) {}

  void begin() override {
    // I2C à 400 kHz : une trame complète passe en ~25 ms, nécessaire aux
    // animations de l'écran principal.
    _d.setBusClock(400000);
    _d.begin();
  }

  void clear() override { _d.clearBuffer(); }
  void send() override { _d.sendBuffer(); }

  void setFont(Font font) override {
    switch (font) {
      case Font::kSmall: _d.setFont(u8g2_font_6x12_tf); break;
      case Font::kMedium: _d.setFont(u8g2_font_helvB12_tr); break;
      case Font::kMenu: _d.setFont(u8g2_font_10x20_tf); break;
      case Font::kBig: _d.setFont(u8g2_font_logisoso24_tr); break;
    }
  }

  void drawText(int16_t x, int16_t y, const char *utf8) override {
    _d.drawUTF8(x, y, utf8);
  }
  uint16_t textWidth(const char *utf8) override {
    return _d.getUTF8Width(utf8);
  }
  void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) override {
    _d.drawBox(x, y, w, h);
  }
  void drawHLine(int16_t x, int16_t y, int16_t w) override {
    _d.drawHLine(x, y, w);
  }

  void setInkInverted(bool inverted) override {
    _d.setDrawColor(inverted ? 0 : 1);
  }
  void invertFrame() override {
    _d.setDrawColor(2);  // XOR
    _d.drawBox(0, 0, _d.getDisplayWidth(), _d.getDisplayHeight());
    _d.setDrawColor(1);
  }

private:
  U8G2 &_d;
};
