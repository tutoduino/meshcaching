#include "SysRandom.h"

#include <Arduino.h>

#if defined(ESP32)

#include <esp_system.h>

uint32_t sysRandom32() {
  return esp_random();
}

#elif defined(ARDUINO_ARCH_NRF52) || defined(NRF52_SERIES)

// Lecture directe du peripherique RNG, permise tant que la pile BLE
// (SoftDevice) n'est pas activee — c'est le cas ici.
uint32_t sysRandom32() {
  uint32_t value = 0;
  NRF_RNG->TASKS_START = 1;
  for (int i = 0; i < 4; i++) {
    NRF_RNG->EVENTS_VALRDY = 0;
    while (NRF_RNG->EVENTS_VALRDY == 0) {}
    value = (value << 8) | (uint8_t)(NRF_RNG->VALUE);
  }
  NRF_RNG->TASKS_STOP = 1;
  return value;
}

#else
#error "sysRandom32() : plateforme non geree"
#endif
