#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <U8g2lib.h>

// =====================================================================
// Description abstraite d'une carte supportée.
//
// Chaque cible concrète (src/hal/boards/*.cpp) implémente cette
// interface et fournit l'unique instance via board(). Le fichier de
// carte est sélectionné à la compilation par le drapeau -D BOARD_xxx
// de l'environnement PlatformIO correspondant.
// =====================================================================

// Touches logiques, indépendantes du nombre de boutons physiques.
// Une carte n'expose que celles qu'elle possède : le Wio Tracker L1 a
// une croix directionnelle complète, les Heltec un seul bouton (Ok).
enum class Key : uint8_t { Ok, Back, Up, Down, Left, Right };

struct ButtonSpec {
  Key key;
  uint8_t pin;
  bool activeLow;
  bool internalPullup;  // false si la carte a déjà sa résistance de tirage
};

struct RadioPins {
  uint32_t nss = RADIOLIB_NC;
  uint32_t dio1 = RADIOLIB_NC;
  uint32_t reset = RADIOLIB_NC;
  uint32_t busy = RADIOLIB_NC;
  int16_t sck = -1;   // -1 : bus SPI par défaut de la variante
  int16_t miso = -1;
  int16_t mosi = -1;
  uint32_t rxEn = RADIOLIB_NC;  // broches du switch d'antenne pilotées par
  uint32_t txEn = RADIOLIB_NC;  // RadioLib, si la carte en a
};

struct RadioTraits {
  RadioPins pins;
  bool dio2AsRfSwitch = false;
  float tcxoVoltage = 0.0f;    // volts ; 0 = quartz simple, pas de TCXO
  uint8_t currentLimitmA = 60;
  // Gain (dB) d'un ampli FEM externe en émission : la puissance demandée
  // "à l'antenne" est réduite d'autant avant d'être passée au SX1262.
  int8_t femTxGainDb = 0;
};

// Plancher de puissance du SX1262 ; la borne haute est propre à chaque
// carte (txPowerMaxDbm).
constexpr int8_t kTxPowerMinDbm = -9;

class Board {
public:
  virtual ~Board() {}
  virtual const char *name() const = 0;

  // Rails d'alimentation des périphériques (Vext, LDO du FEM...).
  // Appelé avant tout accès I2C/SPI.
  virtual void initPower() {}

  // Écran U8g2 de la carte, construit mais pas initialisé.
  virtual U8G2 &display() = 0;
  // Démarre l'écran ; à surcharger si l'init sort de l'ordinaire
  // (sondage d'adresse I2C, etc.)
  virtual void beginDisplay() { display().begin(); }

  virtual RadioTraits radio() const = 0;

  // Puissance TX "à l'antenne", bornes propres à la carte.
  virtual int8_t txPowerMaxDbm() const = 0;
  virtual int8_t txPowerDefaultDbm() const { return txPowerMaxDbm(); }

  // Commutation TX/RX d'un FEM externe, si la carte en a un à piloter.
  virtual void radioTxMode() {}
  virtual void radioRxMode() {}

  // LNA externe (FEM) debrayable en réception ? Si oui, setFemLna()
  // choisit l'aiguillage appliqué au prochain passage en réception.
  virtual bool hasFemLna() const { return false; }
  virtual void setFemLna(bool /*enabled*/) {}

  virtual const ButtonSpec *buttons(size_t &count) const = 0;
};

// Implémentée par l'unique src/hal/boards/*.cpp compilé pour la cible.
Board &board();
