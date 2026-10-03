#pragma once
#include "../hal/Display.h"

#include "../hal/Board.h"
#include "../hal/Buttons.h"
#include "../hal/Settings.h"

// =====================================================================
// Settings menu: target repeater (4 hex digits), TX power, RX gain
// chain (+ FEM LNA on the boards that have one), automatic TRACE ping.
//
// Controls depend on the keys the board provides:
//  - D-pad (Wio Tracker L1): Up/Down navigates or edits, Left/Right
//    moves between digits, Ok confirms, Back cancels the edit or
//    leaves the menu;
//  - single button (Heltec): click = next / edit, long press =
//    confirm; exiting goes through the "Retour" item or the timeout.
//
// The menu works on a copy: when handleEvent()/tickTimeout() returns
// true (closing), the caller applies and saves result().
// =====================================================================
class SettingsMenu {
 public:
  SettingsMenu(Display &display, Board &board);

  void open(const AppSettings &current);
  bool isOpen() const { return _open; }
  const AppSettings &result() const { return _settings; }

  bool handleEvent(const ButtonEvent &event);
  bool tickTimeout();

 private:
  enum class Action : uint8_t { None, Up, Down, Left, Right, Select, Exit };
  enum class Item : uint8_t { Target, TxPower, RxGain, AutoPing, Back };
  static constexpr uint8_t kItemCount = 5;
  enum class Mode : uint8_t {
    Nav,
    EditTarget,
    EditTxPower,
    EditRxGain,
  };

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
  void drawRightAligned(int16_t y, const char *text);

  Display &_d;
  Board &_board;
  bool _hasDpad;

  bool _open = false;
  Mode _mode = Mode::Nav;
  uint8_t _cursor = 0;
  uint8_t _digit = 0;
  AppSettings _settings{};
  AppSettings _backup{};  // values to restore if the edit is canceled
  uint32_t _lastActivityMs = 0;
};