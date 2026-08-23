#pragma once
#include <stddef.h>
#include <stdint.h>

#include <U8g2lib.h>

// État à afficher sur l'écran principal, composé par l'application.
struct MainView {
  const uint8_t *pubkeyPrefix;   // répéteur surveillé (bandeau)
  size_t prefixLen;
  bool rssiValid;                // false : logo de scan à la place du RSSI
  float rssi;
  float snr;
  bool noiseValid;               // bruit de fond mesuré (médiane) dispo ?
  float noiseDbm;
  const char *txBadge;           // témoin d'émission ("LBT", "TX",
                                 // "OCCUPÉ"...) ; nullptr = rien
  bool invert;                   // clignotement : trame inversée
  uint32_t cooldownRemainingMs;  // avant la prochaine émission (0 = prêt)
  uint32_t cooldownTotalMs;
};

// Écrans de l'application, dessinés avec U8g2 (pilote commun aux OLED
// SH1106 / SSD1306 des différentes cartes).
class StatusScreen {
public:
  explicit StatusScreen(U8G2 &display) : _d(display) {}

  void showMessage(const char *line1, const char *line2 = "");

  // Écran de démarrage : MESHCACHING en grand, version en dessous.
  void showSplash(const char *version);

  // Écran principal : RSSI en grand tant qu'il est frais, sinon logo de
  // scan animé ; témoin TX, barre de réarmement de l'émission, flash.
  void drawMain(const MainView &view);

private:
  void drawScanLogo();

  U8G2 &_d;
};
