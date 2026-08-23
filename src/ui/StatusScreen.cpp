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

void StatusScreen::showSplash(const char *version) {
  _d.clearBuffer();
  _d.setFont(u8g2_font_helvB12_tr);
  u8g2_uint_t w = _d.getUTF8Width("MESHCACHING");
  u8g2_uint_t x = w < _d.getDisplayWidth() ? (_d.getDisplayWidth() - w) / 2 : 0;
  _d.drawUTF8(x, 32, "MESHCACHING");
  _d.setFont(u8g2_font_6x12_tf);
  w = _d.getUTF8Width(version);
  x = w < _d.getDisplayWidth() ? (_d.getDisplayWidth() - w) / 2 : 0;
  _d.drawUTF8(x, 50, version);
  _d.sendBuffer();
}

void StatusScreen::drawSleepLogo() {
  // "zZZ" façon émoji sommeil : trois Z croissants en diagonale montante,
  // révélés un à un au rythme d'une respiration.
  uint8_t phase = (millis() / 600) % 3;
  _d.setFont(u8g2_font_6x12_tf);
  _d.drawUTF8(42, 54, "z");
  if (phase >= 1) {
    _d.setFont(u8g2_font_helvB12_tr);
    _d.drawUTF8(54, 46, "Z");
  }
  if (phase >= 2) {
    _d.setFont(u8g2_font_logisoso24_tr);
    _d.drawUTF8(70, 40, "Z");
  }
}

void StatusScreen::drawMain(const MainView &v) {
  _d.clearBuffer();
  char buf[24];

  // --- Bandeau : répéteur surveillé, bruit de fond, témoin d'émission ---
  _d.setFont(u8g2_font_6x12_tf);
  snprintf(buf, sizeof(buf), "RPT %02X%02X", v.pubkeyPrefix[0],
           v.prefixLen >= 2 ? v.pubkeyPrefix[1] : 0);
  _d.drawUTF8(0, 10, buf);
  if (v.noiseValid) {
    snprintf(buf, sizeof(buf), "NF %d", (int)lroundf(v.noiseDbm));
    _d.drawUTF8(56, 10, buf);
  }
  if (v.txBadge != nullptr) {
    // Taille fixe (calée sur "LBT") pour que LBT -> TX ne fasse pas
    // bouger le bandeau, texte centré ; "OCCUPÉ" s'élargit juste assez.
    u8g2_uint_t textWidth = _d.getUTF8Width(v.txBadge);
    u8g2_uint_t badgeWidth = _d.getUTF8Width("LBT") + 6;
    if (textWidth + 6 > badgeWidth) {
      badgeWidth = textWidth + 6;
    }
    u8g2_uint_t badgeX = _d.getDisplayWidth() - badgeWidth;
    _d.drawBox(badgeX, 0, badgeWidth, 12);
    _d.setDrawColor(0);
    _d.drawUTF8(badgeX + (badgeWidth - textWidth) / 2, 10, v.txBadge);
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
    drawSleepLogo();
  }

  // --- SNR du dernier paquet, centré sous le RSSI ---
  if (v.rssiValid) {
    _d.setFont(u8g2_font_6x12_tf);
    int snr10 = (int)lroundf(v.snr * 10.0f);
    snprintf(buf, sizeof(buf), "SNR %s%d.%c dB", snr10 < 0 ? "-" : "",
             abs(snr10) / 10, (char)('0' + abs(snr10) % 10));
    _d.drawUTF8((_d.getDisplayWidth() - _d.getUTF8Width(buf)) / 2, 58, buf);
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
