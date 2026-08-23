#pragma once
#include <stddef.h>
#include <stdint.h>

// Version du firmware, injectée par scripts/version.py (git describe)
#ifndef MESHCACHING_VERSION
#define MESHCACHING_VERSION "dev"
#endif

// =====================================================================
// Configuration de l'application, identique pour toutes les cartes.
// Ce qui dépend du matériel vit dans src/hal/boards/.
// =====================================================================
namespace config {

// Durée de l'écran de démarrage (MESHCACHING + version)
constexpr uint32_t kSplashMs = 4000;

// --- Paramètres radio : preset MeshCore Île-de-France ---
constexpr float kLoraFreqMhz = 869.618f;
constexpr float kLoraBwKhz = 62.5f;
constexpr uint8_t kLoraSf = 8;
constexpr uint8_t kLoraCr = 8;
// La puissance TX (défaut et maxi) est propre à chaque carte : cf. Board.

// Préfixe de la clé publique du répéteur MeshCore visé — valeur d'usine
// au premier démarrage, modifiable ensuite via le menu (persisté).
constexpr uint8_t kTargetPubkeyPrefix[] = { 0x57, 0xDB };

// On n'accepte une réponse TRACE que dans les 10 s suivant notre ping
constexpr uint32_t kTraceReplyTimeoutMs = 10000;

// Fraîcheur du RSSI affiché : sans nouveau paquet du répéteur au-delà
// de ce délai, l'écran principal retourne au logo de scan.
constexpr uint32_t kRssiFreshnessMs = 5000;

// Délai minimal entre deux émissions TRACE. Volontairement distinct de
// kRssiFreshnessMs : les deux durées se règlent indépendamment.
constexpr uint32_t kTxCooldownMs = 5000;

// Durée d'affichage du témoin d'émission "TX"
constexpr uint32_t kTxIndicatorMs = 700;

// LBT (écoute avant émission, CAD du SX126x) : canal occupé -> nouvel
// essai après un slot court aléatoire, abandon à la deadline — pas de
// TX forcé, contrairement à MeshCore (même deadline qu'eux).
constexpr uint32_t kLbtDeadlineMs = 4000;
constexpr uint32_t kLbtSlotMinMs = 100;
constexpr uint32_t kLbtSlotMaxMs = 300;
// Durée d'affichage du témoin "OCCUPÉ" après un abandon LBT
constexpr uint32_t kLbtBusyMsgMs = 2000;

// Bruit de fond : cadence d'échantillonnage du RSSI instantané (la
// médiane par cycle de 64 est dans NoiseFloor)
constexpr uint32_t kNoiseSampleIntervalMs = 20;

// Clignotement quand une réponse valide vient de rafraîchir le RSSI :
// une seule inversion de l'écran, de cette durée.
constexpr uint32_t kRxFlashMs = 150;

// Cadence de rafraîchissement de l'écran principal (animations, barre)
constexpr uint32_t kDisplayRefreshMs = 100;

}  // namespace config
