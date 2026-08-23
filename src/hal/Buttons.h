#pragma once
#include <Arduino.h>

#include "Board.h"

struct ButtonEvent {
  Key key;
  bool longPress;  // appui maintenu kLongPressMs (emis sans attendre le relacher)
};

// Lecture debouncee des boutons declares par la carte, convertis en
// evenements de touches logiques. Un clic court est emis au relacher,
// un appui long des que le seuil est atteint (jamais les deux).
class Buttons {
public:
  void begin(const ButtonSpec *specs, size_t count);

  // true si un evenement est disponible ; a appeler a chaque loop()
  bool poll(ButtonEvent &event);

private:
  static constexpr size_t kMaxButtons = 8;
  static constexpr uint32_t kDebounceMs = 30;
  static constexpr uint32_t kLongPressMs = 600;

  struct State {
    ButtonSpec spec;
    bool raw;        // derniere lecture brute
    bool stable;     // etat debounce (true = presse)
    bool longFired;  // l'appui long de la pression en cours a ete emis
    uint32_t lastEdgeMs;
    uint32_t pressedAtMs;
  };

  bool readPressed(const ButtonSpec &spec) const;

  State _states[kMaxButtons];
  size_t _count = 0;
};
