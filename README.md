# meshcaching

Géolocalisation d'un répéteur [MeshCore](https://github.com/meshcore-dev/MeshCore) :
l'appareil affiche le RSSI (niveau de signal) et le temps écoulé depuis la
dernière réception d'un paquet provenant d'un répéteur donné, et permet de le
« pinger » avec un paquet TRACE (le ping natif de MeshCore) via le bouton.

Basé sur le sketch de [Tutoduino](https://tutoduino.fr/menu-sdr/geolocalisation-repeteur-meshcore/),
restructuré en projet PlatformIO multi-cartes.

## Cartes supportées

| Environnement    | Carte                        | MCU        | Radio            | Écran          | Boutons          | TX défaut / max |
|------------------|------------------------------|------------|------------------|----------------|------------------|-----------------|
| `wio_tracker_l1` | Seeed Wio Tracker L1 Pro     | nRF52840   | SX1262           | SH1106 128×64  | croix + 2 boutons| 22 / 22 dBm     |
| `heltec_v3`      | Heltec WiFi LoRa 32 V3       | ESP32-S3   | SX1262           | SSD1306 128×64 | 1 bouton (PRG)   | 22 / 22 dBm     |
| `heltec_v4`      | Heltec WiFi LoRa 32 V4 (4.3) | ESP32-S3R2 | SX1262 + FEM     | SSD1306 128×64 | 1 bouton (PRG)   | 20 / 20 dBm     |

Sur le V4.3, la puissance est exprimée « à l'antenne » : le FEM KCT8103L
ajoute ~12 dB en émission, retranchés automatiquement de la consigne passée
au SX1262. La révision 4.2 (FEM GC1109) n'est pas gérée.

## Compilation

```sh
task build                     # toutes les cartes
task build TARGET=heltec_v3    # une seule
task upload TARGET=heltec_v3   # téléverse (défaut : wio_tracker_l1)
task flash TARGET=heltec_v3    # téléverse + moniteur série
```

Ou directement : `pio run -e <env>`.

Avant de flasher, adapter le préfixe de clé publique du répéteur visé dans
`src/AppConfig.h` (`kTargetPubkeyPrefix`), ainsi que le preset radio si besoin
(par défaut : MeshCore Île-de-France, 869.618 MHz).

## Organisation du code

```
src/
├── main.cpp          point d'entrée (délègue à App)
├── AppConfig.h       config applicative : preset radio, répéteur cible
├── app/              logique applicative (App)
├── mesh/             protocole MeshCore pur (parsing, TRACE) — aucune
│                     dépendance matérielle
├── ui/               écrans U8g2 (pilote commun SH1106/SSD1306)
└── hal/
    ├── Board.h       interface d'une carte : écran, brochage radio,
    │                 boutons logiques, puissance TX défaut/max
    ├── Radio.*       enveloppe SX1262/RadioLib (FEM, ISR, puissance)
    ├── Buttons.*     boutons débouncés -> touches logiques (Ok, Back,
    │                 Up/Down/Left/Right), base du futur menu
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
