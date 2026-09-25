/**
 * @file    main.cpp
 * @brief   MeshCore repeater geolocation - entry point.
 *
 * @details Displays the RSSI (signal level) and the time elapsed since the
 *          last packet received from a specific MeshCore repeater. See
 *          README.md for the supported boards and the code organization.
 *
 * @see     https://tutoduino.fr/menu-sdr/géolocalisation-répéteur-meshcore/
 */
#include <Arduino.h>

#include "app/App.h"
#include "hal/Board.h"

#if !defined(BOARD_WIO_TRACKER_L1) && !defined(BOARD_HELTEC_V3) && \
    !defined(BOARD_HELTEC_V4_2) && !defined(BOARD_HELTEC_V4_3) && \
    !defined(BOARD_HELTEC_V4_R8) && !defined(BOARD_HELTEC_T096) && \
    !defined(BOARD_TBEAM_SUPREME) && !defined(BOARD_TDECK)
#error "No board selected: build through a PlatformIO environment (see platformio.ini)"
#endif

// Constructed on first use: avoids depending on the initialization
// order of static objects across translation units.
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
