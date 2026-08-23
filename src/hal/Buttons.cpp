#include "Buttons.h"

bool Buttons::readPressed(const ButtonSpec &spec) const {
  int level = digitalRead(spec.pin);
  return spec.activeLow ? (level == LOW) : (level == HIGH);
}

void Buttons::begin(const ButtonSpec *specs, size_t count) {
  _count = min(count, kMaxButtons);
  for (size_t i = 0; i < _count; i++) {
    State &s = _states[i];
    s.spec = specs[i];
    pinMode(s.spec.pin, s.spec.internalPullup ? INPUT_PULLUP : INPUT);
    s.raw = s.stable = false;
    s.lastEdgeMs = millis();
  }
}

bool Buttons::poll(Key &key) {
  uint32_t now = millis();
  for (size_t i = 0; i < _count; i++) {
    State &s = _states[i];
    bool pressed = readPressed(s.spec);
    if (pressed != s.raw) {
      s.raw = pressed;
      s.lastEdgeMs = now;
    }
    if (s.raw != s.stable && now - s.lastEdgeMs >= kDebounceMs) {
      s.stable = s.raw;
      if (s.stable) {
        key = s.spec.key;
        return true;
      }
    }
  }
  return false;
}
