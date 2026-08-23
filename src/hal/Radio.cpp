#include "Radio.h"

#include <SPI.h>

#if defined(ESP32)
#define RADIO_ISR_ATTR IRAM_ATTR
#else
#define RADIO_ISR_ATTR
#endif

volatile bool Radio::s_packetFlag = false;

void RADIO_ISR_ATTR Radio::onDio1Isr() {
  s_packetFlag = true;
}

Radio::Radio(Board &board)
    : _board(board),
      _traits(board.radio()),
      _lora(new Module(_traits.pins.nss, _traits.pins.dio1,
                       _traits.pins.reset, _traits.pins.busy)) {}

int8_t Radio::chipPowerDbm(int8_t antennaDbm) const {
  int chip = antennaDbm - _traits.femTxGainDb;
  return (int8_t)constrain(chip, kTxPowerMinDbm, 22);  // plage du SX1262
}

void Radio::setTxPowerDbm(int8_t antennaDbm) {
  _antennaDbm = (int8_t)constrain(antennaDbm, _board.txPowerMinDbm(),
                                  _board.txPowerMaxDbm());
  _lora.setOutputPower(chipPowerDbm(_antennaDbm));
}

void Radio::setRxGainMode(RxGainMode mode) {
  if (mode == RxGainMode::kFemLna && !_board.hasFemLna()) {
    mode = RxGainMode::kSxBoost;
  }
  _board.setFemLna(mode == RxGainMode::kFemLna);
  _lora.setRxBoostedGainMode(mode == RxGainMode::kSxBoost);
  startReceive();  // réapplique l'aiguillage RX du FEM
}

int16_t Radio::begin(float freqMhz, float bwKhz, uint8_t sf, uint8_t cr,
                     int8_t txPowerDbm) {
#if defined(ESP32)
  if (_traits.pins.sck >= 0) {
    SPI.begin(_traits.pins.sck, _traits.pins.miso, _traits.pins.mosi,
              (int8_t)_traits.pins.nss);
  } else {
    SPI.begin();
  }
#else
  SPI.begin();  // broches fixées par la variante
#endif

  _antennaDbm = (int8_t)constrain(txPowerDbm, _board.txPowerMinDbm(),
                                  _board.txPowerMaxDbm());
  int16_t state = _lora.begin(freqMhz, bwKhz, sf, cr,
                              RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                              chipPowerDbm(_antennaDbm), 8,
                              _traits.tcxoVoltage);
  if (state != RADIOLIB_ERR_NONE) {
    return state;
  }

  if (_traits.dio2AsRfSwitch) {
    _lora.setDio2AsRfSwitch(true);
  }
  if (_traits.pins.rxEn != RADIOLIB_NC || _traits.pins.txEn != RADIOLIB_NC) {
    _lora.setRfSwitchPins(_traits.pins.rxEn, _traits.pins.txEn);
  }
  _lora.setCurrentLimit(_traits.currentLimitmA);
  // La chaîne de gain RX (boost SX126x / LNA FEM) est appliquée ensuite
  // par l'application via setRxGainMode(), selon la config persistée.

  if (_traits.femRxPatch) {
    // Registre 0x8B5 non documenté : "improved RX" pour les Heltec V4 à
    // FEM, en parité avec le firmware MeshCore (recette Heltec). Sans
    // lecture valide, on n'écrit pas : écrire 0x01 seul écraserait les
    // autres bits du registre.
    uint8_t value = 0;
    if (_lora.readRegister(0x8B5, &value, 1) == RADIOLIB_ERR_NONE) {
      value |= 0x01;
      _lora.writeRegister(0x8B5, &value, 1);
    } else {
      Serial.println(F("Patch RX 0x8B5 non appliqué (lecture échouée)"));
    }
  }

  _lora.setDio1Action(onDio1Isr);
  return startReceive();
}

bool Radio::channelClear() {
  // Une erreur de scan est traitée comme un canal occupé.
  int16_t state = _lora.scanChannel();
  // Le CAD-done vient de lever DIO1 : l'effacer maintenant, puis
  // repasser en écoute. Un RX-done ultérieur (paquet reçu pendant que
  // l'appelant patiente) relèvera le drapeau et sera vu par
  // packetAvailable() avant le cycle de CAD suivant.
  s_packetFlag = false;
  startReceive();
  return state == RADIOLIB_CHANNEL_FREE;
}

int16_t Radio::transmit(const uint8_t *data, size_t len) {
  _board.radioTxMode();
  int16_t state = _lora.transmit(data, len);
  // transmit() déclenche aussi une interruption DIO1 de "fin d'émission",
  // qui peut avoir armé le drapeau à tort : on l'efface avant de repasser
  // en écoute, pour ne pas traiter du vide comme un paquet reçu.
  s_packetFlag = false;
  startReceive();
  return state;
}

bool Radio::packetAvailable() {
  if (!s_packetFlag) {
    return false;
  }
  s_packetFlag = false;
  return true;
}

int16_t Radio::readPacket(uint8_t *buf, size_t maxLen, size_t &len,
                          float &rssi, float &snr) {
  len = _lora.getPacketLength();
  if (len == 0 || len > maxLen) {
    len = 0;
    startReceive();
    return RADIOLIB_ERR_NONE;
  }
  int16_t state = _lora.readData(buf, len);
  rssi = _lora.getRSSI();
  snr = _lora.getSNR();
  startReceive();
  return state;
}

int16_t Radio::startReceive() {
  _board.radioRxMode();
  return _lora.startReceive();
}
