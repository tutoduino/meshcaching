#include "App.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../AppConfig.h"
#include "../hal/SysRandom.h"
#include "../mesh/Protocol.h"

// Formats a float with one decimal without relying on printf's %f,
// which is not reliable on every platform (nRF52 in particular).
static void formatDb(float value, char *out, size_t outLen) {
  int v10 = (int)lroundf(value * 10.0f);
  snprintf(out, outLen, "%s%d.%c", v10 < 0 ? "-" : "", abs(v10) / 10,
           (char)('0' + abs(v10) % 10));
}

// Period of the automatic TRACE ping. Must be >= config::kTxCooldownMs,
// otherwise sendTracePing() refuses some pings because of the cooldown.
static constexpr uint32_t kAutoPingIntervalMs = 10000;

App::App(Board &board)
    : _board(board),
      _radio(board),
      _screen(board.display()),
      _menu(board.display(), board) {}

void App::loadSettings() {
  if (!settingsLoad(_settings)) {
    // First boot (or incompatible format): factory defaults
    memcpy(_settings.targetPrefix, config::kTargetPubkeyPrefix,
           sizeof(_settings.targetPrefix));
    _settings.txPowerDbm = _board.txPowerDefaultDbm();
    _settings.rxGainMode = RxGainMode::kSxBoost;
    _settings.autoPing = false;
  }
  // Safety clamps, in particular if the config comes from another board
  if (_settings.txPowerDbm > _board.txPowerMaxDbm()) {
    _settings.txPowerDbm = _board.txPowerMaxDbm();
  }
  if (_settings.txPowerDbm < _board.txPowerMinDbm()) {
    _settings.txPowerDbm = _board.txPowerMinDbm();
  }
  if (_settings.rxGainMode == RxGainMode::kFemLna && !_board.hasFemLna()) {
    _settings.rxGainMode = RxGainMode::kSxBoost;
  }
}

void App::setup() {
  Serial.begin(115200);
  delay(200);

  Serial.printf("MeshCaching %s - board: %s\n", MESHCACHING_VERSION,
                _board.name());

  _board.initPower();
  _board.beginDisplay();
  uint32_t splashStartMs = millis();
  _screen.showSplash(MESHCACHING_VERSION);

  // Unexpected board (e.g. a Heltec V4.2 flashed with the V4.3 build):
  // stop before touching the radio.
  if (const char *err = _board.selfCheckError()) {
    Serial.print(F("Incompatible board: "));
    Serial.println(err);
    _screen.showMessage("Carte incompatible", err);
    while (true) {}
  }

  size_t buttonCount = 0;
  const ButtonSpec *specs = _board.buttons(buttonCount);
  _buttons.begin(specs, buttonCount);
  _board.display().setIdleHook(&App::onDisplayIdle, this);

  loadSettings();
  Serial.printf("Target repeater: %02X%02X\n", _settings.targetPrefix[0],
                _settings.targetPrefix[1]);

  Serial.println(F("Initializing LoRa..."));
  int16_t state = _radio.begin(config::kLoraFreqMhz, config::kLoraBwKhz,
                               config::kLoraSf, config::kLoraCr,
                               _settings.txPowerDbm);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("LoRa error: "));
    Serial.println(state);
    char msg[16];
    snprintf(msg, sizeof(msg), "%d", state);
    _screen.showMessage("Erreur LoRa", msg);
    while (true) {}  // without a radio, no point going any further
  }
  _radio.setRxGainMode(_settings.rxGainMode);
  Serial.printf("TX power: %d dBm, RX gain: %u\n", _radio.txPowerDbm(),
                (unsigned)_settings.rxGainMode);

  Serial.println(F("Waiting for MeshCore packets..."));
  // Keep the splash screen visible for the intended time - the radio
  // and config init ran meanwhile - then go straight to the main
  // screen (sleep logo).
  while (millis() - splashStartMs < config::kSplashMs) {
    delay(10);
  }
  refreshDisplay();
}

