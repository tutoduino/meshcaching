#pragma once
#include <stdint.h>

// =====================================================================
// Abstraction d'affichage de l'application : un repère logique de
// 128x64 monochrome (blanc sur noir), quel que soit le panneau réel —
// OLED piloté par U8g2 (cf. U8g2Display) ou TFT couleur (adaptateur
// dans le fichier de carte concerné, p. ex. le ST7735 du Heltec T096).
// Les écrans (ui/) ne connaissent que cette interface.
// =====================================================================

// Jeu de polices logique, mappé par chaque adaptateur.
enum class Font : uint8_t {
  kSmall,   // 6x12 — texte courant, bandeau, hints
  kMedium,  // ~12 px gras — titre du splash, Z moyen du logo
  kMenu,    // 10x20 — valeurs du menu
  kBig,     // ~24 px — RSSI, digits hex, grand Z
};

class Display {
public:
  virtual ~Display() {}

  virtual void begin() = 0;

  // Dimensions du repère logique (les adaptateurs de panneaux plus
  // grands centrent cette zone).
  uint16_t width() const { return 128; }
  uint16_t height() const { return 64; }

  virtual void clear() = 0;  // efface la trame en composition
  virtual void send() = 0;   // pousse la trame vers le panneau

  virtual void setFont(Font font) = 0;
  virtual void drawText(int16_t x, int16_t y, const char *utf8) = 0;
  virtual uint16_t textWidth(const char *utf8) = 0;
  virtual void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) = 0;
  virtual void drawHLine(int16_t x, int16_t y, int16_t w) = 0;

  // Encre inversée : dessine en couleur de fond (texte d'un badge plein)
  virtual void setInkInverted(bool inverted) = 0;
  // Inverse toute la trame en composition (flash de réception)
  virtual void invertFrame() = 0;
};
