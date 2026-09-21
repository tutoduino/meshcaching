#ifdef BOARD_TDECK
// =====================================================================
// LilyGo T-Deck / T-Deck Plus - ESP32-S3 + SX1262
// ST7789 TFT Display (SPI), I2C Keyboard (0x55), and Physical Trackball.
//
// No PMU (AXP2101 is absent): GPIO10 powers all peripherals.
// One SPI bus (40/38/41) is shared by the radio, TFT, and SD card. It
// must be started explicitly before the TFT driver (see initPower()).
//
// Tuning macros: TDECK_DIO2_RF_SWITCH, TDECK_DISPLAY_ROTATION,
// TDECK_TRACKBALL_EDGES_PER_STEP, TDECK_TRACKBALL_MIN_STEP_MS, TDECK_DEBUG_INPUT
// =====================================================================
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Wire.h>
#include <string.h>

#include "../Board.h"

#ifndef TDECK_DIO2_RF_SWITCH
#define TDECK_DIO2_RF_SWITCH 0
#endif
#ifndef TDECK_DISPLAY_ROTATION
#define TDECK_DISPLAY_ROTATION 3
#endif
#ifndef TDECK_TRACKBALL_EDGES_PER_STEP
#define TDECK_TRACKBALL_EDGES_PER_STEP 2
#endif
#ifndef TDECK_TRACKBALL_MIN_STEP_MS
#define TDECK_TRACKBALL_MIN_STEP_MS 120
#endif
#ifndef TDECK_DEBUG_INPUT
#define TDECK_DEBUG_INPUT 0
#endif

namespace {

// Hardware pin definitions
constexpr uint8_t kPinPeriphPower = 10;
constexpr uint8_t kPinSpiSck = 40;
constexpr uint8_t kPinSpiMiso = 38;
constexpr uint8_t kPinSpiMosi = 41;
constexpr uint8_t kPinSdCs = 39;
constexpr uint8_t kPinLoraNss = 9;
constexpr uint8_t kPinLoraReset = 17;
constexpr uint8_t kPinLoraBusy = 13;
constexpr uint8_t kPinLoraDio1 = 45;
constexpr uint8_t kPinTftCs = 12;
constexpr uint8_t kPinTftDc = 11;
constexpr uint8_t kPinTftBacklight = 42;
constexpr uint8_t kPinI2cSda = 18;
constexpr uint8_t kPinI2cScl = 8;
constexpr uint8_t kKeyboardAddress = 0x55;
constexpr uint8_t kPinTrackballUp = 3;
constexpr uint8_t kPinTrackballDown = 15;
constexpr uint8_t kPinTrackballLeft = 1;
constexpr uint8_t kPinTrackballRight = 2;
constexpr uint8_t kPinTrackballClick = 0;
constexpr uint8_t kPinBatteryAdc = 4;

// The UI assumes a 128x64 display. To fit the 320x240 TFT without a huge
// memory footprint, we render to a 1-bit 128x64 canvas (1 KB) and upscale
// it 2x (to 256x128) on the fly while sending it to the TFT.
constexpr int16_t kLogicalW = 128;
constexpr int16_t kLogicalH = 64;
constexpr int16_t kScale = 2;
constexpr int16_t kScaledW = kLogicalW * kScale;
constexpr int16_t kScaledH = kLogicalH * kScale;
constexpr int16_t kPanelNativeW = 240;
constexpr int16_t kPanelNativeH = 320;
constexpr int16_t kOffX = (kPanelNativeH - kScaledW) / 2;
constexpr int16_t kOffY = (kPanelNativeW - kScaledH) / 2;
constexpr uint32_t kTftSpiHz = 40000000;
constexpr uint16_t kColorOn = 0xFFFF;
constexpr uint16_t kColorOff = 0x0000;
static_assert(kScaledW <= kPanelNativeH && kScaledH <= kPanelNativeW,
              "the scaled frame must fit in the panel");

// Expands a 1-bit row (GFXcanvas1 layout: MSB first) into RGB565 pixels,
// applying the horizontal scale factor.
void expandRow(const uint8_t *src, uint16_t *dst) {
  for (int16_t x = 0; x < kLogicalW; x++) {
    uint16_t color = (src[x >> 3] & (0x80 >> (x & 7))) ? kColorOn : kColorOff;
    for (int16_t k = 0; k < kScale; k++) {
      dst[x * kScale + k] = color;
    }
  }
}

class TDeckDisplay : public Display {
 public:
  void begin() override {
    // Initialize the TFT with a black screen and avoid white flashes during init.
    pinMode(kPinTftBacklight, OUTPUT);
    digitalWrite(kPinTftBacklight, LOW);
    _tft.init(kPanelNativeW, kPanelNativeH);
    _tft.setRotation(TDECK_DISPLAY_ROTATION);
    _tft.setSPISpeed(kTftSpiHz);
    _tft.fillScreen(ST77XX_BLACK);
    _fonts.begin(_canvas);
    _fonts.setFontMode(1);
    _fonts.setForegroundColor(1);
    clear();
    send();
    digitalWrite(kPinTftBacklight, HIGH);
  }