void App::loop() {

  // Poll standard GPIO buttons first. If no standard button event occurred,
  // fall back to the board's advanced input polling (e.g., trackball, I2C keyboard).
  ButtonEvent event;
  bool haveEvent = _buttons.poll(event);
  if (!haveEvent) {
    InputEvent input;
    if (_board.pollInput(input)) {
      event = {input.key, input.longPress};
      haveEvent = true;
    }
  }
  if (haveEvent) {
    if (_menu.isOpen()) {
      if (_menu.handleEvent(event)) {
        applyMenuResult();
      }
    } else {
      handleMainEvent(event);
    }
  }
  if (_menu.isOpen() && _menu.tickTimeout()) {
    applyMenuResult();
  }

  // Noise floor: continuous sampling of the instantaneous RSSI - the
  // radio stays in listen mode, the read is non-intrusive. Packets going
  // through pollute a few samples, the median rejects them.
  if (millis() - _lastNoiseSampleMs >= config::kNoiseSampleIntervalMs) {
    _lastNoiseSampleMs = millis();
    _noise.addSample(_radio.rssiInstant());
  }

  // Refresh the main screen (animations, cooldown bar) - never on top
  // of the menu. Slow panels (e-ink) impose their own, longer cadence.
  uint32_t refreshMs = config::kDisplayRefreshMs;
  if (_board.display().minFrameIntervalMs() > refreshMs) {
    refreshMs = _board.display().minFrameIntervalMs();
  }
  if (!_menu.isOpen() && millis() - _lastDisplayRefreshMs >= refreshMs) {
    _lastDisplayRefreshMs = millis();
    refreshDisplay();
  }

  // Automatic TRACE ping. The deadline is advanced before sending: if the
  // channel is busy (LBT aborted), we retry at the next period instead of
  // looping continuously.
  if (_settings.autoPing && !_menu.isOpen() &&
      (int32_t)(millis() - _nextAutoPingMs) >= 0) {
    _nextAutoPingMs = millis() + kAutoPingIntervalMs;
    sendTracePing();
  }

  handleIncomingPackets();
}

void App::onDisplayIdle(void *self) {
  App &app = *static_cast<App *>(self);
  app._buttons.service();
  app.pumpRadio();
}

void App::handleMainEvent(const ButtonEvent &event) {
  if (event.key == Key::Ok && !event.longPress) {
    // Forced TRACE ping to the target repeater, instead of passively
    // waiting for its next packet
    sendTracePing();
  } else if ((event.key == Key::Ok && event.longPress) ||
             (event.key == Key::Back && !event.longPress)) {
    _menu.open(_settings);
  }
}

void App::applyMenuResult() {
  const AppSettings &updated = _menu.result();
  bool changed = !settingsEqual(updated, _settings);
  bool targetChanged = memcmp(updated.targetPrefix, _settings.targetPrefix,
                              sizeof(_settings.targetPrefix)) != 0;
  bool wasAutoPing = _settings.autoPing;
  _settings = updated;

  _radio.setTxPowerDbm(_settings.txPowerDbm);
  _radio.setRxGainMode(_settings.rxGainMode);
  if (targetChanged) {
    // New repeater tracked: start over from scratch
    _target = RepeaterStatus();
    _lastSentTag = 0;
  }
  if (_settings.autoPing && !wasAutoPing) {
    _nextAutoPingMs = millis();  // auto ping just enabled: first ping now
  }
  if (changed) {
    settingsSave(_settings);
    Serial.printf("Settings saved: target=%02X%02X TX=%ddBm rxGain=%u auto=%d\n",
                  _settings.targetPrefix[0], _settings.targetPrefix[1],
                  _settings.txPowerDbm, (unsigned)_settings.rxGainMode,
                  (int)_settings.autoPing);
  }

  refreshDisplay();
  _lastDisplayRefreshMs = millis();
}

