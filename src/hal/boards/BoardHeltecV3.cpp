#ifdef BOARD_HELTEC_V3
// =====================================================================
// Heltec WiFi LoRa 32 V3 — ESP32-S3 + SX1262, OLED SSD1306 128x64 (I2C),
// un seul bouton utilisateur (PRG).
// =====================================================================
#include "../Board.h"

namespace {

constexpr uint8_t kPinLoraNss = 8;
constexpr uint8_t kPinLoraSck = 9;
constexpr uint8_t kPinLoraMosi = 10;
constexpr uint8_t kPinLoraMiso = 11;
constexpr uint8_t kPinLoraReset = 12;
constexpr uint8_t kPinLoraBusy = 13;
constexpr uint8_t kPinLoraDio1 = 14;

constexpr uint8_t kPinOledSda = 17;
constexpr uint8_t kPinOledScl = 18;
constexpr uint8_t kPinOledReset = 21;
constexpr uint8_t kPinVext = 36;      // alim de l'OLED, actif a l'etat bas
constexpr uint8_t kPinButtonPrg = 0;  // relie a la masse quand presse

const ButtonSpec kButtons[] = {
    {Key::Ok, kPinButtonPrg, /*activeLow=*/true, /*internalPullup=*/true},
};

class HeltecV3Board : public Board {
public:
  const char *name() const override { return "Heltec WiFi LoRa 32 V3"; }

  void initPower() override {
    pinMode(kPinVext, OUTPUT);
    digitalWrite(kPinVext, LOW);  // allume le rail Vext (OLED)
    delay(150);
  }

  U8G2 &display() override { return _display; }

  RadioTraits radio() const override {
    RadioTraits t;
    t.pins.nss = kPinLoraNss;
    t.pins.dio1 = kPinLoraDio1;
    t.pins.reset = kPinLoraReset;
    t.pins.busy = kPinLoraBusy;
    t.pins.sck = kPinLoraSck;
    t.pins.miso = kPinLoraMiso;
    t.pins.mosi = kPinLoraMosi;
    t.dio2AsRfSwitch = true;  // DIO2 pilote le switch d'antenne
    t.tcxoVoltage = 1.8f;
    t.currentLimitmA = 140;
    t.rxBoostedGain = true;
    return t;
  }

  int8_t txPowerMaxDbm() const override { return 22; }

  const ButtonSpec *buttons(size_t &count) const override {
    count = sizeof(kButtons) / sizeof(kButtons[0]);
    return kButtons;
  }

private:
  // Le reset materiel de l'OLED (broche 21) est gere par U8g2 au begin()
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display{U8G2_R0, kPinOledReset,
                                               kPinOledScl, kPinOledSda};
};

}  // namespace

Board &board() {
  static HeltecV3Board instance;
  return instance;
}
#endif  // BOARD_HELTEC_V3
