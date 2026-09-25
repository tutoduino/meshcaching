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
    s.raw = s.stable = s.longFired = false;
    s.lastEdgeMs = millis();
    s.pressedAtMs = 0;
  }
}

void Buttons::push(const ButtonEvent &event) {
  if (_queueCount == kQueueSize) {
    return;  // overflow: the oldest events win, the newest is dropped
  }
  _queue[(_queueHead + _queueCount) % kQueueSize] = event;
  _queueCount++;
}

bool Buttons::poll(ButtonEvent &event) {
  service();
  if (_queueCount == 0) {
    return false;
  }
  event = _queue[_queueHead];
  _queueHead = (_queueHead + 1) % kQueueSize;
  _queueCount--;
  return true;
}

void Buttons::service() {
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
        s.pressedAtMs = now;
        s.longFired = false;
      } else if (!s.longFired) {
        push({s.spec.key, false});  // short click, emitted on release
      }
    }
    if (s.stable && !s.longFired && now - s.pressedAtMs >= kLongPressMs) {
      s.longFired = true;
      push({s.spec.key, true});
    }
  }
}