void App::refreshDisplay() {
  uint32_t now = millis();
  MainView view;
  view.pubkeyPrefix = _settings.targetPrefix;
  view.prefixLen = sizeof(_settings.targetPrefix);
  // The last values stay on screen until the next packet; the sleep
  // logo only shows up before the very first reception.
  view.rssiValid = _target.hasPacket;
  view.rssi = _target.rssi;
  view.despreadRssi = _target.despreadRssi;
  view.autoPing = _settings.autoPing;
  view.snr = _target.snr;
  view.txBadge = nullptr;
  switch (_txPhase) {
    case TxPhase::Lbt:
      view.txBadge = "LBT";
      break;
    case TxPhase::Tx:
      if (now - _txPhaseSinceMs < config::kTxIndicatorMs) {
        view.txBadge = "TX";
      } else {
        _txPhase = TxPhase::Idle;
      }
      break;
    case TxPhase::Busy:
      if (now - _txPhaseSinceMs < config::kLbtBusyMsgMs) {
        view.txBadge = "OCCUPÉ";
      } else {
        _txPhase = TxPhase::Idle;
      }
      break;
    default:
      break;
  }
  view.noiseValid = _noise.hasValue();
  view.noiseDbm = _noise.valueDbm();
  view.invert = _target.hasPacket && now - _rxFlashStartMs < config::kRxFlashMs;
  uint32_t sincePing = now - _lastPingMs;
  view.cooldownTotalMs = config::kTxCooldownMs;
  if (_txPhase == TxPhase::Lbt) {
    // Bar full as soon as the transmit is ordered and frozen during the
    // LBT; the decay only starts at the actual transmission (anchor
    // _lastPingMs, set after the LBT). An abort does not arm it: it just
    // disappears.
    view.cooldownRemainingMs = config::kTxCooldownMs;
  } else {
    view.cooldownRemainingMs = (_hasPinged && sincePing < config::kTxCooldownMs)
                                   ? config::kTxCooldownMs - sincePing
                                   : 0;
  }
  _screen.drawMain(view);
}

void App::sendTracePing() {
  uint32_t now = millis();
  if (_hasPinged && now - _lastPingMs < config::kTxCooldownMs) {
    return;  // cooldown running: no more than one transmit per period
  }

  // Slow panels (e-ink): a refresh blocks for ~0.5 s, so nothing is
  // drawn before the transmission - neither the LBT badge, which would
  // delay the listen, nor the TX badge, which would open a gap between
  // the "channel clear" verdict and the actual send. A single refresh
  // follows the transmission, once the radio is listening again.
  const bool slowPanel = _board.display().minFrameIntervalMs() > 0;

  // LBT: transmit only if the channel is clear. Unlike MeshCore, no
  // forced TX at the deadline: we abort and show it.
  _txPhase = TxPhase::Lbt;
  _txPhaseSinceMs = now;
  if (!_menu.isOpen() && !slowPanel) {
    refreshDisplay();  // LBT indicator during the blocking listen
  }
  bool channelClear = false;
  for (;;) {
    // A real packet may have arrived during the backoff slot (the radio
    // stays in listen mode): handle it instead of losing it.
    handleIncomingPackets();
    if (_radio.channelClear()) {
      channelClear = true;
      break;
    }
    if (millis() - now >= config::kLbtDeadlineMs) {
      break;
    }
    delay(config::kLbtSlotMinMs +
          sysRandom32() %
              (config::kLbtSlotMaxMs - config::kLbtSlotMinMs + 1));
  }
  if (!channelClear) {
    _txPhase = TxPhase::Busy;
    _txPhaseSinceMs = millis();
    Serial.println(F("LBT: channel busy, transmission aborted"));
    if (!_menu.isOpen()) {
      refreshDisplay();
    }
    return;  // nothing was transmitted: no cooldown
  }

  uint8_t buf[meshcore::kTracePingLen];
  uint32_t tag = sysRandom32();  // random identifier for this request

  // Hash size used in the TRACE path (encoded in the flags byte): 2 bytes
  // of the target's public key prefix, to limit collisions with other repeaters.
  constexpr uint8_t kTraceHashSize = 2;
  static_assert(sizeof(_settings.targetPrefix) >= kTraceHashSize,
                "targetPrefix is too short for the trace hash size");

  size_t len = meshcore::buildTracePing(buf, tag, _settings.targetPrefix,
                                        kTraceHashSize);
  if (len == 0) {
    // Invalid hash size: nothing to send, no cooldown, back to idle
    Serial.println(F("buildTracePing: invalid hash size"));
    _txPhase = TxPhase::Idle;
    return;
  }

  _lastSentTag = tag;  // kept for the log only: the reply is no longer matched by tag
  _lastPingMs = millis();
  _hasPinged = true;
  _txPhase = TxPhase::Tx;
  _txPhaseSinceMs = _lastPingMs;
  if (!_menu.isOpen() && !slowPanel) {
    refreshDisplay();  // TX indicator and full bar, before the blocking send
  }

  Serial.printf("Sending TRACE (tag=%08lX) to REPEATER %02X%02X...\n",
                (unsigned long)tag, _settings.targetPrefix[0],
                _settings.targetPrefix[1]);

  int16_t state = _radio.transmit(buf, len);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Transmit error: "));
    Serial.println(state);
  }
  if (!_menu.isOpen() && slowPanel) {
    refreshDisplay();  // TX indicator and full bar, late by the air time
  }
}

