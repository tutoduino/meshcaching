#pragma once
#include <stddef.h>
#include <stdint.h>

#include "../hal/Display.h"
#include "../hal/Settings.h"

// State to render on the main screen, composed by the application.
struct MainView {
  const uint8_t *pubkeyPrefix;   // monitored repeater (header)
  size_t prefixLen;
  bool rssiValid;                // false: sleep logo instead of the RSSI
  float rssi;                    // RssiPkt, averaged over the packet
  float despreadRssi;            // SignalRssiPkt, after despreading
  float snr;
  bool noiseValid;               // measured noise floor (median) available?
  float noiseDbm;
  const char *txBadge;           // transmit indicator ("LBT", "TX",
                                 // "OCCUPÉ"...); nullptr = none
  bool autoPing;                 // automatic TRACE ping enabled                                 
  bool invert;                   // blinking: inverted frame
  uint32_t cooldownRemainingMs;  // until the next transmission (0 = ready)
  uint32_t cooldownTotalMs;
};

// Application screens, drawn through the Display abstraction (U8g2
// OLED or TFT depending on the board).
class StatusScreen {
public:
  explicit StatusScreen(Display &display) : _d(display) {}

  void showMessage(const char *line1, const char *line2 = "");

  // Splash screen: MESHCACHING in large type, version underneath.
  void showSplash(const char *version);

  // Main screen: large RSSI while it is fresh, otherwise the sleep logo
  // (zZZ); transmit indicator, transmit cooldown bar, flash.
  void drawMain(const MainView &view);

private:
  void drawSleepLogo();

  Display &_d;
};
