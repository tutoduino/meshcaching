#if defined(BOARD_HELTEC_V4_3) || defined(BOARD_HELTEC_V4_R8)
// =====================================================================
// Heltec WiFi LoRa 32 V4 — deux déclinaisons partagent cette carte :
//  - BOARD_HELTEC_V4_3 : révision 4.3 (ESP32-S3R2, 2 Mo PSRAM) ;
//  - BOARD_HELTEC_V4_R8 : série "R8" (ESP32-S3R8, 8 Mo PSRAM), vendue
//    ensuite, qui ne diffère ici que par son rail Vext (GPIO40, actif
//    à l'état BAS, contre GPIO36 actif HAUT sur la 4.3).
//
// Commun aux deux : SX1262 (brochage LoRa et OLED identique au V3),
// OLED 128x64 piloté en SSD1306, un seul bouton utilisateur (PRG), et
// un FEM (front-end module) KCT8103L entre le SX1262 et l'antenne, qui
// ajoute ~12 dB en émission et offre un LNA débrayable en réception.
// Valeurs reprises du firmware MeshCore (variants/heltec_v4{,_r8}).
//
// Note : les V4 antérieurs au 4.3 embarquent un autre FEM (GC1109,
// pilotage différent) et ne sont pas gérés : initPower() le détecte et
// bloque le démarrage. Les déclinaisons TFT et e-ink ne sont pas gérées
// non plus.
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
constexpr uint8_t kPinButtonPrg = 0;  // relié à la masse quand pressé

#ifdef BOARD_HELTEC_V4_R8
constexpr char kBoardName[] = "Heltec WiFi LoRa 32 V4 R8";
constexpr uint8_t kPinVext = 40;
constexpr uint8_t kVextOnLevel = LOW;
#else
constexpr char kBoardName[] = "Heltec WiFi LoRa 32 V4.3";
constexpr uint8_t kPinVext = 36;
constexpr uint8_t kVextOnLevel = HIGH;
#endif

const ButtonSpec kButtons[] = {
    {Key::Ok, kPinButtonPrg, /*activeLow=*/true, /*internalPullup=*/true},
};

class HeltecV4Board : public Board {
public:
  const char *name() const override { return kBoardName; }

  void initPower() override {
    pinMode(kPinVext, OUTPUT);
    digitalWrite(kPinVext, kVextOnLevel);  // allume le rail Vext (OLED)

    // Alimente le FEM puis identifie sa référence par le niveau de repos
    // de CSD (astuce reprise de MeshCore) : pull-up interne sur le
    // KCT8103L (V4.3 et R8) -> HIGH, pull-down sur le GC1109 (V4 <= 4.2)
    // -> LOW. Un GC1109 se pilote différemment : plutôt que d'émettre à
    // travers un FEM mal configuré, on coupe son alimentation et on
    // laisse l'application s'arrêter sur selfCheckError().
    pinMode(kPinFemLdo, OUTPUT);
    digitalWrite(kPinFemLdo, HIGH);
    delay(1);  // temps de démarrage du FEM
    pinMode(kPinFemCsd, INPUT);
    delay(1);
    if (digitalRead(kPinFemCsd) == LOW) {
      digitalWrite(kPinFemLdo, LOW);
      _selfCheckError = "FEM GC1109 (V4<=4.2)";
    } else {
      // Configure le FEM. CSD haut = actif ; CTX haut au départ = PA dans
      // le chemin d'émission, LNA contourné en réception. L'aiguillage RX
      // est ensuite géré par radioRxMode() selon setFemLna(). Attention si
      // le LNA est activé : son gain s'ajoute au RSSI mesuré par le
      // SX1262, précisément la donnée que cet appareil affiche.
      pinMode(kPinFemCsd, OUTPUT);
      digitalWrite(kPinFemCsd, HIGH);
      pinMode(kPinFemCtx, OUTPUT);
      digitalWrite(kPinFemCtx, HIGH);
    }

    delay(150);  // stabilisation du Vext avant l'init de l'OLED
  }

  const char *selfCheckError() const override { return _selfCheckError; }

  void radioTxMode() override { digitalWrite(kPinFemCtx, HIGH); }

  void radioRxMode() override {
    digitalWrite(kPinFemCtx, _femLnaEnabled ? LOW : HIGH);
  }

  bool hasFemLna() const override { return true; }

  void setFemLna(bool enabled) override { _femLnaEnabled = enabled; }

  U8G2 &display() override { return _display; }

  void beginDisplay() override {
    _display.begin();
    // Les panneaux OLED de la série V4 sont des clones du SSD1306 (type
    // SSD1315) : au reset ils utilisent leur référence de courant externe
    // et restent à mi-luminosité. La commande 0xAD les bascule sur l'IREF
    // interne en courant maximal — le correctif connu pour ces panneaux.
    // Appliqué écran éteint, comme l'exige le datasheet, puis contraste
    // au maximum.
    _display.setPowerSave(1);
    _display.sendF("ca", 0x0ad, 0x030);  // IREF interne, courant maxi
    _display.setPowerSave(0);
    _display.setContrast(255);
  }

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

  // Même FEM et même chaîne RF que la 4.3 : même plafond de 20 dBm à
  // l'antenne pour la série R8 (à ajuster si son PA est qualifié plus haut).
  int8_t txPowerMaxDbm() const override { return 20; }

  const ButtonSpec *buttons(size_t &count) const override {
    count = sizeof(kButtons) / sizeof(kButtons[0]);
    return kButtons;
  }

private:
  const char *_selfCheckError = nullptr;
  bool _femLnaEnabled = false;
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display{U8G2_R0, kPinOledReset,
                                               kPinOledScl, kPinOledSda};
};

}  // namespace

Board &board() {
  static HeltecV4Board instance;
  return instance;
}
#endif  // BOARD_HELTEC_V4_3 || BOARD_HELTEC_V4_R8
