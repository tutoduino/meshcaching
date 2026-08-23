#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <U8g2lib.h>

// =====================================================================
// Description abstraite d'une carte supportee.
//
// Chaque cible concrete (src/hal/boards/*.cpp) implemente cette
// interface et fournit l'unique instance via board(). Le fichier de
// carte est selectionne a la compilation par le drapeau -D BOARD_xxx
// de l'environnement PlatformIO correspondant.
// =====================================================================

// Touches logiques, independantes du nombre de boutons physiques.
// Une carte n'expose que celles qu'elle possede : le Wio Tracker L1 a
// une croix directionnelle complete, les Heltec un seul bouton (Ok).
enum class Key : uint8_t { Ok, Back, Up, Down, Left, Right };

struct ButtonSpec {
  Key key;
  uint8_t pin;
  bool activeLow;
  bool internalPullup;  // false si la carte a deja sa resistance de tirage
};

struct RadioPins {
  uint32_t nss = RADIOLIB_NC;
  uint32_t dio1 = RADIOLIB_NC;
  uint32_t reset = RADIOLIB_NC;
  uint32_t busy = RADIOLIB_NC;
  int16_t sck = -1;   // -1 : bus SPI par defaut de la variante
  int16_t miso = -1;
  int16_t mosi = -1;
  uint32_t rxEn = RADIOLIB_NC;  // broches du switch d'antenne pilotees par
  uint32_t txEn = RADIOLIB_NC;  // RadioLib, si la carte en a
};

struct RadioTraits {
  RadioPins pins;
  bool dio2AsRfSwitch = false;
  float tcxoVoltage = 0.0f;    // volts ; 0 = quartz simple, pas de TCXO
  uint8_t currentLimitmA = 60;
  // Gain (dB) d'un ampli FEM externe en emission : la puissance demandee
  // "a l'antenne" est reduite d'autant avant d'etre passee au SX1262.
  int8_t femTxGainDb = 0;
};

// Plancher de puissance du SX1262 ; la borne haute est propre a chaque
// carte (txPowerMaxDbm).
constexpr int8_t kTxPowerMinDbm = -9;

class Board {
public:
  virtual ~Board() {}
  virtual const char *name() const = 0;

  // Rails d'alimentation des peripheriques (Vext, LDO du FEM...).
  // Appele avant tout acces I2C/SPI.
  virtual void initPower() {}

  // Ecran U8g2 de la carte, construit mais pas initialise.
  virtual U8G2 &display() = 0;
  // Demarre l'ecran ; a surcharger si l'init sort de l'ordinaire
  // (sondage d'adresse I2C, etc.)
  virtual void beginDisplay() { display().begin(); }

  virtual RadioTraits radio() const = 0;

  // Puissance TX "a l'antenne", bornes propres a la carte.
  virtual int8_t txPowerMaxDbm() const = 0;
  virtual int8_t txPowerDefaultDbm() const { return txPowerMaxDbm(); }

  // Commutation TX/RX d'un FEM externe, si la carte en a un a piloter.
  virtual void radioTxMode() {}
  virtual void radioRxMode() {}

  // LNA externe (FEM) debrayable en reception ? Si oui, setFemLna()
  // choisit l'aiguillage applique au prochain passage en reception.
  virtual bool hasFemLna() const { return false; }
  virtual void setFemLna(bool /*enabled*/) {}

  virtual const ButtonSpec *buttons(size_t &count) const = 0;
};

// Implementee par l'unique src/hal/boards/*.cpp compile pour la cible.
Board &board();
