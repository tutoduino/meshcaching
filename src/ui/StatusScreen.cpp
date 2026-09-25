#include "StatusScreen.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void StatusScreen::showMessage(const char *line1, const char *line2) {
  _d.clear();
  _d.setFont(Font::kSmall);
  _d.drawText(0, 12, line1);
  _d.drawText(0, 26, line2);
  _d.send();
}

void StatusScreen::showSplash(const char *version) {
  _d.clear();
  _d.setFont(Font::kMedium);
  uint16_t w = _d.textWidth("MESHCACHING");
  int16_t x = w < _d.width() ? (_d.width() - w) / 2 : 0;
  _d.drawText(x, 32, "MESHCACHING");
  _d.setFont(Font::kSmall);
  w = _d.textWidth(version);
  x = w < _d.width() ? (_d.width() - w) / 2 : 0;
  _d.drawText(x, 50, version);
  _d.send();
}

void StatusScreen::drawSleepLogo() {
  // "zZZ" like a sleep emoji: three growing Z's on a rising diagonal,
  // packed one pixel apart. The animation starts from nothing and
  // reveals them one by one at a breathing pace - except on slow panels
  // (e-ink), where the logo is simply drawn complete.
  uint8_t phase = 3;
  if (_d.minFrameIntervalMs() == 0) {
    phase = (millis() / 600) % 4;  // 0 = nothing shown
  }
  if (phase == 0) {
    return;
  }
  _d.setFont(Font::kSmall);
  uint16_t wSmall = _d.textWidth("z");
  _d.setFont(Font::kMedium);
  uint16_t wMedium = _d.textWidth("Z");
  _d.setFont(Font::kBig);
  uint16_t wLarge = _d.textWidth("Z");
  int16_t x = (_d.width() - (wSmall + wMedium + wLarge + 2)) / 2;

  _d.setFont(Font::kSmall);
  _d.drawText(x, 54, "z");
  if (phase >= 2) {
    _d.setFont(Font::kMedium);
    _d.drawText(x + wSmall + 1, 50, "Z");
  }
  if (phase >= 3) {
    _d.setFont(Font::kBig);
    _d.drawText(x + wSmall + 1 + wMedium + 1, 46, "Z");
  }
}

void StatusScreen::drawMain(const MainView &v) {
  _d.clear();
  char buf[24];

  // --- Header: monitored repeater, noise floor, transmit indicator ---
  _d.setFont(Font::kSmall);
  snprintf(buf, sizeof(buf), "RPT %02X%02X", v.pubkeyPrefix[0],
           v.prefixLen >= 2 ? v.pubkeyPrefix[1] : 0);
  _d.drawText(0, 10, buf);
  if (v.noiseValid) {
    snprintf(buf, sizeof(buf), "NF %d", (int)lroundf(v.noiseDbm));
    _d.drawText(56, 10, buf);
  }
  if (v.txBadge != nullptr) {
    // Fixed size (based on "LBT") so that LBT -> TX does not shift the
    // header, with centered text; "OCCUPÉ" widens just enough.
    uint16_t textWidth = _d.textWidth(v.txBadge);
    uint16_t badgeWidth = _d.textWidth("LBT") + 6;
    if (textWidth + 6 > badgeWidth) {
      badgeWidth = textWidth + 6;
    }
    int16_t badgeX = _d.width() - badgeWidth;
    _d.drawBox(badgeX, 0, badgeWidth, 12);
    _d.setInkInverted(true);
    _d.drawText(badgeX + (badgeWidth - textWidth) / 2, 10, v.txBadge);
    _d.setInkInverted(false);
  }
  _d.drawHLine(0, 13, _d.width());

  if (v.rssiValid && v.rssiDisplay == RssiDisplayMode::kBoth) {
    // --- Average RSSI (left) and despread RSSI (right), side by side,
    // each centered in its own half of the screen ---
    const int16_t half = _d.width() / 2;
    _d.setFont(Font::kSmall);
    _d.drawText((half - _d.textWidth("RSSI")) / 2, 25, "RSSI");
    _d.drawText(half + (half - _d.textWidth("DESPREAD")) / 2, 25, "DESPREAD");
    _d.setFont(Font::kMenu);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(v.rssi));
    _d.drawText((half - _d.textWidth(buf)) / 2, 45, buf);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(v.despreadRssi));
    _d.drawText(half + (half - _d.textWidth(buf)) / 2, 45, buf);
    _d.drawBox(half - 1, 17, 1, 30);  // separator between the columns
  } else if (v.rssiValid) {
    // --- A single value, large and untitled: all the focus on it ---
    float value = v.rssiDisplay == RssiDisplayMode::kDespreadOnly
                      ? v.despreadRssi
                      : v.rssi;
    _d.setFont(Font::kBig);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(value));
    uint16_t w = _d.textWidth(buf);
    int16_t x = (_d.width() - w) / 2;
    _d.drawText(x, 44, buf);
    _d.setFont(Font::kSmall);
    _d.drawText(x + w + 3, 44, "dBm");
  } else {
    drawSleepLogo();
  }

  // --- SNR of the last packet, centered under the RSSI ---
  if (v.rssiValid) {
    _d.setFont(Font::kSmall);
    int snr10 = (int)lroundf(v.snr * 10.0f);
    snprintf(buf, sizeof(buf), "SNR %s%d.%c dB", snr10 < 0 ? "-" : "",
             abs(snr10) / 10, (char)('0' + abs(snr10) % 10));
    _d.drawText((_d.width() - _d.textWidth(buf)) / 2, 58, buf);
  }

  // --- Shrinking bar: time until the next transmission is allowed ---
  if (v.cooldownRemainingMs > 0 && v.cooldownTotalMs > 0) {
    uint16_t barWidth = (uint16_t)((uint32_t)_d.width() *
                                   v.cooldownRemainingMs / v.cooldownTotalMs);
    _d.drawBox(0, 61, barWidth, 3);
  }

  // --- Valid reply received: blink by inverting the frame (pointless
  // on a slow panel: a refresh would only catch it by chance) ---
  if (v.invert && _d.minFrameIntervalMs() == 0) {
    _d.invertFrame();
  }

  _d.send();
}
