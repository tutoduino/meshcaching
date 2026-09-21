#pragma once
#include <Arduino.h>
#include <RadioLib.h>

#include "Display.h"

// =====================================================================
// Abstract description of a supported board.
//
// Each concrete target (src/hal/boards/*.cpp) implements this interface
// and provides the single instance through board(). The board file is
// selected at compile time by the -D BOARD_xxx flag of the matching
// PlatformIO environment.
// =====================================================================

// Logical keys, independent of the number of physical buttons. A board
// only exposes the ones it has: the Wio Tracker L1 has a full
// directional pad, the Heltec boards a single button (Ok).
enum class Key : uint8_t { Ok, Back, Up, Down, Left, Right };

struct ButtonSpec {
  Key key;
  uint8_t pin;
  bool activeLow;
  bool internalPullup;  // false if the board already has its pull-up
};

struct InputEvent {
  Key key;
  bool longPress;
};

struct RadioPins {
  uint32_t nss = RADIOLIB_NC;
  uint32_t dio1 = RADIOLIB_NC;
  uint32_t reset = RADIOLIB_NC;
  uint32_t busy = RADIOLIB_NC;
  int16_t sck = -1;   // -1: default SPI bus of the variant
  int16_t miso = -1;
  int16_t mosi = -1;
  uint32_t rxEn = RADIOLIB_NC;  // antenna switch pins driven by
  uint32_t txEn = RADIOLIB_NC;  // RadioLib, if the board has them
};

struct RadioTraits {
  RadioPins pins;
  bool dio2AsRfSwitch = false;
  float tcxoVoltage = 0.0f;    // volts; 0 = plain crystal, no TCXO
  uint8_t currentLimitmA = 60;
  // Gain (dB) of an external FEM amplifier on transmit: the power
  // requested "at the antenna" is reduced by that much before being
  // passed to the SX1262.
  int8_t femTxGainDb = 0;
  // Patch for register 0x8B5 (undocumented): Heltec recipe taken from
  // the MeshCore firmware, "improved RX" on the V4 boards fitted with a
  // FEM.
  bool femRxPatch = false;
};

// SX1262 power floor; the upper bound is specific to each board
// (txPowerMaxDbm).
constexpr int8_t kTxPowerMinDbm = -9;

class Board {
public:
  virtual ~Board() {}
  virtual const char *name() const = 0;

  // Peripheral power rails (Vext, FEM LDO...).
  // Called before any I2C/SPI access.
  virtual void initPower() {}

  // Board display (see hal/Display.h), built but not initialized.
  virtual Display &display() = 0;
  // Starts the display; override when the init is out of the ordinary
  // (I2C address probing, contrast setting, etc.)
  virtual void beginDisplay() { display().begin(); }

  virtual RadioTraits radio() const = 0;

  // TX power "at the antenna", bounds specific to the board.
  virtual int8_t txPowerMaxDbm() const = 0;
  virtual int8_t txPowerDefaultDbm() const { return txPowerMaxDbm(); }
  // Useful floor: the SX1262 floor raised by the FEM TX gain - below
  // that, the setpoint would be clamped on the chip side and the
  // displayed power would lie about the power actually transmitted.
  int8_t txPowerMinDbm() const {
    return (int8_t)(kTxPowerMinDbm + radio().femTxGainDb);
  }

  // TX/RX switching of an external FEM, if the board has one to drive.
  virtual void radioTxMode() {}
  virtual void radioRxMode() {}

  // External LNA (FEM) that can be disengaged on receive? If so,
  // setFemLna() picks the routing applied at the next switch to receive.
  virtual bool hasFemLna() const { return false; }
  virtual void setFemLna(bool /*enabled*/) {}

  // Hardware self-test performed during initPower(): error message if
  // the board is not the expected one (nullptr otherwise). The
  // application checks it after the display init and stops there on
  // failure.
  virtual const char *selfCheckError() const { return nullptr; }

  virtual const ButtonSpec *buttons(size_t &count) const = 0;

  // Indicates whether the board features a directional pad, trackball,
  // or keyboard capable of UP/DOWN navigation. The UI uses this to adapt
  // its layout (e.g., single-button vs. multi-button menu navigation).
  // By default, it checks if Key::Up is mapped in buttons(). Boards with
  // custom input polling (like trackballs) should override this to return true.
  virtual bool hasDpad() const {
    size_t count = 0;
    const ButtonSpec *specs = buttons(count);
    for (size_t i = 0; i < count; i++) {
      if (specs[i].key == Key::Up) return true;
    }
    return false;
  }

  // Extension point for complex input devices that cannot be handled by simple
  // GPIO interrupts/polling (e.g., I2C keyboards or trackball step counting).
  // Called on every loop iteration. Returns true and populates 'event' if an
  // input occurred. Existing boards using standard buttons can ignore this.
  virtual bool pollInput(InputEvent & /*event*/) { return false; }
};

// Implemented by the single src/hal/boards/*.cpp built for the target.
Board &board();
