/**
 * @file    main.cpp
 * @brief   Geolocalisation d'un repeteur MeshCore — point d'entree.
 *
 * @details Affiche le RSSI (niveau de signal) et le temps ecoule depuis la
 *          derniere reception d'un paquet provenant d'un repeteur MeshCore
 *          specifique. Voir README.md pour les cartes supportees et
 *          l'organisation du code.
 *
 * @see     https://tutoduino.fr/menu-sdr/geolocalisation-repeteur-meshcore/
 */
#include <Arduino.h>

#include "app/App.h"
#include "hal/Board.h"

#if !defined(BOARD_WIO_TRACKER_L1) && !defined(BOARD_HELTEC_V3) && \
    !defined(BOARD_HELTEC_V4)
#error "Aucune carte selectionnee : compiler via un environnement PlatformIO (pio run -e wio_tracker_l1 | heltec_v3 | heltec_v4)"
#endif

// Construction a la premiere utilisation : evite de dependre de l'ordre
// d'initialisation des objets statiques entre unites de compilation.
static App &app() {
  static App instance(board());
  return instance;
}

void setup() {
  app().setup();
}

void loop() {
  app().loop();
}
