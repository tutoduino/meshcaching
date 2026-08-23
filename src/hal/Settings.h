#pragma once
#include <stdint.h>

#include "RxGain.h"

// =====================================================================
// Configuration persistée de l'application, modifiable via le menu.
// Stockage : NVS (Preferences) sur ESP32, LittleFS interne sur nRF52.
// Les défauts d'usine sont composés par l'application (AppConfig + Board).
// =====================================================================
struct AppSettings {
  uint8_t targetPrefix[2];  // préfixe de clé publique du répéteur cible
  int8_t txPowerDbm;        // puissance d'émission "à l'antenne"
  RxGainMode rxGainMode;
};

bool settingsEqual(const AppSettings &a, const AppSettings &b);

// false si aucune config valide n'est stockée (premier démarrage,
// version incompatible) : l'appelant part alors des défauts d'usine.
bool settingsLoad(AppSettings &out);
void settingsSave(const AppSettings &s);
