#pragma once
#include <stddef.h>
#include <stdint.h>

// Firmware version, injected by scripts/version.py (git describe)
#ifndef MESHCACHING_VERSION
#define MESHCACHING_VERSION "dev"
#endif

// =====================================================================
// Application configuration, identical for every board.
// Whatever depends on the hardware lives in src/hal/boards/.
// =====================================================================
namespace config {

// Splash screen duration (MESHCACHING + version)
constexpr uint32_t kSplashMs = 3000;

// --- Radio settings: MeshCore EU Narrow preset ---
constexpr float kLoraFreqMhz = 869.618f;
constexpr float kLoraBwKhz = 62.5f;
constexpr uint8_t kLoraSf = 8;
constexpr uint8_t kLoraCr = 8;
// TX power (default and max) is specific to each board: see Board.

// MeshCore US preset (use instead of the block above):
// constexpr float kLoraFreqMhz = 910.525f;
// constexpr float kLoraBwKhz = 62.5f;
// constexpr uint8_t kLoraSf = 7;
// constexpr uint8_t kLoraCr = 5;

// Public key prefix of the targeted MeshCore repeater - factory value
// on first boot, then editable from the menu (persisted).
constexpr uint8_t kTargetPubkeyPrefix[] = { 0x57, 0xDB };

// A TRACE reply is only accepted within 10 s of our ping
constexpr uint32_t kTraceReplyTimeoutMs = 10000;

// Minimum delay between two TRACE transmissions
constexpr uint32_t kTxCooldownMs = 5000;

// Display duration of the "TX" transmit indicator
constexpr uint32_t kTxIndicatorMs = 700;

// LBT (listen before talk, SX126x CAD): channel busy -> retry after a
// short random slot, give up at the deadline - no forced TX, unlike
// MeshCore (same deadline as theirs).
constexpr uint32_t kLbtDeadlineMs = 4000;
constexpr uint32_t kLbtSlotMinMs = 100;
constexpr uint32_t kLbtSlotMaxMs = 300;
// Display duration of the "OCCUPÉ" indicator after an LBT give-up
constexpr uint32_t kLbtBusyMsgMs = 2000;

// Noise floor: sampling rate of the instantaneous RSSI (the median
// over each cycle of 64 lives in NoiseFloor)
constexpr uint32_t kNoiseSampleIntervalMs = 20;

// Blink when a valid reply has just refreshed the RSSI: a single
// inversion of the screen, lasting this long.
constexpr uint32_t kRxFlashMs = 150;

// Refresh rate of the main screen (animations, bar)
constexpr uint32_t kDisplayRefreshMs = 100;

}  // namespace config
