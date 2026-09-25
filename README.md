# MeshCaching Firmware

Géolocalisation d'un répéteur [MeshCore](https://github.com/meshcore-dev/MeshCore) :
l'appareil affiche le RSSI (niveau de signal) des paquets provenant d'un
répéteur donné, ainsi que le bruit de fond du canal, et permet de « pinger »
le répéteur avec un paquet TRACE (le ping natif de MeshCore) via le bouton.

Firmware compagnon de l'article
[Géolocalisation d'un répéteur MeshCore](https://tutoduino.fr/menu-sdr/geolocalisation-repeteur-meshcore/)
de Tutoduino, préconisé pour l'évènement
[MeshCaching Île-de-France](https://tutoduino.fr/blog/meshcaching/).

## Cartes supportées

| Environnement    | Carte                          | MCU        | Radio            | Écran          | Boutons          | TX défaut / max |
|------------------|--------------------------------|------------|------------------|----------------|------------------|-----------------|
| `wio_tracker_l1` | Seeed Wio Tracker L1 Pro       | nRF52840   | SX1262           | SH1106 128×64  | croix + 2 boutons| 22 / 22 dBm     |
| `heltec_v3`      | Heltec WiFi LoRa 32 V3         | ESP32-S3   | SX1262           | SSD1306 128×64 | 1 bouton (PRG)   | 22 / 22 dBm     |
| `heltec_v4_2`    | Heltec WiFi LoRa 32 V4.2       | ESP32-S3R2 | SX1262 + FEM     | SSD1306 128×64 | 1 bouton (PRG)   | 20 / 20 dBm     |
| `heltec_v4_3`    | Heltec WiFi LoRa 32 V4.3       | ESP32-S3R2 | SX1262 + FEM     | SSD1306 128×64 | 1 bouton (PRG)   | 20 / 20 dBm     |
| `heltec_v4_r8`   | Heltec WiFi LoRa 32 V4 « R8 »  | ESP32-S3R8 | SX1262 + FEM     | SSD1306 128×64 | 1 bouton (PRG)   | 20 / 20 dBm     |
| `heltec_t096`    | Heltec T096                    | nRF52840   | SX1262 + FEM     | ST7735 160×80  | 1 bouton         | 22 / 22 dBm     |
| `tbeam_supreme`  | LilyGo T-Beam Supreme (868)    | ESP32-S3   | SX1262           | SH1106 128×64  | 1 bouton         | 22 / 22 dBm     |
| `tdeck`          | LilyGo T-Deck / T-Deck Plus    | ESP32-S3   | SX1262           | ST7789 320×240 | trackball + clavier | 22 / 22 dBm  |
| `techo`          | LilyGo T-Echo                  | nRF52840   | SX1262           | e-ink 200×200  | 1 bouton + touche | 22 / 22 dBm    |

Sur les cartes à FEM (V4.2, V4.3, V4 R8, T096), la puissance est exprimée « à
l'antenne » : le gain du KCT8103L en émission (~12 dB sur les V4, ~13 dB sur
le T096) est retranché automatiquement de la consigne passée au SX1262. Non
gérés : les déclinaisons TFT / e-ink des V4. Sur les V4, le type de FEM
est vérifié au démarrage (même détection que MeshCore, via le niveau de repos
de la broche CSD) : un binaire flashé sur la mauvaise révision (build 4.3 sur
un 4.2, ou l'inverse) s'arrête sur « Carte incompatible » sans jamais
émettre. Le GC1109 du 4.2 n'a pas de LNA débrayable : le réglage *Gain RX*
n'y propose pas `FEM LNA`.

Sur le T-Beam Supreme, seule la déclinaison SX1262 (868 MHz) est gérée (ni
LR1121, ni 2,4 GHz). Le PMU AXP2101 y alimente la radio et l'écran : le
firmware n'allume que ces rails (GNSS, carte SD et connecteurs restent
éteints) et règle la charge de la 18650 à 500 mA. Le bouton PWR du PMU
éteint l'appareil (appui de 4 s) ; le bouton utilisateur est celui du
milieu (GPIO0).

Sur le T-Deck, l'écran 128×64 de l'application est agrandi 2× au centre du
TFT. Le trackball sert de croix directionnelle (son clic = Ok) et le
clavier complète : Entrée ou Espace = Ok, Retour arrière = Back. Sans
clavier détecté au démarrage, le trackball seul suffit.

Sur le T-Echo, premier écran e-ink du projet, la zone 128×64 est dessinée
telle quelle au centre de la dalle 200×200 (pas de facteur entier possible),
en noir sur blanc. Un rafraîchissement partiel prend ~0,5 s et bloque le
programme : l'écran n'est redessiné que si son contenu a changé, au plus
une fois par seconde, et les boutons restent lus pendant l'attente. Les
animations (clignotement à la réception, logo de sommeil) sont désactivées.
Un rafraîchissement complet, plus lent (~2,6 s), nettoie les fantômes au
démarrage puis toutes les 30 mises à jour ou 10 minutes. Le bouton
utilisateur sert de Ok ; une tape sur la touche capacitive allume ou éteint
le rétroéclairage. Les lots équipés d'une dalle DEPG0150BN se compilent avec
`-D TECHO_EINK_MODEL=GxEPD2_150_BN`.

## Flasher une release

Des binaires prêts à flasher sont publiés pour chaque tag `vX.Y.Z` (onglet
*Releases*). Un tag avec suffixe (`vX.Y.Z-rc1`, `vX.Y.Z-tbeam.1`...) produit
les mêmes binaires en *pre-release*, pour les tests : ils restent
téléchargeables par tout le monde sans être présentés comme version stable. Le plus simple : l'outil web [flasher.meshcore.io](https://flasher.meshcore.io),
en choisissant « Custom Firmware » tout en bas de la liste des modèles de
cartes (les alternatives esptool / UF2 sont dans les notes de release). La
version (`git describe`) est injectée au build et s'affiche au démarrage
sous le titre MESHCACHING.

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

Écran principal : le RSSI du répéteur cible en grand, avec le SNR en
dessous. Deux mesures sont disponibles (cf. le réglage *Affichage*) — le
RSSI moyenné sur le paquet et celui du despreader (`SignalRssiPkt`, estimé
après désétalement du signal LoRa), qui reste significatif sous le plancher
de bruit. La dernière mesure reste affichée jusqu'à la suivante ; un logo
de sommeil (zZZ) occupe l'écran tant que rien n'a encore été reçu.

L'écran clignote (inversion) quand une réponse valide rafraîchit les
mesures, et une barre en bas indique le réarmement de l'émission (au plus
un ping toutes les 5 s) : pleine dès l'ordre d'émission, gelée pendant le
LBT, décroissante après l'émission réelle — et absente si le LBT a
abandonné.

