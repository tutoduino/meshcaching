#include "App.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../AppConfig.h"
#include "../hal/SysRandom.h"
#include "../mesh/Protocol.h"

// Formate un float avec une décimale sans dépendre du %f de printf,
// qui n'est pas fiable sur toutes les plateformes (nRF52 notamment).
static void formatDb(float value, char *out, size_t outLen) {
  int v10 = (int)lroundf(value * 10.0f);
  snprintf(out, outLen, "%s%d.%c", v10 < 0 ? "-" : "", abs(v10) / 10,
           (char)('0' + abs(v10) % 10));
}

App::App(Board &board)
    : _board(board),
      _radio(board),
      _screen(board.display()),
      _menu(board.display(), board) {}

void App::loadSettings() {
  if (!settingsLoad(_settings)) {
    // Premier démarrage (ou format incompatible) : défauts d'usine
    memcpy(_settings.targetPrefix, config::kTargetPubkeyPrefix,
           sizeof(_settings.targetPrefix));
    _settings.txPowerDbm = _board.txPowerDefaultDbm();
    _settings.rxGainMode = RxGainMode::kSxBoost;
  }
  // Garde-fous, notamment si la config vient d'une autre carte
  if (_settings.txPowerDbm > _board.txPowerMaxDbm()) {
    _settings.txPowerDbm = _board.txPowerMaxDbm();
  }
  if (_settings.txPowerDbm < kTxPowerMinDbm) {
    _settings.txPowerDbm = kTxPowerMinDbm;
  }
  if (_settings.rxGainMode == RxGainMode::kFemLna && !_board.hasFemLna()) {
    _settings.rxGainMode = RxGainMode::kSxBoost;
  }
}

void App::setup() {
  Serial.begin(115200);
  delay(200);

  Serial.printf("Carte : %s\n", _board.name());

  _board.initPower();
  _board.beginDisplay();
  _screen.showMessage("Initialisation...");

  size_t buttonCount = 0;
  const ButtonSpec *specs = _board.buttons(buttonCount);
  _buttons.begin(specs, buttonCount);

  loadSettings();
  Serial.printf("Répéteur cible : %02X%02X\n", _settings.targetPrefix[0],
                _settings.targetPrefix[1]);

  Serial.println(F("Initialisation LoRa..."));
  int16_t state = _radio.begin(config::kLoraFreqMhz, config::kLoraBwKhz,
                               config::kLoraSf, config::kLoraCr,
                               _settings.txPowerDbm);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Erreur LoRa : "));
    Serial.println(state);
    char msg[16];
    snprintf(msg, sizeof(msg), "%d", state);
    _screen.showMessage("Erreur LoRa", msg);
    while (true) {}  // sans radio, pas la peine de continuer
  }
  _radio.setRxGainMode(_settings.rxGainMode);
  Serial.printf("Puissance TX : %d dBm, gain RX : %u\n", _radio.txPowerDbm(),
                (unsigned)_settings.rxGainMode);

  Serial.println(F("En attente de paquets MeshCore..."));
  _screen.showMessage("En attente de", "paquets MeshCore...");
}

