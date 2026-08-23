#include "NoiseFloor.h"

#include <algorithm>

void NoiseFloor::addSample(float rssiDbm) {
  _samples[_count++] = rssiDbm;
  if (_count < kSamplesPerCycle) {
    return;
  }
  std::sort(_samples, _samples + kSamplesPerCycle);
  _medianDbm = (_samples[kSamplesPerCycle / 2 - 1] +
                _samples[kSamplesPerCycle / 2]) / 2.0f;
  _hasValue = true;
  _count = 0;  // cycle suivant, sans pause
}