  void clear() override { _canvas.fillScreen(0); }

  void send() override {
    const uint8_t *frame = _canvas.getBuffer();
    constexpr int16_t kRowBytes = (kLogicalW + 7) / 8;
    _tft.startWrite();
    _tft.setAddrWindow(kOffX, kOffY, kScaledW, kScaledH);
    for (int16_t y = 0; y < kLogicalH; y++) {
      expandRow(frame + y * kRowBytes, _line);
      for (int16_t k = 1; k < kScale; k++) {
        memcpy(_line + k * kScaledW, _line, kScaledW * sizeof(uint16_t));
      }
      _tft.writePixels(_line, (uint32_t)kScale * kScaledW);
    }
    _tft.endWrite();
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
    _fonts.setFontMode(inverted ? 0 : 1);
    _fonts.setForegroundColor(_ink);
    _fonts.setBackgroundColor(inverted ? 1 : 0);
  }
  void invertFrame() override {
    uint8_t *frame = _canvas.getBuffer();
    for (int16_t i = 0; i < ((kLogicalW + 7) / 8) * kLogicalH; i++) {
      frame[i] ^= 0xFF;
    }
  }

 private:
  Adafruit_ST7789 _tft{&SPI, kPinTftCs, kPinTftDc, -1};
  GFXcanvas1 _canvas{kLogicalW, kLogicalH};
  U8G2_FOR_ADAFRUIT_GFX _fonts;
  uint16_t _ink = 1;
  uint16_t _line[kScale * kScaledW];
};

// Frees an I2C bus held low by a slave (interrupted transaction).
// This can happen if the ESP32 resets while the keyboard is transmitting.
void recoverI2cBus(uint8_t sda, uint8_t scl) {
  pinMode(sda, INPUT_PULLUP);
  pinMode(scl, OUTPUT_OPEN_DRAIN);
  digitalWrite(scl, HIGH);
  delayMicroseconds(5);
  for (int i = 0; i < 9 && digitalRead(sda) == LOW; i++) {
    digitalWrite(scl, LOW);
    delayMicroseconds(5);
    digitalWrite(scl, HIGH);
    delayMicroseconds(5);
  }
  pinMode(sda, OUTPUT_OPEN_DRAIN);
  digitalWrite(sda, LOW);
  delayMicroseconds(5);
  digitalWrite(sda, HIGH);
  delayMicroseconds(5);
  pinMode(sda, INPUT);
  pinMode(scl, INPUT);
}

// Helper to check if a specific I2C address acknowledges.
bool i2cAck(TwoWire &bus, uint8_t addr) {
  bus.beginTransmission(addr);
  return bus.endTransmission() == 0;
}

// Reads battery voltage via a 1:2 voltage divider on GPIO4. Used for boot logs.
uint16_t batteryMilliVolts() {
  analogReadResolution(12);
  uint32_t raw = 0;
  for (int i = 0; i < 8; i++) raw += analogRead(kPinBatteryAdc);
  raw /= 8;
  return (uint16_t)((2.0f * 3.3f * 1000.0f * (float)raw) / 4096.0f);
}

// Keyboard: 1 ASCII byte per key press (0 = none). Enter 0x0D, Backspace 0x08.
bool mapKeyboardByte(uint8_t code, Key &key) {
  switch (code) {
    case '\r':
    case '\n':
    case ' ':
      key = Key::Ok;
      return true;
    case 0x08:
    case 0x7F:
      key = Key::Back;
      return true;
    default:
      return false;
  }
}

// Trackball: each direction pin toggles at every physical pulse.
// Edges are counted in the ISR and turned into UI steps by pollInput().
struct TrackballAxis {
  uint8_t pin;
  Key key;
  volatile uint16_t edges;
};

TrackballAxis g_axes[4] = {
    {kPinTrackballUp, Key::Up, 0},
    {kPinTrackballDown, Key::Down, 0},
    {kPinTrackballLeft, Key::Left, 0},
    {kPinTrackballRight, Key::Right, 0},
};
constexpr size_t kAxisCount = sizeof(g_axes) / sizeof(g_axes[0]);

void IRAM_ATTR onTrackballEdge(void *arg) {
  TrackballAxis *axis = static_cast<TrackballAxis *>(arg);
  axis->edges = axis->edges + 1;
}

void clearTrackballEdges() {
  for (size_t i = 0; i < kAxisCount; i++) g_axes[i].edges = 0;
}

const ButtonSpec kButtons[] = {
    // Standard GPIO button for the trackball click
    {Key::Ok, kPinTrackballClick, /*activeLow=*/true, /*internalPullup=*/true},
};

class TDeckBoard : public Board {
 public:
  const char *name() const override { return "LilyGo T-Deck"; }

