#pragma once
#include <stdint.h>

#include "../hal/Board.h"
#include "../hal/Buttons.h"
#include "../hal/Radio.h"
#include "../hal/Settings.h"
#include "../ui/SettingsMenu.h"
#include "../ui/StatusScreen.h"

// =====================================================================
// Application : géolocalisation d'un répéteur MeshCore.
//
// Affiche le RSSI et le temps écoulé depuis la dernière réception d'un
// paquet provenant du répéteur cible, permet de le "pinger" avec un
// paquet TRACE, et propose un menu de réglages persistés (répéteur
// cible, puissance TX, gain RX).
// =====================================================================
class App {
public:
  explicit App(Board &board);

  void setup();
  void loop();

private:
  // Dernier paquet vu du répéteur cible
  struct RepeaterStatus {
    bool hasPacket = false;
    uint32_t lastSeenMs = 0;
    float rssi = 0;
    float snr = 0;
  };

  void loadSettings();
  void applyMenuResult();
  void refreshDisplay();
  void handleMainEvent(const ButtonEvent &event);
  void sendTracePing();
  void handleIncomingPacket();
  bool packetComesFromTarget(const uint8_t *packet, size_t len);

  Board &_board;
  Radio _radio;
  Buttons _buttons;
  StatusScreen _screen;
  SettingsMenu _menu;

  AppSettings _settings{};
  RepeaterStatus _target;
  // Tag de la dernière requête TRACE envoyée, et instant d'envoi :
  // permet de reconnaître la réponse (qui renvoie ce même tag), et
  // sert d'ancre au réarmement de l'émission (kTxCooldownMs).
  uint32_t _lastSentTag = 0;
  uint32_t _lastPingMs = 0;
  bool _hasPinged = false;
  // Départ du clignotement declenché par une réponse valide
  uint32_t _rxFlashStartMs = 0;
  uint32_t _lastDisplayRefreshMs = 0;
};
