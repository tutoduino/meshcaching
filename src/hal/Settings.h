#pragma once
#include <stdint.h>

#include "RxGain.h"

// =====================================================================
// Configuration persistee de l'application, modifiable via le menu.
// Stockage : NVS (Preferences) sur ESP32, LittleFS interne sur nRF52.
// Les defauts d'usine sont composes par l'application (AppConfig + Board).
// =====================================================================
struct AppSettings {
  uint8_t targetPrefix[2];  // prefixe de cle publique du repeteur cible
  int8_t txPowerDbm;        // puissance d'emission "a l'antenne"
  RxGainMode rxGainMode;
};

bool settingsEqual(const AppSettings &a, const AppSettings &b);

// false si aucune config valide n'est stockee (premier demarrage,
// version incompatible) : l'appelant part alors des defauts d'usine.
bool settingsLoad(AppSettings &out);
void settingsSave(const AppSettings &s);
