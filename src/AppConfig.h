#pragma once
#include <stddef.h>
#include <stdint.h>

// =====================================================================
// Configuration de l'application, identique pour toutes les cartes.
// Ce qui depend du materiel vit dans src/hal/boards/.
// =====================================================================
namespace config {

// --- Parametres radio : preset MeshCore Ile de France ---
constexpr float kLoraFreqMhz = 869.618f;
constexpr float kLoraBwKhz = 62.5f;
constexpr uint8_t kLoraSf = 8;
constexpr uint8_t kLoraCr = 8;
// La puissance TX (defaut et maxi) est propre a chaque carte : cf. Board.

// Prefixe de la cle publique du repeteur MeshCore vise — valeur d'usine
// au premier demarrage, modifiable ensuite via le menu (persiste).
constexpr uint8_t kTargetPubkeyPrefix[] = { 0x57, 0xDB };

// On n'accepte une reponse TRACE que dans les 10 s suivant notre ping
constexpr uint32_t kTraceReplyTimeoutMs = 10000;

// Rafraichissement periodique de l'ecran (compteur "temps ecoule")
constexpr uint32_t kDisplayRefreshMs = 1000;

}  // namespace config
