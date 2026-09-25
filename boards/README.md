Définitions de cartes reprises du firmware MeshCore
(https://github.com/meshcore-dev/MeshCore, licence MIT) :

- Seeed Wio Tracker L1 : `boards/seeed-wio-tracker-l1.json`,
  `boards/nrf52840_s140_v7.ld` et `variants/Seeed_Wio_Tracker_L1/variant.{h,cpp}` ;
- Heltec WiFi LoRa 32 V4 (4.3) : `boards/heltec_v4.json` et
  `variants/heltec_v4/pins_arduino.h` (le brochage LoRa/FEM/OLED utilisé dans
  `src/hal/boards/BoardHeltecV4.cpp` en est également repris) ;
- Heltec WiFi LoRa 32 V4 R8 : `boards/heltec_v4_r8.json` et
  `variants/heltec_v4_r8/pins_arduino.h` ;
- Heltec T096 : `boards/heltec_t096.json`, `boards/nrf52840_s140_v6.ld`
  et `variants/Heltec_T096_Board/variant.{h,cpp}` ;
- LilyGo T-Beam Supreme (SX1262) : `boards/lilygo_tbeam_supreme.json` (le
  brochage LoRa / I2C et l'affectation des rails du PMU AXP2101 utilisés dans
  `src/hal/boards/BoardTBeamSupreme.cpp` en sont également repris, recoupés
  avec les exemples LilyGo-LoRa-Series) ;
- LilyGo T-Deck : `boards/t-deck.json` (brochage repris de
  `variants/lilygo_tdeck`, recoupé avec Meshtastic) ;
- LilyGo T-Echo : `boards/lilygo_t_echo.json` et
  `variants/LilyGo_T_Echo/variant.{h,cpp}` (repris de `variants/lilygo_techo`,
  recoupés avec le firmware d'usine LilyGo et Meshtastic).

Le Heltec V3 utilise la définition `heltec_wifi_lora_32_V3` fournie par la
plateforme PlatformIO `espressif32`, rien à embarquer ici.
