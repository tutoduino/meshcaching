#include "Settings.h"

#include <string.h>

namespace {

constexpr uint16_t kSettingsVersion = 1;

// Representation stockee, versionnee : toute evolution du format passe
// par une incrementation de version (les anciennes donnees sont alors
// ignorees et les defauts d'usine reappliques).
struct StoredSettings {
  uint16_t version;
  uint8_t targetPrefix[2];
  int8_t txPowerDbm;
  uint8_t rxGainMode;
} __attribute__((packed));

void pack(const AppSettings &s, StoredSettings &out) {
  out.version = kSettingsVersion;
  memcpy(out.targetPrefix, s.targetPrefix, sizeof(out.targetPrefix));
  out.txPowerDbm = s.txPowerDbm;
  out.rxGainMode = (uint8_t)s.rxGainMode;
}

bool unpack(const StoredSettings &in, AppSettings &out) {
  if (in.version != kSettingsVersion) {
    return false;
  }
  memcpy(out.targetPrefix, in.targetPrefix, sizeof(out.targetPrefix));
  out.txPowerDbm = in.txPowerDbm;
  out.rxGainMode = (RxGainMode)in.rxGainMode;
  return true;
}

}  // namespace

bool settingsEqual(const AppSettings &a, const AppSettings &b) {
  return memcmp(a.targetPrefix, b.targetPrefix, sizeof(a.targetPrefix)) == 0 &&
         a.txPowerDbm == b.txPowerDbm && a.rxGainMode == b.rxGainMode;
}

#if defined(ESP32)

#include <Preferences.h>

namespace {
constexpr char kNvsNamespace[] = "meshcaching";
constexpr char kNvsKey[] = "cfg";
}  // namespace

bool settingsLoad(AppSettings &out) {
  Preferences prefs;
  // begin() en lecture seule echoue si le namespace n'existe pas encore
  if (!prefs.begin(kNvsNamespace, true)) {
    return false;
  }
  StoredSettings stored;
  size_t n = prefs.getBytes(kNvsKey, &stored, sizeof(stored));
  prefs.end();
  return n == sizeof(stored) && unpack(stored, out);
}

void settingsSave(const AppSettings &s) {
  StoredSettings stored;
  pack(s, stored);
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, false)) {
    return;
  }
  prefs.putBytes(kNvsKey, &stored, sizeof(stored));
  prefs.end();
}

#elif defined(ARDUINO_ARCH_NRF52) || defined(NRF52_SERIES)

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

namespace {
constexpr char kSettingsPath[] = "/meshcaching.cfg";

bool fsBegin() {
  static bool started = InternalFS.begin();
  return started;
}
}  // namespace

bool settingsLoad(AppSettings &out) {
  if (!fsBegin()) {
    return false;
  }
  File file(InternalFS);
  if (!file.open(kSettingsPath, FILE_O_READ)) {
    return false;
  }
  StoredSettings stored;
  int n = file.read((uint8_t *)&stored, sizeof(stored));
  file.close();
  return n == (int)sizeof(stored) && unpack(stored, out);
}

void settingsSave(const AppSettings &s) {
  if (!fsBegin()) {
    return;
  }
  StoredSettings stored;
  pack(s, stored);
  // FILE_O_WRITE ecrit en fin de fichier existant : on repart d'un
  // fichier neuf a chaque sauvegarde.
  InternalFS.remove(kSettingsPath);
  File file(InternalFS);
  if (!file.open(kSettingsPath, FILE_O_WRITE)) {
    return;
  }
  file.write((const uint8_t *)&stored, sizeof(stored));
  file.close();
}

#else
#error "Settings : plateforme non geree"
#endif
