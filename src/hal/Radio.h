#pragma once
#include <RadioLib.h>

#include "Board.h"
#include "RxGain.h"

// Enveloppe du SX1262 (RadioLib) : le cablage vient de la description de
// carte, et la puissance TX s'exprime "a l'antenne" — le gain d'un
// eventuel FEM externe est retranche avant d'etre passe au SX1262.
//
// La reception est pilotee par interruption : DIO1 leve un drapeau,
// consomme tranquillement dans loop() via packetAvailable() (regle de
// base avec RadioLib : jamais de traitement dans l'ISR elle-meme).
class Radio {
public:
  explicit Radio(Board &board);

  // Configure et met en ecoute. Renvoie RADIOLIB_ERR_NONE si tout va bien.
  int16_t begin(float freqMhz, float bwKhz, uint8_t sf, uint8_t cr,
                int8_t txPowerDbm);

  // Puissance a l'antenne, bornee a la plage de la carte.
  void setTxPowerDbm(int8_t antennaDbm);
  int8_t txPowerDbm() const { return _antennaDbm; }

  // Chaine de gain en reception (boost interne du SX126x, LNA du FEM si
  // la carte en a un, ou rien) ; remet la radio en ecoute.
  void setRxGainMode(RxGainMode mode);

  // Emet puis repasse en ecoute.
  int16_t transmit(const uint8_t *data, size_t len);

  // true si un paquet est arrive depuis le dernier appel
  bool packetAvailable();

  // Lit le paquet recu puis remet la radio en ecoute. len vaut 0 si le
  // paquet etait vide ou trop grand pour buf.
  int16_t readPacket(uint8_t *buf, size_t maxLen, size_t &len,
                     float &rssi, float &snr);

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
