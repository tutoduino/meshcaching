/**
 * @file    main.cpp
 * @brief   Géolocalisation d'un répéteur MeshCore — point d'entrée.
 *
 * @details Affiche le RSSI (niveau de signal) et le temps écoulé depuis la
 *          dernière réception d'un paquet provenant d'un répéteur MeshCore
 *          spécifique. Voir README.md pour les cartes supportées et
 *          l'organisation du code.
 *
 * @see     https://tutoduino.fr/menu-sdr/géolocalisation-répéteur-meshcore/
 */
#include <Arduino.h>

#include "app/App.h"
#include "hal/Board.h"

#if !defined(BOARD_WIO_TRACKER_L1) && !defined(BOARD_HELTEC_V3) && \
    !defined(BOARD_HELTEC_V4_3) && !defined(BOARD_HELTEC_V4_R8) && \
    !defined(BOARD_HELTEC_T096)
#error "Aucune carte sélectionnée : compiler via un environnement PlatformIO (cf. platformio.ini)"
#endif

// Construction à la première utilisation : évite de dépendre de l'ordre
// d'initialisation des objets statiques entre unités de compilation.
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
