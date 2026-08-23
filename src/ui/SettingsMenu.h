#pragma once
#include <U8g2lib.h>

#include "../hal/Board.h"
#include "../hal/Buttons.h"
#include "../hal/Settings.h"

// =====================================================================
// Menu de reglages : repeteur cible (4 digits hex), puissance TX,
// chaine de gain RX (+ LNA FEM sur les cartes qui en ont un).
//
// Pilotage selon les touches de la carte :
//  - croix directionnelle (Wio Tracker L1) : Up/Down navigue ou modifie,
//    Left/Right change de digit, Ok valide, Back annule l'edition ou
//    sort du menu ;
//  - bouton unique (Heltec) : clic = suivant / modifier, appui long =
//    valider ; la sortie passe par l'item "Retour" ou le timeout.
//
// Le menu travaille sur une copie : quand handleEvent()/tickTimeout()
// renvoie true (fermeture), l'appelant applique et sauvegarde result().
// =====================================================================
class SettingsMenu {
public:
  SettingsMenu(U8G2 &display, Board &board);

  void open(const AppSettings &current);
  bool isOpen() const { return _open; }
  const AppSettings &result() const { return _settings; }

  bool handleEvent(const ButtonEvent &event);
  bool tickTimeout();

private:
  enum class Action : uint8_t { None, Up, Down, Left, Right, Select, Exit };
  enum class Item : uint8_t { Target, TxPower, RxGain, Back };
  static constexpr uint8_t kItemCount = 4;
  enum class Mode : uint8_t { Nav, EditTarget, EditTxPower, EditRxGain };

  static constexpr uint32_t kTimeoutMs = 20000;
  static constexpr uint8_t kTargetDigits = 4;

  Action translate(const ButtonEvent &event) const;
  void handleNav(Action action, bool &closed);
  void handleEditTarget(Action action);
  void handleEditTxPower(Action action);
  void handleEditRxGain(Action action);

  uint8_t nibble(uint8_t index) const;
  void setNibble(uint8_t index, uint8_t value);
  uint8_t rxGainChoices(RxGainMode out[3]) const;
  static const char *rxGainLabel(RxGainMode mode);

  void draw();
  void drawNav();
  void drawEditTarget();
  void drawEditTxPower();
  void drawEditRxGain();
  void drawTitle(const char *title);
  void drawHint(const char *dpadHint, const char *singleButtonHint);
  void drawRightAligned(u8g2_uint_t y, const char *text);

  U8G2 &_d;
  Board &_board;
  bool _hasDpad;

  bool _open = false;
  Mode _mode = Mode::Nav;
  uint8_t _cursor = 0;
  uint8_t _digit = 0;
  AppSettings _settings{};
  AppSettings _backup{};  // valeurs a restaurer si l'edition est annulee
  uint32_t _lastActivityMs = 0;
};
