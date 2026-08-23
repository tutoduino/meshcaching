#pragma once
#include <stdint.h>

#include "../hal/Board.h"
#include "../hal/Buttons.h"
#include "../hal/Radio.h"
#include "../ui/StatusScreen.h"

// =====================================================================
// Application : geolocalisation d'un repeteur MeshCore.
//
// Affiche le RSSI et le temps ecoule depuis la derniere reception d'un
// paquet provenant du repeteur cible (config::kTargetPubkeyPrefix), et
// permet de le "pinger" avec un paquet TRACE via le bouton Ok.
// =====================================================================
class App {
public:
  explicit App(Board &board);

  void setup();
  void loop();

private:
  // Dernier paquet vu du repeteur cible
  struct RepeaterStatus {
    bool hasPacket = false;
    uint32_t lastSeenMs = 0;
    float rssi = 0;
    float snr = 0;
  };

  void refreshDisplay();
  void handleKey(Key key);
  void sendTracePing();
  void handleIncomingPacket();
  bool packetComesFromTarget(const uint8_t *packet, size_t len);

  Board &_board;
  Radio _radio;
  Buttons _buttons;
  StatusScreen _screen;

  RepeaterStatus _target;
  // Tag de la derniere requete TRACE envoyee, et instant d'envoi :
  // permet de reconnaitre la reponse (qui renvoie ce meme tag).
  uint32_t _lastSentTag = 0;
  uint32_t _lastPingMs = 0;
  uint32_t _lastDisplayRefreshMs = 0;
};
