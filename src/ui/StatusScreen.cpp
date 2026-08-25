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
  // "zZZ" façon émoji sommeil : trois Z croissants en diagonale montante,
  // serrés à un pixel d'écart. L'animation part du vide et les révèle
  // un à un au rythme d'une respiration.
  uint8_t phase = (millis() / 600) % 4;  // 0 = rien d'affiché
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

  // --- Bandeau : répéteur surveillé, bruit de fond, témoin d'émission ---
  _d.setFont(Font::kSmall);
  snprintf(buf, sizeof(buf), "RPT %02X%02X", v.pubkeyPrefix[0],
           v.prefixLen >= 2 ? v.pubkeyPrefix[1] : 0);
  _d.drawText(0, 10, buf);
  if (v.noiseValid) {
    snprintf(buf, sizeof(buf), "NF %d", (int)lroundf(v.noiseDbm));
    _d.drawText(56, 10, buf);
  }
  if (v.txBadge != nullptr) {
    // Taille fixe (calée sur "LBT") pour que LBT -> TX ne fasse pas
    // bouger le bandeau, texte centré ; "OCCUPÉ" s'élargit juste assez.
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

  if (v.rssiValid) {
    // --- RSSI en grand, centré ---
    _d.setFont(Font::kBig);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(v.rssi));
    uint16_t w = _d.textWidth(buf);
    int16_t x = (_d.width() - w) / 2;
    _d.drawText(x, 44, buf);
    _d.setFont(Font::kSmall);
    _d.drawText(x + w + 3, 44, "dBm");
  } else {
    drawSleepLogo();
  }

  // --- SNR du dernier paquet, centré sous le RSSI ---
  if (v.rssiValid) {
    _d.setFont(Font::kSmall);
    int snr10 = (int)lroundf(v.snr * 10.0f);
    snprintf(buf, sizeof(buf), "SNR %s%d.%c dB", snr10 < 0 ? "-" : "",
             abs(snr10) / 10, (char)('0' + abs(snr10) % 10));
    _d.drawText((_d.width() - _d.textWidth(buf)) / 2, 58, buf);
  }

  // --- Barre décroissante : temps avant la prochaine émission possible ---
  if (v.cooldownRemainingMs > 0 && v.cooldownTotalMs > 0) {
    uint16_t barWidth = (uint16_t)((uint32_t)_d.width() *
                                   v.cooldownRemainingMs / v.cooldownTotalMs);
    _d.drawBox(0, 61, barWidth, 3);
  }

  // --- Réponse valide reçue : clignotement par inversion de la trame ---
  if (v.invert) {
    _d.invertFrame();
  }

  _d.send();
}
