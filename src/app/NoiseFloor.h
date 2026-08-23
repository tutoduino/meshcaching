#pragma once
#include <stddef.h>

// Mesure du bruit de fond par cycles de 64 échantillons de RSSI
// instantané : à chaque cycle complet, la médiane devient la valeur
// courante et le cycle suivant démarre aussitôt (évaluation continue).
// La médiane rejette naturellement les échantillons pris pendant le
// passage d'un paquet, sans avoir à les détecter.
class NoiseFloor {
public:
  void addSample(float rssiDbm);

  bool hasValue() const { return _hasValue; }
  float valueDbm() const { return _medianDbm; }

private:
  static constexpr size_t kSamplesPerCycle = 64;

  float _samples[kSamplesPerCycle];
  size_t _count = 0;
  float _medianDbm = 0;
  bool _hasValue = false;
};
