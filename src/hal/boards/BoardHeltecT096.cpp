#ifdef BOARD_HELTEC_T096
// =====================================================================
// Heltec T096 — nRF52840 + SX1262, TFT couleur ST7735 0,96" (160x80),
// un seul bouton utilisateur, et le même FEM KCT8103L que les V4
// (LNA débrayable en réception, ~13 dB de gain PA en émission d'après
// MeshCore : 9 dBm demandés au SX1262 pour ~22 dBm à l'antenne).
//
// Brochage repris du firmware MeshCore (variants/heltec_t096) : la
// variante Arduino (variants/Heltec_T096_Board) fournit les macros
// LORA_*/SX126X_*/PIN_TFT_*/PIN_USER_BTN, et le bus SPI par défaut est
// celui de la radio. Le TFT est sur SPI1.
// =====================================================================
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <U8g2_for_Adafruit_GFX.h>

#include "../Board.h"

namespace {

// FEM KCT8103L : LDO d'alimentation, CSD (enable) et CTX (aiguillage :
// HIGH = PA en émission / LNA contourné en réception, LOW = LNA actif).
constexpr uint8_t kPinFemLdo = 30;
constexpr uint8_t kPinFemCsd = 12;
constexpr uint8_t kPinFemCtx = 41;
constexpr int8_t kFemTxGainDb = 13;

constexpr uint8_t kPinVext = 26;   // VDD du TFT et du GPS, actif à l'état HAUT
constexpr uint8_t kPin3V3En = 38;  // rail 3,3 V des périphériques

const ButtonSpec kButtons[] = {
    // pull-up externe sur la carte
    {Key::Ok, PIN_USER_BTN, /*activeLow=*/true, /*internalPullup=*/false},
};

// Adaptateur Display -> ST7735 : trame composée dans un framebuffer
// 16 bits (25,6 Ko de RAM), zone logique 128x64 centrée dans le panneau
// 160x80, polices U8g2 rendues par U8g2_for_Adafruit_GFX. Init "mini
// 160x80" sans inversion de couleurs, comme le pilote MeshCore du T096.
class T096Display : public Display {
public:
  void begin() override {
    // Rétroéclairage coupé pendant l'init (actif à l'état bas)
    pinMode(PIN_TFT_LEDA_CTL, OUTPUT);
    digitalWrite(PIN_TFT_LEDA_CTL, !PIN_TFT_LEDA_CTL_ACTIVE);
    _tft.initR(INITR_MINI160x80);
    _tft.setRotation(1);  // paysage 160x80
    _fonts.begin(_canvas);
    _fonts.setFontMode(1);  // fond transparent
    _fonts.setForegroundColor(kWhite);
    clear();
    send();
    digitalWrite(PIN_TFT_LEDA_CTL, PIN_TFT_LEDA_CTL_ACTIVE);
  }

  void clear() override { _canvas.fillScreen(kBlack); }
  void send() override {
    _tft.drawRGBBitmap(0, 0, _canvas.getBuffer(), kPanelW, kPanelH);
  }

  void setFont(Font font) override {
    switch (font) {
      case Font::kSmall: _fonts.setFont(u8g2_font_6x12_tf); break;
      case Font::kMedium: _fonts.setFont(u8g2_font_helvB12_tr); break;
      case Font::kMenu: _fonts.setFont(u8g2_font_10x20_tf); break;
      case Font::kBig: _fonts.setFont(u8g2_font_logisoso24_tr); break;
    }
  }

  void drawText(int16_t x, int16_t y, const char *utf8) override {
    _fonts.drawUTF8(x + kOffX, y + kOffY, utf8);
  }
  uint16_t textWidth(const char *utf8) override {
    return _fonts.getUTF8Width(utf8);
  }
  void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) override {
    _canvas.fillRect(x + kOffX, y + kOffY, w, h, _ink);
  }
  void drawHLine(int16_t x, int16_t y, int16_t w) override {
    _canvas.drawFastHLine(x + kOffX, y + kOffY, w, _ink);
  }

  void setInkInverted(bool inverted) override {
    _ink = inverted ? kBlack : kWhite;
    _fonts.setForegroundColor(_ink);
  }
  void invertFrame() override {
    uint16_t *pixels = _canvas.getBuffer();
    for (int32_t i = 0; i < (int32_t)kPanelW * kPanelH; i++) {
      pixels[i] ^= 0xFFFF;
    }
  }

private:
  static constexpr int16_t kPanelW = 160;
  static constexpr int16_t kPanelH = 80;
  static constexpr int16_t kOffX = (kPanelW - 128) / 2;
  static constexpr int16_t kOffY = (kPanelH - 64) / 2;
  static constexpr uint16_t kBlack = 0x0000;
  static constexpr uint16_t kWhite = 0xFFFF;

  Adafruit_ST7735 _tft{&SPI1, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST};
  GFXcanvas16 _canvas{kPanelW, kPanelH};
  U8G2_FOR_ADAFRUIT_GFX _fonts;
  uint16_t _ink = kWhite;
};

class HeltecT096Board : public Board {
public:
  const char *name() const override { return "Heltec T096"; }

  void initPower() override {
    pinMode(kPin3V3En, OUTPUT);
    digitalWrite(kPin3V3En, HIGH);
    pinMode(kPinVext, OUTPUT);
    digitalWrite(kPinVext, HIGH);  // allume le VDD du TFT

    // Alimente puis configure le FEM. CSD haut = actif ; CTX haut au
    // départ = PA dans le chemin d'émission, LNA contourné en réception.
    // L'aiguillage RX est ensuite géré par radioRxMode() selon setFemLna().
    // Attention si le LNA est activé : son gain s'ajoute au RSSI mesuré
    // par le SX1262, précisément la donnée que cet appareil affiche.
    pinMode(kPinFemLdo, OUTPUT);
    digitalWrite(kPinFemLdo, HIGH);
    delay(1);  // temps de démarrage du FEM
    pinMode(kPinFemCsd, OUTPUT);
    digitalWrite(kPinFemCsd, HIGH);
    pinMode(kPinFemCtx, OUTPUT);
    digitalWrite(kPinFemCtx, HIGH);

    delay(50);  // stabilisation des rails avant l'init du TFT
  }

  void radioTxMode() override { digitalWrite(kPinFemCtx, HIGH); }

  void radioRxMode() override {
    digitalWrite(kPinFemCtx, _femLnaEnabled ? LOW : HIGH);
  }

  bool hasFemLna() const override { return true; }

  void setFemLna(bool enabled) override { _femLnaEnabled = enabled; }

  Display &display() override { return _display; }

  RadioTraits radio() const override {
    RadioTraits t;
    t.pins.nss = LORA_CS;
    t.pins.dio1 = SX126X_DIO1;
    t.pins.reset = SX126X_RESET;
    t.pins.busy = SX126X_BUSY;
    // sck/miso/mosi à -1 : bus SPI par défaut de la variante
    t.dio2AsRfSwitch = true;
    t.tcxoVoltage = SX126X_DIO3_TCXO_VOLTAGE;
    t.currentLimitmA = 140;
    t.femTxGainDb = kFemTxGainDb;
    return t;
  }

  int8_t txPowerMaxDbm() const override { return 22; }

  const ButtonSpec *buttons(size_t &count) const override {
    count = sizeof(kButtons) / sizeof(kButtons[0]);
    return kButtons;
  }

private:
  bool _femLnaEnabled = false;
  T096Display _display;
};

}  // namespace

Board &board() {
  static HeltecT096Board instance;
  return instance;
}
#endif  // BOARD_HELTEC_T096
