#pragma once
#include <RadioLib.h>

#include "Board.h"
#include "RxGain.h"

// Enveloppe du SX1262 (RadioLib) : le câblage vient de la description de
// carte, et la puissance TX s'exprime "à l'antenne" — le gain d'un
// éventuel FEM externe est retranché avant d'être passé au SX1262.
//
// La réception est pilotée par interruption : DIO1 lève un drapeau,
// consommé tranquillement dans loop() via packetAvailable() (règle de
// base avec RadioLib : jamais de traitement dans l'ISR elle-même).
class Radio {
public:
  explicit Radio(Board &board);

  // Configure et met en écoute. Renvoie RADIOLIB_ERR_NONE si tout va bien.
  int16_t begin(float freqMhz, float bwKhz, uint8_t sf, uint8_t cr,
                int8_t txPowerDbm);

  // Puissance à l'antenne, bornée à la plage de la carte.
  void setTxPowerDbm(int8_t antennaDbm);
  int8_t txPowerDbm() const { return _antennaDbm; }

  // Chaîne de gain en réception (boost interne du SX126x, LNA du FEM si
  // la carte en a un, ou rien) ; remet la radio en écoute.
  void setRxGainMode(RxGainMode mode);

  // Un cycle de CAD (LBT) : true si le canal est libre. Remet la radio
  // en écoute et n'efface que le drapeau levé par le CAD lui-même — la
  // boucle d'essais vit chez l'appelant, qui peut ainsi traiter un vrai
  // paquet arrivé entre deux cycles au lieu de le perdre.
  bool channelClear();

  // Émet puis repasse en écoute.
  int16_t transmit(const uint8_t *data, size_t len);

  // RSSI instantané, lu sans perturber la réception en cours.
  float rssiInstant() { return _lora.getRSSI(false); }

  // true si un paquet est arrivé depuis le dernier appel
  bool packetAvailable();

  // Lit le paquet reçu puis remet la radio en écoute. len vaut 0 si le
  // paquet était vide ou trop grand pour buf. rssi est le RssiPkt moyenné
  // sur le paquet (bruit compris), despreadRssi le SignalRssiPkt estimé
  // après désétalement du signal LoRa.
  int16_t readPacket(uint8_t *buf, size_t maxLen, size_t &len, float &rssi,
                     float &snr, float &despreadRssi);

  int16_t startReceive();

private:
  int8_t chipPowerDbm(int8_t antennaDbm) const;
  static void onDio1Isr();

  static volatile bool s_packetFlag;

  Board &_board;
  RadioTraits _traits;
  SX1262 _lora;
  int8_t _antennaDbm = 0;
};
