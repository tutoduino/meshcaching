#pragma once
#include <stddef.h>
#include <stdint.h>

#include "../hal/Display.h"

// État à afficher sur l'écran principal, composé par l'application.
struct MainView {
  const uint8_t *pubkeyPrefix;   // répéteur surveillé (bandeau)
  size_t prefixLen;
  bool rssiValid;                // false : logo de sommeil à la place du RSSI
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

// Écrans de l'application, dessinés via l'abstraction Display (OLED
// U8g2 ou TFT selon la carte).
class StatusScreen {
public:
  explicit StatusScreen(Display &display) : _d(display) {}

  void showMessage(const char *line1, const char *line2 = "");

  // Écran de démarrage : MESHCACHING en grand, version en dessous.
  void showSplash(const char *version);

  // Écran principal : RSSI en grand tant qu'il est frais, sinon logo de
  // sommeil (zZZ) ; témoin TX, barre de réarmement de l'émission, flash.
  void drawMain(const MainView &view);

private:
  void drawSleepLogo();

  Display &_d;
};
