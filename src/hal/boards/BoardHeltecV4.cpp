#if defined(BOARD_HELTEC_V4_2) || defined(BOARD_HELTEC_V4_3) || \
    defined(BOARD_HELTEC_V4_R8)
// =====================================================================
// Heltec WiFi LoRa 32 V4 - three variants share this board file:
//  - BOARD_HELTEC_V4_2: revision <= 4.2 (ESP32-S3R2 + GC1109 FEM);
//  - BOARD_HELTEC_V4_3: revision 4.3 (ESP32-S3R2 + KCT8103L FEM);
//  - BOARD_HELTEC_V4_R8: "R8" series (ESP32-S3R8, 8 MB PSRAM), sold
//    later, which differs from the 4.3 only by the pin of its Vext rail
//    (GPIO40 instead of GPIO36).
//
// Common: SX1262 (LoRa and OLED pinout identical to the V3), 128x64
// OLED driven as SSD1306, a single user button (PRG), and a FEM
// (front-end module) between the SX1262 and the antenna (~12 dB on
// transmit). Values taken from the MeshCore firmware
// (variants/heltec_v4{,_r8}).
//
// FEM difference: the GC1109 (V4 <= 4.2) is driven through CSD (GPIO2)
// + CPS (GPIO46), with its TX/RX switch on the SX1262's DIO2; the
// KCT8103L (V4.3 / R8) through CSD (GPIO2) + CTX (GPIO5), with a
// bypassable RX LNA. The part is checked at startup (idle level of
// CSD): a binary flashed on the wrong revision stops without
// transmitting. The TFT and e-ink variants are not supported.
// =====================================================================
#include <U8g2lib.h>

#include "../Board.h"
#include "../U8g2Display.h"

namespace {

constexpr uint8_t kPinLoraNss = 8;
constexpr uint8_t kPinLoraSck = 9;
constexpr uint8_t kPinLoraMosi = 10;
constexpr uint8_t kPinLoraMiso = 11;
constexpr uint8_t kPinLoraReset = 12;
constexpr uint8_t kPinLoraBusy = 13;
constexpr uint8_t kPinLoraDio1 = 14;

// FEM: shared supply LDO, then GC1109 (EN + CPS) or KCT8103L (CSD +
// CTX). PA gain on transmit: MeshCore documents 10 dBm requested from
// the SX1262 for 22 dBm measured at the antenna.
constexpr uint8_t kPinFemLdo = 7;
constexpr uint8_t kPinFemCsd = 2;         // GC1109 EN / KCT8103L CSD
constexpr uint8_t kPinFemGc1109Cps = 46;  // HIGH = full PA (TX), LOW = RX
constexpr uint8_t kPinFemKctCtx = 5;      // HIGH = TX / LNA bypass
constexpr int8_t kFemTxGainDb = 12;

constexpr uint8_t kPinOledSda = 17;
constexpr uint8_t kPinOledScl = 18;
constexpr uint8_t kPinOledReset = 21;
constexpr uint8_t kPinButtonPrg = 0;  // tied to ground when pressed

// Vext (OLED supply): active LOW across the whole series - the rail is
// switched by a P-channel MOSFET (official HTIT-WB32LAF V4.3 schematic,
// transistor Q2 AO3401A), as on the V3. Do not trust the
// PIN_VEXT_EN_ACTIVE=HIGH of MeshCore's heltec_v4 variant: their display
// is built without any reference to the rail, so that level is never
// applied (and their newer R8 variant does say LOW).
#ifdef BOARD_HELTEC_V4_R8
constexpr char kBoardName[] = "Heltec WiFi LoRa 32 V4 R8";
constexpr uint8_t kPinVext = 40;
#elif defined(BOARD_HELTEC_V4_2)
constexpr char kBoardName[] = "Heltec WiFi LoRa 32 V4.2";
constexpr uint8_t kPinVext = 36;
#else
constexpr char kBoardName[] = "Heltec WiFi LoRa 32 V4.3";
constexpr uint8_t kPinVext = 36;
#endif
constexpr uint8_t kVextOnLevel = LOW;

#ifdef BOARD_HELTEC_V4_2
constexpr bool kExpectGc1109 = true;
#else
constexpr bool kExpectGc1109 = false;
#endif

const ButtonSpec kButtons[] = {
    {Key::Ok, kPinButtonPrg, /*activeLow=*/true, /*internalPullup=*/true},
};

class HeltecV4Board : public Board {
public:
  const char *name() const override { return kBoardName; }