  void initPower() override {
    // Hardware initialization. Order is critical:
    // 1. Power on all peripherals via GPIO10.
    pinMode(kPinPeriphPower, OUTPUT);
    digitalWrite(kPinPeriphPower, HIGH);

    // 2. Set chip selects high to prevent SPI bus contention.
    const uint8_t chipSelects[] = {kPinSdCs, kPinLoraNss, kPinTftCs};
    for (size_t i = 0; i < sizeof(chipSelects); i++) {
      pinMode(chipSelects[i], OUTPUT);
      digitalWrite(chipSelects[i], HIGH);
    }
    pinMode(kPinSpiMiso, INPUT_PULLUP);

    // 3. Initialize the shared SPI bus before the TFT or Radio drivers.
    SPI.begin(kPinSpiSck, kPinSpiMiso, kPinSpiMosi, -1);

    for (size_t i = 0; i < kAxisCount; i++) {
      pinMode(g_axes[i].pin, INPUT_PULLUP);
      attachInterruptArg(g_axes[i].pin, onTrackballEdge, &g_axes[i], CHANGE);
    }

    recoverI2cBus(kPinI2cSda, kPinI2cScl);
    Wire.begin(kPinI2cSda, kPinI2cScl);

    delay(50);
    Serial.printf("Peripherals powered, battery %u mV\n",
                  (unsigned)batteryMilliVolts());
  }

  Display &display() override { return _display; }

  RadioTraits radio() const override {
    RadioTraits t;
    t.pins.nss = kPinLoraNss;
    t.pins.dio1 = kPinLoraDio1;
    t.pins.reset = kPinLoraReset;
    t.pins.busy = kPinLoraBusy;
    t.pins.sck = kPinSpiSck;
    t.pins.miso = kPinSpiMiso;
    t.pins.mosi = kPinSpiMosi;
    // Radio configuration. Note: MeshCore defaults dio2AsRfSwitch to false
    // for the T-Deck, but Meshtastic uses true. Overridable via build flag.
    t.dio2AsRfSwitch = TDECK_DIO2_RF_SWITCH != 0;
    t.tcxoVoltage = 1.8f;
    t.currentLimitmA = 140;
    return t;
  }

  int8_t txPowerMaxDbm() const override { return 22; }

  const ButtonSpec *buttons(size_t &count) const override {
    count = sizeof(kButtons) / sizeof(kButtons[0]);
    return kButtons;
  }

  // Declares to the UI that directional navigation is available (via Trackball).
  bool hasDpad() const override { return true; }

