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

// Rafraîchissement périodique de l'écran (compteur "temps écoulé")
constexpr uint32_t kDisplayRefreshMs = 1000;

}  // namespace config
