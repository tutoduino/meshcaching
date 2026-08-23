#pragma once
#include <stddef.h>
#include <stdint.h>

// =====================================================================
// Configuration de l'application, identique pour toutes les cartes.
// Ce qui dépend du matériel vit dans src/hal/boards/.
// =====================================================================
namespace config {

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

// Clignotement par inversion de l'écran quand une réponse valide vient
// de rafraîchir le RSSI : durée totale et demi-période.
constexpr uint32_t kRxFlashMs = 450;
constexpr uint32_t kRxFlashHalfPeriodMs = 150;

// Cadence de rafraîchissement de l'écran principal (animations, barre)
constexpr uint32_t kDisplayRefreshMs = 100;

}  // namespace config
