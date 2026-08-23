Définitions de cartes reprises du firmware MeshCore
(https://github.com/meshcore-dev/MeshCore, licence MIT) :

- Seeed Wio Tracker L1 : `boards/seeed-wio-tracker-l1.json`,
  `boards/nrf52840_s140_v7.ld` et `variants/Seeed_Wio_Tracker_L1/variant.{h,cpp}` ;
- Heltec WiFi LoRa 32 V4 : `boards/heltec_v4.json` et
  `variants/heltec_v4/pins_arduino.h` (le brochage LoRa/FEM/OLED utilisé dans
  `src/hal/boards/BoardHeltecV4.cpp` en est également repris).

Le Heltec V3 utilise la définition `heltec_wifi_lora_32_V3` fournie par la
plateforme PlatformIO `espressif32`, rien à embarquer ici.