bool App::packetComesFromTarget(const uint8_t *packet, size_t len) {
  if (len < 2) {
    return false;
  }

  uint8_t routeType   = packet[0] & 0x03;
  uint8_t payloadType = (packet[0] >> 2) & 0x0F;

  // Only direct-route TRACE packets are of interest
  if (payloadType != meshcore::kPayloadTrace ||
      routeType != meshcore::kRouteDirect) {
    return false;
  }

  // Number of SNR bytes in the path = hops already done.
  // 0 = original packet (sent by the node that started the trace), not a reply.
  uint8_t hopCount = packet[1] & 0x3F;
  if (hopCount == 0) {
    return false;
  }

  // The payload starts after the SNR bytes (1 byte per hop)
  size_t offset = 2 + hopCount;
  if (offset + 9 > len) {
    return false;
  }
  const uint8_t *payload = packet + offset;

  // TRACE payload: tag(4) + auth(4) + flags(1) + path hashes
  uint8_t hashSize = 1 << (payload[8] & 0x03);  // 1, 2, 4 or 8 bytes
  size_t hashesLen = len - offset - 9;
  if ((size_t)hopCount * hashSize > hashesLen) {
    return false;
  }

  // Last node that relayed = hash #(hopCount-1) of the traced path
  const uint8_t *lastHop = payload + 9 + (size_t)(hopCount - 1) * hashSize;

  size_t compareLen = min((size_t)hashSize, sizeof(_settings.targetPrefix));
  return memcmp(lastHop, _settings.targetPrefix, compareLen) == 0;
}

void App::pumpRadio() {
  if (!_radio.packetAvailable()) {
    return;
  }
  if (_rxQueueCount == kRxQueueSize) {
    // Queue full: read the packet anyway to free the radio, and drop the
    // oldest queued one - the newest is the most likely to be the reply
    // we are waiting for.
    _rxQueueHead = (_rxQueueHead + 1) % kRxQueueSize;
    _rxQueueCount--;
  }
  RxPacket &slot = _rxQueue[(_rxQueueHead + _rxQueueCount) % kRxQueueSize];
  slot.state = _radio.readPacket(slot.data, sizeof(slot.data), slot.len,
                                 slot.rssi, slot.snr, slot.despreadRssi);
  if (slot.len == 0) {
    return;  // empty or oversized packet: nothing to queue
  }
  _rxQueueCount++;
}

void App::handleIncomingPackets() {
  pumpRadio();
  while (_rxQueueCount > 0) {
    // Copy out first: processing may refresh a slow panel, whose idle
    // hook pumps the radio and can reuse this slot.
    RxPacket packet = _rxQueue[_rxQueueHead];
    _rxQueueHead = (_rxQueueHead + 1) % kRxQueueSize;
    _rxQueueCount--;
    processPacket(packet);
  }
}

void App::processPacket(const RxPacket &packet) {
  // Ignore packets that are unreadable OR whose CRC is invalid: a
  // corrupted packet must never be interpreted as coming from the
  // target repeater (risk of false detection).
  if (packet.state != RADIOLIB_ERR_NONE) {
    if (packet.state != RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.print(F("Receive error: "));
      Serial.println(packet.state);
    }
    return;
  }

  bool isTarget = packetComesFromTarget(packet.data, packet.len);
  char rssiStr[16], despreadStr[16], snrStr[16];
  formatDb(packet.rssi, rssiStr, sizeof(rssiStr));
  formatDb(packet.despreadRssi, despreadStr, sizeof(despreadStr));
  formatDb(packet.snr, snrStr, sizeof(snrStr));
  Serial.printf(
      "Packet received: len=%u RSSI=%s dBm despread=%s dBm SNR=%s dB %s\n",
      (unsigned)packet.len, rssiStr, despreadStr, snrStr,
      isTarget ? "[TARGET REPEATER]" : "");

  if (isTarget) {
    _target.hasPacket = true;
    _target.lastSeenMs = millis();
    _target.rssi = packet.rssi;
    _target.despreadRssi = packet.despreadRssi;
    _target.snr = packet.snr;
    _rxFlashStartMs = _target.lastSeenMs;  // triggers the blink
    if (!_menu.isOpen()) {
      refreshDisplay();  // immediate screen update
    }
  }
}