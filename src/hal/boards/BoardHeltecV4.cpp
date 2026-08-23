#ifdef BOARD_HELTEC_V4
// =====================================================================
// Heltec WiFi LoRa 32 V4, révision 4.3 — ESP32-S3R2 + SX1262, OLED
// SSD1306 128x64 (I2C), un seul bouton utilisateur (PRG).
//
// Brochage LoRa et OLED identique au V3, mais : Vext actif à l'état
// HAUT, et un FEM (front-end module) KCT8103L entre le SX1262 et
// l'antenne, qui ajoute ~12 dB en émission. Valeurs reprises du firmware
// MeshCore (variants/heltec_v4).
//
// Note : la révision 4.2 embarque un autre FEM (GC1109, pilotage
// différent) et n'est pas gérée ici.
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
// HIGH = PA en émission / LNA contourné en réception, LOW = LNA actif).
constexpr uint8_t kPinFemLdo = 7;
constexpr uint8_t kPinFemCsd = 2;
constexpr uint8_t kPinFemCtx = 5;
// Gain du PA en émission : MeshCore documente 10 dBm demandés au SX1262
// pour 22 dBm mesurés à l'antenne.
constexpr int8_t kFemTxGainDb = 12;

constexpr uint8_t kPinOledSda = 17;
constexpr uint8_t kPinOledScl = 18;
constexpr uint8_t kPinOledReset = 21;
constexpr uint8_t kPinVext = 36;      // actif à l'état HAUT sur le V4
constexpr uint8_t kPinButtonPrg = 0;  // relié à la masse quand pressé

const ButtonSpec kButtons[] = {
    {Key::Ok, kPinButtonPrg, /*activeLow=*/true, /*internalPullup=*/true},
};

class HeltecV4Board : public Board {
public:
  const char *name() const override { return "Heltec WiFi LoRa 32 V4.3"; }

  void initPower() override {
    pinMode(kPinVext, OUTPUT);
    digitalWrite(kPinVext, HIGH);  // allume le rail Vext (OLED)

    // Alimente puis configure le FEM. CSD haut = FEM actif ; CTX haut au
    // depart = PA dans le chemin d'émission, LNA contourné en réception.
    // L'aiguillage RX est ensuite géré par radioRxMode() selon setFemLna().
    // Attention si le LNA est activé : son gain s'ajoute au RSSI mesure
    // par le SX1262, précisément la donnée que cet appareil affiche.
    pinMode(kPinFemLdo, OUTPUT);
    digitalWrite(kPinFemLdo, HIGH);
    delay(1);  // temps de démarrage du FEM
    pinMode(kPinFemCsd, OUTPUT);
    digitalWrite(kPinFemCsd, HIGH);
    pinMode(kPinFemCtx, OUTPUT);
    digitalWrite(kPinFemCtx, HIGH);

    delay(150);  // stabilisation du Vext avant l'init de l'OLED
  }

  void radioTxMode() override { digitalWrite(kPinFemCtx, HIGH); }

  void radioRxMode() override {
    digitalWrite(kPinFemCtx, _femLnaEnabled ? LOW : HIGH);
  }

  bool hasFemLna() const override { return true; }

  void setFemLna(bool enabled) override { _femLnaEnabled = enabled; }

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
    t.femTxGainDb = kFemTxGainDb;
    return t;
  }

  int8_t txPowerMaxDbm() const override { return 20; }

  const ButtonSpec *buttons(size_t &count) const override {
    count = sizeof(kButtons) / sizeof(kButtons[0]);
    return kButtons;
  }

private:
  bool _femLnaEnabled = false;
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display{U8G2_R0, kPinOledReset,
                                               kPinOledScl, kPinOledSda};
};

}  // namespace

Board &board() {
  static HeltecV4Board instance;
  return instance;
}
#endif  // BOARD_HELTEC_V4