  void initPower() override {
    pinMode(kPinVext, OUTPUT);
    digitalWrite(kPinVext, kVextOnLevel);  // turn on the Vext rail (OLED)

    // Power the FEM, then identify its part number from the idle level
    // of CSD (trick taken from MeshCore): internal pull-up on the
    // KCT8103L (V4.3 and R8) -> HIGH, pull-down on the GC1109 (V4 <= 4.2)
    // -> LOW. The binary only configures the FEM it was built for:
    // rather than transmitting through a misconfigured FEM, we cut its
    // supply and let the application stop on selfCheckError().
    pinMode(kPinFemLdo, OUTPUT);
    digitalWrite(kPinFemLdo, HIGH);
    delay(1);  // FEM start-up time
    pinMode(kPinFemCsd, INPUT);
    delay(1);
    const bool isGc1109 = (digitalRead(kPinFemCsd) == LOW);

    if (isGc1109 != kExpectGc1109) {
      digitalWrite(kPinFemLdo, LOW);
      _selfCheckError =
          kExpectGc1109 ? "FEM KCT8103L (V4.3+)" : "FEM GC1109 (V4<=4.2)";
    } else if (isGc1109) {
      // GC1109: EN high = enabled; CPS high = full PA (transmit), low on
      // receive (its TX/RX switch is driven by the SX1262's DIO2). No
      // software-controlled LNA.
      pinMode(kPinFemCsd, OUTPUT);
      digitalWrite(kPinFemCsd, HIGH);
      pinMode(kPinFemGc1109Cps, OUTPUT);
      digitalWrite(kPinFemGc1109Cps, LOW);
    } else {
      // KCT8103L: CSD high = enabled; CTX high initially = PA in the
      // transmit path, LNA bypassed on receive. RX routing is then
      // handled by radioRxMode() according to setFemLna(). Careful when
      // the LNA is enabled: its gain adds to the RSSI measured by the
      // SX1262, precisely the figure this device displays.
      pinMode(kPinFemCsd, OUTPUT);
      digitalWrite(kPinFemCsd, HIGH);
      pinMode(kPinFemKctCtx, OUTPUT);
      digitalWrite(kPinFemKctCtx, HIGH);
    }

    delay(150);  // let Vext settle before initializing the OLED
  }

  const char *selfCheckError() const override { return _selfCheckError; }

  void radioTxMode() override {
    if (kExpectGc1109) {
      digitalWrite(kPinFemGc1109Cps, HIGH);
    } else {
      digitalWrite(kPinFemKctCtx, HIGH);
    }
  }

  void radioRxMode() override {
    if (kExpectGc1109) {
      digitalWrite(kPinFemGc1109Cps, LOW);
    } else {
      digitalWrite(kPinFemKctCtx, _femLnaEnabled ? LOW : HIGH);
    }
  }

  bool hasFemLna() const override { return !kExpectGc1109; }

  void setFemLna(bool enabled) override {
    if (!kExpectGc1109) {
      _femLnaEnabled = enabled;
    }
  }

  Display &display() override { return _display; }

  void beginDisplay() override {
    _display.begin();
    _u8g2.setContrast(255);
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
    t.femRxPatch = true;
    return t;
  }

  // Ceiling "at the antenna": same policy for the three variants (the
  // ~12 dB FEM gain is already subtracted before the SX1262 setpoint).
  int8_t txPowerMaxDbm() const override { return 20; }

  const ButtonSpec *buttons(size_t &count) const override {
    count = sizeof(kButtons) / sizeof(kButtons[0]);
    return kButtons;
  }

private:
  const char *_selfCheckError = nullptr;
  bool _femLnaEnabled = false;
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C _u8g2{U8G2_R0, kPinOledReset,
                                            kPinOledScl, kPinOledSda};
  U8g2Display _display{_u8g2};
};

}  // namespace

Board &board() {
  static HeltecV4Board instance;
  return instance;
}
#endif  // BOARD_HELTEC_V4_2 || BOARD_HELTEC_V4_3 || BOARD_HELTEC_V4_R8
