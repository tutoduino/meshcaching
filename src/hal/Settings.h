#pragma once
#include <stdint.h>

#include "RxGain.h"

// =====================================================================
// Persisted application configuration, editable from the menu.
// Storage: NVS (Preferences) on ESP32, internal LittleFS on nRF52.
// Factory defaults are composed by the application (AppConfig + Board).
// =====================================================================
struct AppSettings {
  uint8_t targetPrefix[2];  // public key prefix of the target repeater
  int8_t txPowerDbm;        // transmit power "at the antenna"
  RxGainMode rxGainMode;
  bool autoPing;            // automatic TRACE ping every 10 s
};

bool settingsEqual(const AppSettings &a, const AppSettings &b);

// false when no valid config is stored (first boot, incompatible
// version): the caller then starts from the factory defaults.
bool settingsLoad(AppSettings &out);
void settingsSave(const AppSettings &s);