#pragma once
#include <stdint.h>

// Chaine de gain en reception. Les modes sont exclusifs.
enum class RxGainMode : uint8_t {
  kNone = 0,     // tout coupe
  kSxBoost = 1,  // "RX boosted gain" interne du SX126x (~+2 dB) — defaut
  kFemLna = 2,   // LNA du FEM externe (Heltec V4.3 uniquement)
};