void App::loop() {
  ButtonEvent event;
  if (_buttons.poll(event)) {
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

  // Rafraîchit l'écran principal une fois par seconde (compteur "temps
  // écoulé") — jamais par-dessus le menu
  if (!_menu.isOpen() &&
      millis() - _lastDisplayRefreshMs >= config::kDisplayRefreshMs) {
    _lastDisplayRefreshMs = millis();
    refreshDisplay();
  }

  if (_radio.packetAvailable()) {
    handleIncomingPacket();
  }
}

void App::handleMainEvent(const ButtonEvent &event) {
  if (event.key == Key::Ok && !event.longPress) {
    // Ping TRACE forcé vers le répéteur cible, au lieu d'attendre
    // passivement son prochain paquet
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
  _settings = updated;

  _radio.setTxPowerDbm(_settings.txPowerDbm);
  _radio.setRxGainMode(_settings.rxGainMode);
  if (targetChanged) {
    // Nouveau répéteur suivi : on repart de zéro
    _target = RepeaterStatus();
    _lastSentTag = 0;
  }
  if (changed) {
    settingsSave(_settings);
    Serial.printf("Config sauvegardée : cible=%02X%02X TX=%ddBm gainRX=%u\n",
                  _settings.targetPrefix[0], _settings.targetPrefix[1],
                  _settings.txPowerDbm, (unsigned)_settings.rxGainMode);
  }

  refreshDisplay();
  _lastDisplayRefreshMs = millis();
}

void App::refreshDisplay() {
  if (!_target.hasPacket) {
    _screen.showMessage("En attente de", "paquets MeshCore...");
    return;
  }
  uint32_t ageSeconds = (millis() - _target.lastSeenMs) / 1000;
  _screen.drawStatus(_target.rssi, _target.snr, ageSeconds,
                     _settings.targetPrefix, sizeof(_settings.targetPrefix));
}

void App::sendTracePing() {
  uint8_t buf[meshcore::kTracePingLen];
  uint32_t tag = sysRandom32();  // identifiant aléatoire de cette requête
  size_t len =
      meshcore::buildTracePing(buf, tag, _settings.targetPrefix[0]);

  _lastSentTag = tag;  // on retiendra ce tag pour reconnaître la réponse
  _lastPingMs = millis();

  Serial.printf("Envoi TRACE (tag=%08lX) vers RÉPÉTEUR %02X...\n",
                (unsigned long)tag, _settings.targetPrefix[0]);
  int16_t state = _radio.transmit(buf, len);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Erreur d'émission : "));
    Serial.println(state);
  }
}

bool App::packetComesFromTarget(const uint8_t *packet, size_t len) {
  meshcore::PacketView pkt;
  if (!meshcore::parse(packet, len, pkt)) {
    return false;
  }

  // Une réponse TRACE ne se reconnaît que par son tag, renvoyé tel quel :
  // on la compare à notre dernier ping, dans la fenêtre de temps admise.
  if (pkt.payloadType == meshcore::kPayloadTrace) {
    uint32_t tag;
    if (!meshcore::traceTag(pkt, tag) || _lastSentTag == 0) {
      return false;
    }
    if (millis() - _lastPingMs > config::kTraceReplyTimeoutMs) {
      return false;
    }
    return tag == _lastSentTag;
  }

  // Sinon : l'identifiant du dernier nœud émetteur, comparé au préfixe
  // de clé publique du répéteur cible.
  const uint8_t *id = nullptr;
  size_t idLen = 0;
  if (!meshcore::lastHopId(pkt, id, idLen)) {
    return false;
  }
  size_t compareLen = min(idLen, sizeof(_settings.targetPrefix));
  return memcmp(id, _settings.targetPrefix, compareLen) == 0;
}

void App::handleIncomingPacket() {
  uint8_t buf[meshcore::kMaxPacketLen];
  size_t len = 0;
  float rssi = 0, snr = 0;

  int16_t state = _radio.readPacket(buf, sizeof(buf), len, rssi, snr);
  if (len == 0) {
    return;
  }
  // On ignore les paquets illisibles OU dont le CRC est invalide : un
  // paquet corrompu ne doit jamais être interprété comme venant du
  // répéteur cible (risque de fausse détection).
  if (state != RADIOLIB_ERR_NONE) {
    if (state != RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.print(F("Erreur de réception : "));
      Serial.println(state);
    }
    return;
  }

  bool isTarget = packetComesFromTarget(buf, len);
  char rssiStr[16], snrStr[16];
  formatDb(rssi, rssiStr, sizeof(rssiStr));
  formatDb(snr, snrStr, sizeof(snrStr));
  Serial.printf("Paquet reçu : len=%u RSSI=%s dBm SNR=%s dB %s\n",
                (unsigned)len, rssiStr, snrStr,
                isTarget ? "[RÉPÉTEUR CIBLE]" : "");

  if (isTarget) {
    _target.hasPacket = true;
    _target.lastSeenMs = millis();
    _target.rssi = rssi;
    _target.snr = snr;
    if (!_menu.isOpen()) {
      refreshDisplay();  // mise à jour immédiate de l'écran
    }
  }
}
