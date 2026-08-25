#pragma once
#include <stdint.h>

#include "RxGain.h"

// Choix des mesures de signal affichées sur l'écran principal
enum class RssiDisplayMode : uint8_t {
  kBoth = 0,      // RSSI moyen et despreader côte à côte — défaut
  kRssiOnly = 1,  // RSSI moyen seul, en grand
  kDespreadOnly = 2,  // RSSI du despreader seul, en grand
};

// =====================================================================
// Configuration persistée de l'application, modifiable via le menu.
// Stockage : NVS (Preferences) sur ESP32, LittleFS interne sur nRF52.
// Les défauts d'usine sont composés par l'application (AppConfig + Board).
// =====================================================================
struct AppSettings {
  uint8_t targetPrefix[2];  // préfixe de clé publique du répéteur cible
  int8_t txPowerDbm;        // puissance d'émission "à l'antenne"
  RxGainMode rxGainMode;
  RssiDisplayMode rssiDisplay;
};

bool settingsEqual(const AppSettings &a, const AppSettings &b);

// false si aucune config valide n'est stockée (premier démarrage,
// version incompatible) : l'appelant part alors des défauts d'usine.
bool settingsLoad(AppSettings &out);
void settingsSave(const AppSettings &s);
