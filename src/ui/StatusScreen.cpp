#include "StatusScreen.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void StatusScreen::showMessage(const char *line1, const char *line2) {
  _d.clearBuffer();
  _d.setFont(u8g2_font_6x12_tf);
  _d.drawUTF8(0, 12, line1);
  _d.drawUTF8(0, 26, line2);
  _d.sendBuffer();
}

void StatusScreen::drawStatus(float rssi, float snr, uint32_t ageSeconds,
                              const uint8_t *pubkeyPrefix, size_t prefixLen) {
  _d.clearBuffer();
  char buf[24];

  // --- Titre : identifiant du répéteur surveillé ---
  _d.setFont(u8g2_font_6x12_tf);
  snprintf(buf, sizeof(buf), "RÉPÉTEUR %02X%02X", pubkeyPrefix[0],
           prefixLen >= 2 ? pubkeyPrefix[1] : 0);
  _d.drawUTF8(0, 10, buf);

  // --- RSSI en grand, centré horizontalement ---
  _d.setFont(u8g2_font_logisoso24_tr);
  snprintf(buf, sizeof(buf), "%d", (int)lroundf(rssi));
  u8g2_uint_t w = _d.getUTF8Width(buf);
  u8g2_uint_t x = (_d.getDisplayWidth() - w) / 2;
  _d.drawUTF8(x, 48, buf);
  _d.setFont(u8g2_font_6x12_tf);
  _d.drawUTF8(x + w + 3, 48, "dBm");

  // --- SNR et temps écoulé depuis le dernier paquet ---
  int snr10 = (int)lroundf(snr * 10.0f);
  snprintf(buf, sizeof(buf), "SNR:%s%d.%cdB", snr10 < 0 ? "-" : "",
           abs(snr10) / 10, (char)('0' + abs(snr10) % 10));
  _d.drawUTF8(0, 63, buf);
  snprintf(buf, sizeof(buf), "%lus", (unsigned long)ageSeconds);
  _d.drawUTF8(_d.getDisplayWidth() - _d.getUTF8Width(buf), 63, buf);

  _d.sendBuffer();
}
