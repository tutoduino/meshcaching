#pragma once
#include <stdint.h>

// Chaîne de gain en réception. Les modes sont exclusifs.
enum class RxGainMode : uint8_t {
  kNone = 0,     // tout coupé
  kSxBoost = 1,  // "RX boosted gain" interne du SX126x (~+2 dB) — défaut
  kFemLna = 2,   // LNA du FEM externe (Heltec V4.3 uniquement)
};
