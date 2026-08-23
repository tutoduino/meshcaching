#pragma once
#include <stddef.h>
#include <stdint.h>

#include <U8g2lib.h>

// Écrans de l'application, dessinés avec U8g2 (pilote commun aux OLED
// SH1106 / SSD1306 des différentes cartes ; servira aussi au futur menu).
class StatusScreen {
public:
  explicit StatusScreen(U8G2 &display) : _d(display) {}

  void showMessage(const char *line1, const char *line2 = "");

  // Écran principal : RSSI en grand, SNR et ancienneté du dernier paquet.
  void drawStatus(float rssi, float snr, uint32_t ageSeconds,
                  const uint8_t *pubkeyPrefix, size_t prefixLen);

private:
  U8G2 &_d;
};