L'émission est précédée d'un LBT (écoute du canal par CAD) : essais espacés
de slots aléatoires courts pendant 4 s au plus, puis abandon — pas de TX
forcé. Le témoin en haut à droite suit la séquence : « LBT », puis « TX »,
ou « OCCUPÉ » en cas d'abandon. Le bandeau du haut affiche le répéteur
surveillé (« RPT ») et le bruit de fond (« NF », médiane de 64 lectures de
RSSI instantané, évaluation continue et non intrusive) ; le SNR du dernier
paquet est centré sous le RSSI.

- **Ping TRACE** : appui court sur Ok (joystick sur le L1, clic du trackball
  sur le T-Deck, bouton PRG sur les Heltec, bouton du milieu sur le T-Beam,
  bouton utilisateur du T-Echo).
- **Menu** : bouton Menu sur le L1, appui long sur Ok sur les autres cartes
  (bouton unique des Heltec, du T-Beam et du T-Echo, clic du trackball sur
  le T-Deck).

Quatre réglages, persistés (NVS sur ESP32, LittleFS interne sur nRF52) :

- **Répéteur** : préfixe de clé publique, 4 digits hex édités un par un ;
- **Puiss. TX** : puissance d'émission « à l'antenne », du plancher utile de
  la carte à son maximum ;
- **Gain RX** : `AUCUN` / `RX BOOST` (le +2 dB interne du SX126x, défaut
  partout) / `FEM LNA` (cartes à FEM : Heltec V4 et T096, exclusif du boost).
  Attention : le LNA du FEM ajoute son gain au RSSI affiché ;
- **Affichage** : `RSSI` (défaut) / `DESPREAD`, une seule mesure en grand,
  ou `R+D` pour les deux côte à côte.

Navigation : sur le L1 et le T-Deck, Up/Down navigue ou modifie, Left/Right
change de digit, Ok valide, Back annule l'édition ou sort du menu. Sur les
cartes à bouton unique (Heltec, T-Beam, T-Echo) : clic = suivant/modifier,
appui long = valider, sortie par l'item « Retour ». Dans les deux cas, 20 s
d'inactivité referment le menu ; les changements sont appliqués et
sauvegardés à la fermeture.

## Organisation du code

```
src/
├── main.cpp          point d'entrée (délègue à App)
├── AppConfig.h       config applicative : preset radio, défauts d'usine
├── app/              logique applicative : App, NoiseFloor (médiane du
│                     bruit de fond)
├── mesh/             protocole MeshCore pur (parsing, TRACE) — aucune
│                     dépendance matérielle
├── ui/               écrans (StatusScreen + SettingsMenu), dessinés via
│                     l'abstraction Display
└── hal/
    ├── Board.h       interface d'une carte : écran, brochage radio,
    │                 boutons logiques, puissance TX défaut/max, FEM
    ├── Display.h     abstraction d'affichage (repère logique 128×64) ;
    │                 U8g2Display pour les OLED, adaptateur TFT dans le
    │                 fichier de carte concerné (T096)
    ├── Radio.*       enveloppe SX1262/RadioLib (FEM, ISR, puissance,
    │                 gain RX, LBT par CAD)
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
   gardée par `#ifdef BOARD_MA_CARTE`) et ajouter le drapeau à la liste
   vérifiée en tête de `src/main.cpp` ;
2. ajouter un `[env:ma_carte]` dans `platformio.ini` avec
   `-D BOARD_MA_CARTE` — et, si la carte n'est pas connue de PlatformIO,
   sa définition dans `boards/` (+ variante dans `variants/`).

## Crédits

- Sketch d'origine : [Tutoduino](https://tutoduino.fr/menu-sdr/geolocalisation-repeteur-meshcore/)
- Définitions de cartes et brochages : firmware
  [MeshCore](https://github.com/meshcore-dev/MeshCore) (licence MIT),
  cf. `boards/README.md`
