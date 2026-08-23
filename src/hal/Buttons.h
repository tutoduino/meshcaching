#pragma once
#include <Arduino.h>

#include "Board.h"

// Lecture debouncee des boutons declares par la carte, convertis en
// touches logiques. Une pression physique = un evenement (front d'appui).
// (Appuis longs et repetition viendront avec le systeme de menu.)
class Buttons {
public:
  void begin(const ButtonSpec *specs, size_t count);

  // true si une touche vient d'etre pressee ; a appeler a chaque loop()
  bool poll(Key &key);

private:
  static constexpr size_t kMaxButtons = 8;
  static constexpr uint32_t kDebounceMs = 30;

  struct State {
    ButtonSpec spec;
    bool raw;        // derniere lecture brute
    bool stable;     // etat debounce (true = presse)
    uint32_t lastEdgeMs;
  };

  bool readPressed(const ButtonSpec &spec) const;

  State _states[kMaxButtons];
  size_t _count = 0;
};
