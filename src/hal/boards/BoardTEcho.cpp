#ifdef BOARD_TECHO
// =====================================================================
// LilyGo T-Echo - nRF52840 + SX1262, 1.54" 200x200 e-ink (SSD1681:
// GDEH0154D67 on the factory firmware and Meshtastic, DEPG0150BN in
// MeshCore - select with -D TECHO_EINK_MODEL), user button + capacitive
// touch pad, backlight behind the panel.
//
// Pinout from the MeshCore firmware (variants/lilygo_techo, see
// boards/README.md), cross-checked with the LilyGo factory firmware and
// Meshtastic: the Arduino variant (variants/LilyGo_T_Echo) provides the
// LORA_*/SX126X_*/DISP_*/PIN_* macros; SPI is the radio's bus, SPI1 the
// panel's.
//
// First e-ink panel of the project: the logical 128x64 frame is drawn
// 1:1 and centered (200/128 is not an integer, no scaling), black ink
// on white paper. A refresh blocks for ~0.5 s (partial) or ~2.6 s
// (full), so the adapter only refreshes when the frame changed, asks
// the application for a slow cadence (minFrameIntervalMs) and keeps the
// buttons serviced through the idle hook while it waits for the panel.
// A full refresh (clears ghosting) is done at boot, then every
// kMaxPartialsBetweenFull partial refreshes or kFullRefreshEveryMs.
// =====================================================================
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <string.h>

#include "../Board.h"

#ifndef TECHO_EINK_MODEL
#define TECHO_EINK_MODEL GxEPD2_154_D67
#endif
#ifndef TECHO_DISPLAY_ROTATION
#define TECHO_DISPLAY_ROTATION 3  // same mounting as MeshCore
#endif

namespace {

constexpr int8_t kFemTxGainDb = 0;  // no FEM: bare SX1262, 22 dBm max

const ButtonSpec kButtons[] = {
    {Key::Ok, PIN_USER_BTN, /*activeLow=*/true, /*internalPullup=*/true},
};

// Display -> GxEPD2 adapter. The frame is composed in a 1-bit 128x64
// canvas (1 KB) with the U8g2 fonts, then copied into the panel buffer
// (5 KB) as black ink at the center of the paper.
class TEchoDisplay : public Display {
public:
  void begin() override {
    pinMode(DISP_BACKLIGHT, OUTPUT);
    digitalWrite(DISP_BACKLIGHT, LOW);  // off by default; touch pad toggles it
    _epd.epd2.selectSPI(SPI1, SPISettings(4000000, MSBFIRST, SPI_MODE0));
    SPI1.begin();
    _epd.epd2.setBusyCallback(&TEchoDisplay::onBusy, this);
    _epd.init(0, /*initial=*/true, /*reset_duration=*/2, false);
    _epd.setRotation(TECHO_DISPLAY_ROTATION);
    _epd.setFullWindow();
    _fonts.begin(_canvas);
    _fonts.setFontMode(1);  // transparent background
    _fonts.setForegroundColor(1);
    clear();
    // Full refresh of a white page: wipes whatever the previous firmware
    // left on the paper. The next send() (splash) will be partial.
    _epd.fillScreen(GxEPD_WHITE);
    _epd.display(false);
    _partialsSinceFull = 0;
    _lastFullMs = millis();
    memset(_lastSent, 0, sizeof(_lastSent));
    _hasLastSent = true;  // the paper is white = an empty frame
  }

  uint32_t minFrameIntervalMs() const override { return kFrameIntervalMs; }

  void clear() override { _canvas.fillScreen(0); }

  void send() override {
    const uint8_t *frame = _canvas.getBuffer();
    if (_hasLastSent && memcmp(frame, _lastSent, sizeof(_lastSent)) == 0) {
      return;  // nothing changed: spare the panel
    }
    memcpy(_lastSent, frame, sizeof(_lastSent));
    _hasLastSent = true;

    bool full = _partialsSinceFull >= kMaxPartialsBetweenFull ||
                millis() - _lastFullMs >= kFullRefreshEveryMs;
    _epd.fillScreen(GxEPD_WHITE);
    _epd.drawBitmap(kOffX, kOffY, _canvas.getBuffer(), kLogicalW, kLogicalH,
                    GxEPD_BLACK);
    _epd.display(/*partial_update_mode=*/!full);
    if (full) {
      _partialsSinceFull = 0;
      _lastFullMs = millis();
    } else {
      _partialsSinceFull++;
    }
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
    _fonts.drawUTF8(x, y, utf8);
  }
  uint16_t textWidth(const char *utf8) override {
    return _fonts.getUTF8Width(utf8);
  }
  void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) override {
    _canvas.fillRect(x, y, w, h, _ink);
  }
  void drawHLine(int16_t x, int16_t y, int16_t w) override {
    _canvas.drawFastHLine(x, y, w, _ink);
  }
  void setInkInverted(bool inverted) override {
    _ink = inverted ? 0 : 1;
    // Inverted text rendered in "solid" mode: the library paints the
    // glyph background itself, instead of the transparent mode on top
    // of the already filled box.
    _fonts.setFontMode(inverted ? 0 : 1);
    _fonts.setForegroundColor(_ink);
    _fonts.setBackgroundColor(inverted ? 1 : 0);
  }
  void invertFrame() override {
    uint8_t *frame = _canvas.getBuffer();
    for (size_t i = 0; i < sizeof(_lastSent); i++) {
      frame[i] ^= 0xFF;
    }
  }

