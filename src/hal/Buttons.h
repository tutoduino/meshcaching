#pragma once
#include <Arduino.h>

#include "Board.h"

struct ButtonEvent {
  Key key;
  bool longPress;  // appui maintenu kLongPressMs (émis sans attendre le relâcher)
};

// Lecture débouncée des boutons déclarés par la carte, convertis en
// événements de touches logiques. Un clic court est émis au relâcher,
// un appui long dès que le seuil est atteint (jamais les deux).
class Buttons {
public:
  void begin(const ButtonSpec *specs, size_t count);

  // true si un événement est disponible ; à appeler à chaque loop()
  bool poll(ButtonEvent &event);

private:
  static constexpr size_t kMaxButtons = 8;
  static constexpr uint32_t kDebounceMs = 30;
  static constexpr uint32_t kLongPressMs = 600;

  struct State {
    ButtonSpec spec;
    bool raw;        // dernière lecture brute
    bool stable;     // état débouncé (true = pressé)
    bool longFired;  // l'appui long de la pression en cours a été émis
    uint32_t lastEdgeMs;
    uint32_t pressedAtMs;
  };

  bool readPressed(const ButtonSpec &spec) const;

  State _states[kMaxButtons];
  size_t _count = 0;
};