  // Overrides the standard input polling to support the T-Deck's I2C keyboard
  // and physical trackball. Evaluates the keyboard state machine first,
  // then falls back to calculating trackball steps based on ISR edge counts.
  bool pollInput(InputEvent &event) override {
    uint32_t now = millis();
    if (now - _lastKeyboardPollMs >= kKeyboardPollMs) {
      _lastKeyboardPollMs = now;
      if (pollKeyboard(now, event)) return true;
    }
    return pollTrackball(now, event);
  }

 private:
  enum class KeyboardState : uint8_t { Searching, Online, Absent };

  static constexpr uint32_t kKeyboardPollMs = 25;
  static constexpr uint32_t kKeyboardProbeMs = 250;
  static constexpr uint32_t kKeyboardGiveUpMs = 6000;
  static constexpr uint8_t kKeyboardMaxErrors = 20;
  static constexpr uint32_t kTrackballWindowMs = 250;

  bool pollKeyboard(uint32_t now, InputEvent &event) {
    switch (_keyboard) {
      case KeyboardState::Searching:
        if (now - _lastKeyboardProbeMs < kKeyboardProbeMs) return false;
        _lastKeyboardProbeMs = now;
        if (i2cAck(Wire, kKeyboardAddress)) {
          _keyboard = KeyboardState::Online;
          Serial.println(F("Keyboard found (I2C 0x55)"));
        } else if (now > kKeyboardGiveUpMs) {
          _keyboard = KeyboardState::Absent;
          Serial.println(F("Keyboard not found: trackball only"));
        }
        return false;

      case KeyboardState::Online: {
        if (Wire.requestFrom(kKeyboardAddress, (uint8_t)1) != 1) {
          if (++_keyboardErrors >= kKeyboardMaxErrors) {
            _keyboard = KeyboardState::Absent;
            Serial.println(F("Keyboard lost: trackball only"));
          }
          return false;
        }
        _keyboardErrors = 0;
        uint8_t code = (uint8_t)Wire.read();
        if (code == 0) return false;
#if TDECK_DEBUG_INPUT
        Serial.printf("Keyboard: 0x%02X\n", (unsigned)code);
#endif
        Key key;
        if (!mapKeyboardByte(code, key)) return false;
        event.key = key;
        event.longPress = false;
        return true;
      }

      case KeyboardState::Absent:
      default:
        return false;
    }
  }

  bool pollTrackball(uint32_t now, InputEvent &event) {
    if (now - _lastStepMs < TDECK_TRACKBALL_MIN_STEP_MS) {
      clearTrackballEdges();
      return false;
    }
    size_t best = kAxisCount;
    uint16_t bestEdges = 0;
    for (size_t i = 0; i < kAxisCount; i++) {
      uint16_t edges = g_axes[i].edges;
      if (edges > bestEdges) {
        best = i;
        bestEdges = edges;
      }
    }
    if (best == kAxisCount) {
      _edgesSinceMs = 0;
      return false;
    }
    if (_edgesSinceMs == 0) _edgesSinceMs = now;
    if (bestEdges >= TDECK_TRACKBALL_EDGES_PER_STEP) {
#if TDECK_DEBUG_INPUT
      Serial.printf("Trackball: axis %u, %u edges\n", (unsigned)best,
                    (unsigned)bestEdges);
#endif
      event.key = g_axes[best].key;
      event.longPress = false;
      clearTrackballEdges();
      _lastStepMs = now;
      _edgesSinceMs = 0;
      return true;
    }
    if (now - _edgesSinceMs > kTrackballWindowMs) {
      clearTrackballEdges();
      _edgesSinceMs = 0;
    }
    return false;
  }

  KeyboardState _keyboard = KeyboardState::Searching;
  uint8_t _keyboardErrors = 0;
  uint32_t _lastKeyboardPollMs = 0;
  uint32_t _lastKeyboardProbeMs = 0;
  uint32_t _lastStepMs = 0;
  uint32_t _edgesSinceMs = 0;
  TDeckDisplay _display;
};

}  // namespace

Board &board() {
  static TDeckBoard instance;
  return instance;
}
#endif  // BOARD_TDECK
