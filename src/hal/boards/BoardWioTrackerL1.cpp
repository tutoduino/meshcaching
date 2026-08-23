#ifdef BOARD_WIO_TRACKER_L1
// =====================================================================
// Seeed Wio Tracker L1 Pro — nRF52840 + SX1262, OLED SH1106 128x64 (I2C),
// croix directionnelle + bouton menu.
//
// Les broches (P_LORA_*, SX126X_*, PIN_BUTTON*, JOYSTICK_*) viennent de
// variants/Seeed_Wio_Tracker_L1/variant.h, repris du firmware MeshCore.
// SPI et I2C utilisent les bus par défaut fixes par la variante.
// =====================================================================
#include <Wire.h>

#include "../Board.h"

namespace {

const ButtonSpec kButtons[] = {
    // Le joystick et ses directions ont des résistances de tirage sur la
    // carte ; seul le bouton menu utilise le pull-up interne.
    {Key::Ok, JOYSTICK_PRESS, /*activeLow=*/true, /*internalPullup=*/false},
    {Key::Back, PIN_BUTTON1, /*activeLow=*/true, /*internalPullup=*/true},
    {Key::Up, JOYSTICK_UP, /*activeLow=*/true, /*internalPullup=*/false},
    {Key::Down, JOYSTICK_DOWN, /*activeLow=*/true, /*internalPullup=*/false},
    {Key::Left, JOYSTICK_LEFT, /*activeLow=*/true, /*internalPullup=*/false},
    {Key::Right, JOYSTICK_RIGHT, /*activeLow=*/true, /*internalPullup=*/false},
};

class WioTrackerL1Board : public Board {
public:
  const char *name() const override { return "Seeed Wio Tracker L1 Pro"; }

  // Pas de rail Vext ni de reset dédié : l'écran est alimenté en
  // permanence, initPower() par défaut (vide) suffit.

  U8G2 &display() override { return _display; }

  void beginDisplay() override {
    // Adresse nominale 0x3D (cf. variant.h) ; certains modules SH1106
    // répondent en 0x3C, on sonde avant d'initialiser.
    Wire.begin();
    uint8_t addr = DISPLAY_ADDRESS;
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() != 0) {
      addr = 0x3C;
    }
    _display.setI2CAddress(addr << 1);  // U8g2 attend l'adresse 8 bits
    _display.setBusClock(400000);
    _display.begin();
  }

  RadioTraits radio() const override {
    RadioTraits t;
    t.pins.nss = P_LORA_NSS;
    t.pins.dio1 = P_LORA_DIO_1;
    t.pins.reset = P_LORA_RESET;
    t.pins.busy = P_LORA_BUSY;
    // sck/miso/mosi à -1 : bus SPI par défaut de la variante
    t.pins.rxEn = SX126X_RXEN;  // RadioLib doit piloter RXEN sur cette carte
    t.pins.txEn = SX126X_TXEN;
    t.dio2AsRfSwitch = SX126X_DIO2_AS_RF_SWITCH;
    // Sans la déclaration du TCXO 1.8V sur DIO3, l'init de la radio
    // échoue (le quartz ne démarre jamais).
    t.tcxoVoltage = SX126X_DIO3_TCXO_VOLTAGE;
    t.currentLimitmA = 140;
    return t;
  }

  int8_t txPowerMaxDbm() const override { return 22; }

  const ButtonSpec *buttons(size_t &count) const override {
    count = sizeof(kButtons) / sizeof(kButtons[0]);
    return kButtons;
  }

private:
  U8G2_SH1106_128X64_NONAME_F_HW_I2C _display{U8G2_R0, U8X8_PIN_NONE};
};

}  // namespace

Board &board() {
  static WioTrackerL1Board instance;
  return instance;
}
#endif  // BOARD_WIO_TRACKER_L1
