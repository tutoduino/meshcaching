#pragma once
#include <stdint.h>

#include "../hal/Board.h"
#include "../hal/Buttons.h"
#include "../hal/Radio.h"
#include "../hal/Settings.h"
#include "../ui/SettingsMenu.h"
#include "../ui/StatusScreen.h"
#include "NoiseFloor.h"

// =====================================================================
// Application: geolocating a MeshCore repeater.
//
// Displays the RSSI and the time elapsed since the last packet received
// from the target repeater, allows "pinging" it with a TRACE packet,
// and offers a menu of persisted settings (target repeater, TX power,
// RX gain).
// =====================================================================
class App {
public:
  explicit App(Board &board);

  void setup();
  void loop();

private:
  // Last packet seen from the target repeater
  struct RepeaterStatus {
    bool hasPacket = false;
    uint32_t lastSeenMs = 0;
    float rssi = 0;          // RssiPkt, averaged over the packet
    float despreadRssi = 0;  // SignalRssiPkt, after despreading
    float snr = 0;
  };

  void loadSettings();
  void applyMenuResult();
  void refreshDisplay();
  void handleMainEvent(const ButtonEvent &event);
  // Display idle hook (slow panels): keeps the buttons serviced while
  // the panel refreshes.
  static void onDisplayIdle(void *self);
  void sendTracePing();
  void handleIncomingPacket();
  bool packetComesFromTarget(const uint8_t *packet, size_t len);

  Board &_board;
  Radio _radio;
  Buttons _buttons;
  StatusScreen _screen;
  SettingsMenu _menu;

  // Transmit sequence: LBT running, transmission done, or channel left
  // busy (aborted) - drives the indicator at the top of the screen.
  enum class TxPhase : uint8_t { Idle, Lbt, Tx, Busy };

  AppSettings _settings{};
  RepeaterStatus _target;
  NoiseFloor _noise;
  // Tag of the last TRACE request sent, and the time it was sent:
  // lets us recognize the reply (which echoes that same tag), and
  // anchors the transmit cooldown (kTxCooldownMs).
  uint32_t _lastSentTag = 0;
  uint32_t _lastPingMs = 0;
  bool _hasPinged = false;
  TxPhase _txPhase = TxPhase::Idle;
  uint32_t _txPhaseSinceMs = 0;
  // Start of the blink triggered by a valid reply
  uint32_t _rxFlashStartMs = 0;
  uint32_t _lastNoiseSampleMs = 0;
  uint32_t _lastDisplayRefreshMs = 0;
};
