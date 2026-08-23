# meshcaching

Géolocalisation d'un répéteur [MeshCore](https://github.com/meshcore-dev/MeshCore) :
l'appareil affiche le RSSI (niveau de signal) et le temps écoulé depuis la
dernière réception d'un paquet provenant d'un répéteur donné, et permet de le
« pinger » avec un paquet TRACE (le ping natif de MeshCore) via le bouton.

Basé sur le sketch de [Tutoduino](https://tutoduino.fr/menu-sdr/geolocalisation-repeteur-meshcore/),
restructuré en projet PlatformIO multi-cartes.

## Cartes supportées

| Environnement    | Carte                          | MCU        | Radio            | Écran          | Boutons          | TX défaut / max |
|------------------|--------------------------------|------------|------------------|----------------|------------------|-----------------|
| `wio_tracker_l1` | Seeed Wio Tracker L1 Pro       | nRF52840   | SX1262           | SH1106 128×64  | croix + 2 boutons| 22 / 22 dBm     |
| `heltec_v3`      | Heltec WiFi LoRa 32 V3         | ESP32-S3   | SX1262           | SSD1306 128×64 | 1 bouton (PRG)   | 22 / 22 dBm     |
| `heltec_v4`      | Heltec WiFi LoRa 32 V4 (4.3)   | ESP32-S3R2 | SX1262 + FEM     | SSD1306 128×64 | 1 bouton (PRG)   | 20 / 20 dBm     |
| `heltec_v4_r8`   | Heltec WiFi LoRa 32 V4 « R8 »  | ESP32-S3R8 | SX1262 + FEM     | SSD1306 128×64 | 1 bouton (PRG)   | 20 / 20 dBm     |

Sur les V4 (4.3 et R8), la puissance est exprimée « à l'antenne » : le FEM
KCT8103L ajoute ~12 dB en émission, retranchés automatiquement de la consigne
passée au SX1262. Non gérés : les V4 antérieurs à la 4.3 (FEM GC1109) et les
déclinaisons TFT / e-ink.

## Compilation

```sh
task build                     # toutes les cartes
task build TARGET=heltec_v3    # une seule
task upload TARGET=heltec_v3   # téléverse (défaut : wio_tracker_l1)
task flash TARGET=heltec_v3    # téléverse + moniteur série
```

Ou directement : `pio run -e <env>`.

Le preset radio (par défaut : MeshCore Île-de-France, 869.618 MHz) se règle
dans `src/AppConfig.h` ; le répéteur visé s'y trouve aussi (`kTargetPubkeyPrefix`)
mais seulement comme valeur d'usine, modifiable ensuite via le menu.

## Utilisation

Écran principal : RSSI du répéteur cible, SNR et ancienneté du dernier paquet.

- **Ping TRACE** : appui court sur Ok (joystick sur le L1, bouton PRG sur les
  Heltec).
- **Menu** : bouton Menu sur le L1, appui long sur PRG sur les Heltec.

Trois réglages, persistés (NVS sur ESP32, LittleFS interne sur nRF52) :

- **Répéteur** : préfixe de clé publique, 4 digits hex édités un par un ;
- **Puiss. TX** : puissance d'émission « à l'antenne », de −9 dBm au maximum
  de la carte ;
- **Gain RX** : `AUCUN` / `RX BOOST` (le +2 dB interne du SX126x, défaut
  partout) / `FEM LNA` (Heltec V4.3 uniquement, exclusif du boost). Attention :
  le LNA du FEM ajoute son gain au RSSI affiché.

Navigation : sur le L1, Up/Down navigue ou modifie, Left/Right change de
digit, Ok valide, Back annule l'édition ou sort du menu. Sur les Heltec
(bouton unique) : clic = suivant/modifier, appui long = valider, sortie par
l'item « Retour ». Dans les deux cas, 20 s d'inactivité referment le menu ;
les changements sont appliqués et sauvegardés à la fermeture.

## Organisation du code

```
src/
├── main.cpp          point d'entrée (délègue à App)
├── AppConfig.h       config applicative : preset radio, défauts d'usine
├── app/              logique applicative (App)
├── mesh/             protocole MeshCore pur (parsing, TRACE) — aucune
│                     dépendance matérielle
├── ui/               écrans U8g2 (pilote commun SH1106/SSD1306) :
│                     StatusScreen + SettingsMenu
└── hal/
    ├── Board.h       interface d'une carte : écran, brochage radio,
    │                 boutons logiques, puissance TX défaut/max, FEM
    ├── Radio.*       enveloppe SX1262/RadioLib (FEM, ISR, puissance,
    │                 gain RX)
    ├── Buttons.*     boutons débouncés -> touches logiques (Ok, Back,
    │                 Up/Down/Left/Right), clic court / appui long
    ├── Settings.*    config persistée (NVS ESP32 / LittleFS nRF52)
    ├── SysRandom.*   RNG matériel (ESP32 / nRF52)
    └── boards/       une implémentation de Board par carte
```

Chaque environnement PlatformIO définit un drapeau `-D BOARD_xxx` qui
sélectionne l'unique fichier `src/hal/boards/*.cpp` compilé ; celui-ci décrit
tout ce qui est propre à la carte (rails d'alimentation, écran, brochage
radio, boutons, bornes de puissance). L'application ne voit que l'interface
`Board`.

### Ajouter une carte

1. créer `src/hal/boards/BoardMaCarte.cpp` (implémentation de `Board`,
   gardée par `#ifdef BOARD_MA_CARTE`) ;
2. ajouter un `[env:ma_carte]` dans `platformio.ini` avec
   `-D BOARD_MA_CARTE` — et, si la carte n'est pas connue de PlatformIO,
   sa définition dans `boards/` (+ variante dans `variants/`).

## Crédits

- Sketch d'origine : [Tutoduino](https://tutoduino.fr/menu-sdr/geolocalisation-repeteur-meshcore/)
- Définitions de cartes et brochages : firmware
  [MeshCore](https://github.com/meshcore-dev/MeshCore) (licence MIT),
  cf. `boards/README.md`
