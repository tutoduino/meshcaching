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

void StatusScreen::drawScanLogo() {
  // Ondes concentriques émises par un point : le scan est en cours.
  // La phase avance avec le temps pour animer la propagation.
  const u8g2_uint_t cx = _d.getDisplayWidth() / 2;
  const u8g2_uint_t cy = 47;
  uint8_t phase = (millis() / 350) % 3;
  _d.drawDisc(cx, cy, 3);
  for (uint8_t i = 0; i <= phase; i++) {
    _d.drawCircle(cx, cy, 10 + i * 8,
                  U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
  }
}

void StatusScreen::drawMain(const MainView &v) {
  _d.clearBuffer();
  char buf[24];

  // --- Bandeau : répéteur surveillé + témoin d'émission ---
  _d.setFont(u8g2_font_6x12_tf);
  snprintf(buf, sizeof(buf), "RÉPÉTEUR %02X%02X", v.pubkeyPrefix[0],
           v.prefixLen >= 2 ? v.pubkeyPrefix[1] : 0);
  _d.drawUTF8(0, 10, buf);
  if (v.txBadge != nullptr) {
    u8g2_uint_t badgeWidth = _d.getUTF8Width(v.txBadge) + 6;
    u8g2_uint_t badgeX = _d.getDisplayWidth() - badgeWidth;
    _d.drawBox(badgeX, 0, badgeWidth, 12);
    _d.setDrawColor(0);
    _d.drawUTF8(badgeX + 3, 10, v.txBadge);
    _d.setDrawColor(1);
  }
  _d.drawHLine(0, 13, _d.getDisplayWidth());

  if (v.rssiValid) {
    // --- RSSI en grand, centré ---
    _d.setFont(u8g2_font_logisoso24_tr);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(v.rssi));
    u8g2_uint_t w = _d.getUTF8Width(buf);
    u8g2_uint_t x = (_d.getDisplayWidth() - w) / 2;
    _d.drawUTF8(x, 44, buf);
    _d.setFont(u8g2_font_6x12_tf);
    _d.drawUTF8(x + w + 3, 44, "dBm");
  } else {
    drawScanLogo();
  }

  // --- Ligne d'infos : bruit de fond à gauche, SNR à droite ---
  _d.setFont(u8g2_font_6x12_tf);
  if (v.noiseValid) {
    snprintf(buf, sizeof(buf), "NF %d", (int)lroundf(v.noiseDbm));
    _d.drawUTF8(0, 58, buf);
  }
  if (v.rssiValid) {
    int snr10 = (int)lroundf(v.snr * 10.0f);
    snprintf(buf, sizeof(buf), "SNR %s%d.%c", snr10 < 0 ? "-" : "",
             abs(snr10) / 10, (char)('0' + abs(snr10) % 10));
    _d.drawUTF8(_d.getDisplayWidth() - _d.getUTF8Width(buf), 58, buf);
  }

  // --- Barre décroissante : temps avant la prochaine émission possible ---
  if (v.cooldownRemainingMs > 0 && v.cooldownTotalMs > 0) {
    u8g2_uint_t barWidth = (u8g2_uint_t)((uint32_t)_d.getDisplayWidth() *
                                         v.cooldownRemainingMs /
                                         v.cooldownTotalMs);
    _d.drawBox(0, 61, barWidth, 3);
  }

  // --- Réponse valide reçue : clignotement par inversion de la trame ---
  if (v.invert) {
    _d.setDrawColor(2);  // XOR
    _d.drawBox(0, 0, _d.getDisplayWidth(), _d.getDisplayHeight());
    _d.setDrawColor(1);
  }

  _d.sendBuffer();
}