  void setBacklight(bool on) { digitalWrite(DISP_BACKLIGHT, on ? HIGH : LOW); }

private:
  static constexpr int16_t kLogicalW = 128;
  static constexpr int16_t kLogicalH = 64;
  static constexpr int16_t kPanelW = TECHO_EINK_MODEL::WIDTH;
  static constexpr int16_t kPanelH = TECHO_EINK_MODEL::HEIGHT;
  static constexpr int16_t kOffX = (kPanelW - kLogicalW) / 2;
  static constexpr int16_t kOffY = (kPanelH - kLogicalH) / 2;
  static constexpr uint32_t kFrameIntervalMs = 1000;
  static constexpr uint16_t kMaxPartialsBetweenFull = 30;
  static constexpr uint32_t kFullRefreshEveryMs = 10UL * 60UL * 1000UL;
  static_assert(kPanelW >= kLogicalW && kPanelH >= kLogicalH,
                "the logical frame must fit in the panel");

  // GxEPD2 polls BUSY in a loop and calls this between two samples.
  static void onBusy(const void *self) {
    const_cast<TEchoDisplay *>(static_cast<const TEchoDisplay *>(self))->idle();
  }

  GxEPD2_BW<TECHO_EINK_MODEL, TECHO_EINK_MODEL::HEIGHT> _epd{
      TECHO_EINK_MODEL(DISP_CS, DISP_DC, DISP_RST, DISP_BUSY)};
  GFXcanvas1 _canvas{kLogicalW, kLogicalH};
  U8G2_FOR_ADAFRUIT_GFX _fonts;
  uint16_t _ink = 1;
  uint8_t _lastSent[(kLogicalW / 8) * kLogicalH];
  bool _hasLastSent = false;
  uint16_t _partialsSinceFull = 0;
  uint32_t _lastFullMs = 0;
};

class TEchoBoard : public Board {
public:
  const char *name() const override { return "LilyGo T-Echo"; }

  void initPower() override {
    // Peripheral rail (already raised by initVariant(), made explicit)
    pinMode(PIN_PWR_EN, OUTPUT);
    digitalWrite(PIN_PWR_EN, HIGH);

    // LEDs off (active low), GPS left asleep: neither is used here.
    const uint8_t leds[] = {LED_RED, LED_GREEN, LED_BLUE};
    for (size_t i = 0; i < sizeof(leds); i++) {
      pinMode(leds[i], OUTPUT);
      digitalWrite(leds[i], HIGH);
    }
    pinMode(GPS_EN, OUTPUT);
    digitalWrite(GPS_EN, LOW);

    // Touch pad: digital output of the TTP223, HIGH while touched.
    pinMode(PIN_TOUCH, INPUT_PULLDOWN);

    delay(50);  // rail settling before the SX1262 and the panel
  }

  Display &display() override { return _display; }

  RadioTraits radio() const override {
    RadioTraits t;
    t.pins.nss = LORA_CS;
    t.pins.dio1 = SX126X_DIO1;
    t.pins.reset = SX126X_RESET;
    t.pins.busy = SX126X_BUSY;
    // sck/miso/mosi left at -1: default SPI bus of the variant
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

  // The touch pad is handled here, not as a logical key: a tap toggles
  // the backlight (useful at night on a paper-like panel). Never
  // produces an application event.
  bool pollInput(InputEvent & /*event*/) override {
    bool touched = digitalRead(PIN_TOUCH) == HIGH;
    uint32_t now = millis();
    if (touched != _touchRaw) {
      _touchRaw = touched;
      _touchEdgeMs = now;
    }
    if (_touchRaw != _touchStable && now - _touchEdgeMs >= kTouchDebounceMs) {
      _touchStable = _touchRaw;
      if (_touchStable) {
        _backlight = !_backlight;
        _display.setBacklight(_backlight);
      }
    }
    return false;
  }

private:
  static constexpr uint32_t kTouchDebounceMs = 50;

  TEchoDisplay _display;
  bool _touchRaw = false;
  bool _touchStable = false;
  uint32_t _touchEdgeMs = 0;
  bool _backlight = false;
};

}  // namespace

Board &board() {
  static TEchoBoard instance;
  return instance;
}
#endif  // BOARD_TECHO
