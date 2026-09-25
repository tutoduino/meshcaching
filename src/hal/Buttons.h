#pragma once
#include <Arduino.h>

#include "Board.h"

struct ButtonEvent {
  Key key;
  bool longPress;  // held for kLongPressMs (emitted without waiting for release)
};

// Debounced reading of the buttons declared by the board, turned into
// logical key events. A short click is emitted on release, a long press
// as soon as the threshold is reached (never both).
class Buttons {
public:
  void begin(const ButtonSpec *specs, size_t count);

  // Samples the pins, updates the debounced state and queues the
  // resulting events. Called by poll(); also usable from a display idle
  // hook so that a slow panel refresh does not swallow a click.
  void service();

  // true when an event is available; call on every loop()
  bool poll(ButtonEvent &event);

private:
  static constexpr size_t kMaxButtons = 8;
  static constexpr uint32_t kDebounceMs = 30;
  static constexpr uint32_t kLongPressMs = 600;

  struct State {
    ButtonSpec spec;
    bool raw;        // last raw reading
    bool stable;     // debounced state (true = pressed)
    bool longFired;  // long press of the current hold has been emitted
    uint32_t lastEdgeMs;
    uint32_t pressedAtMs;
  };

  static constexpr size_t kQueueSize = 4;

  bool readPressed(const ButtonSpec &spec) const;
  void push(const ButtonEvent &event);

  State _states[kMaxButtons];
  size_t _count = 0;
  ButtonEvent _queue[kQueueSize];
  uint8_t _queueHead = 0;
  uint8_t _queueCount = 0;
};
