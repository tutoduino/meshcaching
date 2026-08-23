#ifdef BOARD_HELTEC_V4
// =====================================================================
// Heltec WiFi LoRa 32 V4, revision 4.3 — ESP32-S3R2 + SX1262, OLED
// SSD1306 128x64 (I2C), un seul bouton utilisateur (PRG).
//
// Brochage LoRa et OLED identique au V3, mais : Vext actif a l'etat
// HAUT, et un FEM (front-end module) KCT8103L entre le SX1262 et
// l'antenne, qui ajoute ~12 dB en emission. Valeurs reprises du firmware
// MeshCore (variants/heltec_v4).
//
// Note : la revision 4.2 embarque un autre FEM (GC1109, pilotage
// different) et n'est pas geree ici.
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

// FEM KCT8103L : LDO d'alimentation, CSD (enable) et CTX (aiguillage :
// HIGH = PA en emission / LNA contourne en reception, LOW = LNA actif).
constexpr uint8_t kPinFemLdo = 7;
constexpr uint8_t kPinFemCsd = 2;
constexpr uint8_t kPinFemCtx = 5;
// Gain du PA en emission : MeshCore documente 10 dBm demandes au SX1262
// pour 22 dBm mesures a l'antenne.
constexpr int8_t kFemTxGainDb = 12;

constexpr uint8_t kPinOledSda = 17;
constexpr uint8_t kPinOledScl = 18;
constexpr uint8_t kPinOledReset = 21;
constexpr uint8_t kPinVext = 36;      // actif a l'etat HAUT sur le V4
constexpr uint8_t kPinButtonPrg = 0;  // relie a la masse quand presse

const ButtonSpec kButtons[] = {
    {Key::Ok, kPinButtonPrg, /*activeLow=*/true, /*internalPullup=*/true},
};

class HeltecV4Board : public Board {
public:
  const char *name() const override { return "Heltec WiFi LoRa 32 V4.3"; }

  void initPower() override {
    pinMode(kPinVext, OUTPUT);
    digitalWrite(kPinVext, HIGH);  // allume le rail Vext (OLED)

    // Alimente puis configure le FEM. CSD haut = FEM actif ; CTX reste
    // haut en permanence : PA dans le chemin d'emission, et LNA contourne
    // en reception — un LNA actif fausserait le RSSI mesure, ce qui est
    // precisement la donnee que cet appareil affiche.
    pinMode(kPinFemLdo, OUTPUT);
    digitalWrite(kPinFemLdo, HIGH);
    delay(1);  // temps de demarrage du FEM
    pinMode(kPinFemCsd, OUTPUT);
    digitalWrite(kPinFemCsd, HIGH);
    pinMode(kPinFemCtx, OUTPUT);
    digitalWrite(kPinFemCtx, HIGH);

    delay(150);  // stabilisation du Vext avant l'init de l'OLED
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
    t.dio2AsRfSwitch = true;
    t.tcxoVoltage = 1.8f;
    t.currentLimitmA = 140;
    t.rxBoostedGain = true;
    t.femTxGainDb = kFemTxGainDb;
    return t;
  }

  int8_t txPowerMaxDbm() const override { return 20; }

  const ButtonSpec *buttons(size_t &count) const override {
    count = sizeof(kButtons) / sizeof(kButtons[0]);
    return kButtons;
  }

private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display{U8G2_R0, kPinOledReset,
                                               kPinOledScl, kPinOledSda};
};

}  // namespace

Board &board() {
  static HeltecV4Board instance;
  return instance;
}
#endif  // BOARD_HELTEC_V4
