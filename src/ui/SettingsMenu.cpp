#include "SettingsMenu.h"

#include <stdio.h>
#include <string.h>

namespace {
const char kHexDigits[] = "0123456789ABCDEF";
}

SettingsMenu::SettingsMenu(Display &display, Board &board)
    : _d(display), _board(board), _hasDpad(false) {
  size_t count = 0;
  const ButtonSpec *specs = board.buttons(count);
  for (size_t i = 0; i < count; i++) {
    if (specs[i].key == Key::Up) {
      _hasDpad = true;
    }
  }
}

void SettingsMenu::open(const AppSettings &current) {
  _settings = current;
  _backup = current;
  _mode = Mode::Nav;
  _cursor = 0;
  _digit = 0;
  _open = true;
  _lastActivityMs = millis();
  draw();
}

SettingsMenu::Action SettingsMenu::translate(const ButtonEvent &event) const {
  if (!_hasDpad) {
    // Bouton unique : appui long = valider ; clic court = item suivant en
    // navigation, incrément en édition (digit hex, gain RX) — sauf la
    // puissance, qui décrémente depuis son maximum (valeur de départ).
    if (event.key == Key::Ok) {
      if (event.longPress) {
        return Action::Select;
      }
      return (_mode == Mode::EditTarget || _mode == Mode::EditRxGain ||
              _mode == Mode::EditRssiDisplay)
                 ? Action::Up
                 : Action::Down;
    }
    return Action::None;
  }
  switch (event.key) {
    case Key::Up: return Action::Up;
    case Key::Down: return Action::Down;
    case Key::Left: return Action::Left;
    case Key::Right: return Action::Right;
    case Key::Ok: return Action::Select;
    case Key::Back: return Action::Exit;
    default: return Action::None;
  }
}

bool SettingsMenu::handleEvent(const ButtonEvent &event) {
  if (!_open) {
    return false;
  }
  _lastActivityMs = millis();
  Action action = translate(event);
  bool closed = false;
  switch (_mode) {
    case Mode::Nav: handleNav(action, closed); break;
    case Mode::EditTarget: handleEditTarget(action); break;
    case Mode::EditTxPower: handleEditTxPower(action); break;
    case Mode::EditRxGain: handleEditRxGain(action); break;
    case Mode::EditRssiDisplay: handleEditRssiDisplay(action); break;
  }
  if (closed) {
    _open = false;
    return true;
  }
  draw();
  return false;
}

bool SettingsMenu::tickTimeout() {
  if (_open && millis() - _lastActivityMs >= kTimeoutMs) {
    _open = false;  // les valeurs en cours (même mi-édition) sont gardées
    return true;
  }
  return false;
}

void SettingsMenu::handleNav(Action action, bool &closed) {
  switch (action) {
    case Action::Up:
      _cursor = (_cursor + kItemCount - 1) % kItemCount;
      break;
    case Action::Down:
      _cursor = (_cursor + 1) % kItemCount;
      break;
    case Action::Select:
      _backup = _settings;
      switch ((Item)_cursor) {
        case Item::Target:
          _digit = 0;
          _mode = Mode::EditTarget;
          break;
        case Item::TxPower: _mode = Mode::EditTxPower; break;
        case Item::RxGain: _mode = Mode::EditRxGain; break;
        case Item::RssiDisplay: _mode = Mode::EditRssiDisplay; break;
        case Item::Back: closed = true; break;
      }
      break;
    case Action::Exit:
      closed = true;
      break;
    default:
      break;
  }
}

void SettingsMenu::handleEditTarget(Action action) {
  switch (action) {
    case Action::Up:
      setNibble(_digit, (nibble(_digit) + 1) & 0x0F);
      break;
    case Action::Down:
      setNibble(_digit, (nibble(_digit) + 15) & 0x0F);
      break;
    case Action::Left:
      _digit = (_digit + kTargetDigits - 1) % kTargetDigits;
      break;
    case Action::Right:
      _digit = (_digit + 1) % kTargetDigits;
      break;
    case Action::Select:
      // Valide le digit courant ; le dernier valide l'adresse entière
      if (_digit + 1 < kTargetDigits) {
        _digit++;
      } else {
        _mode = Mode::Nav;
      }
      break;
    case Action::Exit:
      memcpy(_settings.targetPrefix, _backup.targetPrefix,
             sizeof(_settings.targetPrefix));
      _mode = Mode::Nav;
      break;
    default:
      break;
  }
}

void SettingsMenu::handleEditTxPower(Action action) {
  // Plage utile de la carte : sur les V4 à FEM, le plancher remonte du
  // gain du PA pour que la valeur affichée reste la puissance réelle.
  int8_t minDbm = _board.txPowerMinDbm();
  int8_t maxDbm = _board.txPowerMaxDbm();
  switch (action) {
    case Action::Up:
      _settings.txPowerDbm =
          _settings.txPowerDbm >= maxDbm ? minDbm
                                         : (int8_t)(_settings.txPowerDbm + 1);
      break;
    case Action::Down:
      _settings.txPowerDbm =
          _settings.txPowerDbm <= minDbm
              ? maxDbm
              : (int8_t)(_settings.txPowerDbm - 1);
      break;
    case Action::Select:
      _mode = Mode::Nav;
      break;
    case Action::Exit:
      _settings.txPowerDbm = _backup.txPowerDbm;
      _mode = Mode::Nav;
      break;
    default:
      break;
  }
}

uint8_t SettingsMenu::rxGainChoices(RxGainMode out[3]) const {
  out[0] = RxGainMode::kNone;
  out[1] = RxGainMode::kSxBoost;
  out[2] = RxGainMode::kFemLna;
  return _board.hasFemLna() ? 3 : 2;
}

void SettingsMenu::handleEditRxGain(Action action) {
  RxGainMode choices[3];
  uint8_t count = rxGainChoices(choices);
  uint8_t index = 0;
  for (uint8_t i = 0; i < count; i++) {
    if (choices[i] == _settings.rxGainMode) {
      index = i;
    }
  }
  switch (action) {
    case Action::Up:
      _settings.rxGainMode = choices[(index + 1) % count];
      break;
    case Action::Down:
      _settings.rxGainMode = choices[(index + count - 1) % count];
      break;
    case Action::Select:
      _mode = Mode::Nav;
      break;
    case Action::Exit:
      _settings.rxGainMode = _backup.rxGainMode;
      _mode = Mode::Nav;
      break;
    default:
      break;
  }
}

void SettingsMenu::handleEditRssiDisplay(Action action) {
  constexpr uint8_t kChoiceCount = 3;  // DEUX / RSSI / DESPREAD
  uint8_t index = (uint8_t)_settings.rssiDisplay;
  switch (action) {
    case Action::Up:
      _settings.rssiDisplay = (RssiDisplayMode)((index + 1) % kChoiceCount);
      break;
    case Action::Down:
      _settings.rssiDisplay =
          (RssiDisplayMode)((index + kChoiceCount - 1) % kChoiceCount);
      break;
    case Action::Select:
      _mode = Mode::Nav;
      break;
    case Action::Exit:
      _settings.rssiDisplay = _backup.rssiDisplay;
      _mode = Mode::Nav;
      break;
    default:
      break;
  }
}

uint8_t SettingsMenu::nibble(uint8_t index) const {
  uint8_t byte = _settings.targetPrefix[index / 2];
  return index % 2 == 0 ? (byte >> 4) : (byte & 0x0F);
}

void SettingsMenu::setNibble(uint8_t index, uint8_t value) {
  uint8_t &byte = _settings.targetPrefix[index / 2];
  byte = index % 2 == 0 ? (uint8_t)((value << 4) | (byte & 0x0F))
                        : (uint8_t)((byte & 0xF0) | value);
}

const char *SettingsMenu::rxGainLabel(RxGainMode mode) {
  switch (mode) {
    case RxGainMode::kNone: return "AUCUN";
    case RxGainMode::kSxBoost: return "RX BOOST";
    case RxGainMode::kFemLna: return "FEM LNA";
  }
  return "?";
}

const char *SettingsMenu::rssiDisplayLabel(RssiDisplayMode mode) {
  switch (mode) {
    case RssiDisplayMode::kBoth: return "R+D";
    case RssiDisplayMode::kRssiOnly: return "RSSI";
    case RssiDisplayMode::kDespreadOnly: return "DESPREAD";
  }
  return "?";
}

// ---------------------------------------------------------------------
// Rendu
// ---------------------------------------------------------------------

void SettingsMenu::drawTitle(const char *title) {
  _d.setFont(Font::kSmall);
  _d.drawText(0, 10, title);
  _d.drawHLine(0, 13, _d.width());
}

void SettingsMenu::drawHint(const char *dpadHint,
                            const char *singleButtonHint) {
  _d.setFont(Font::kSmall);
  _d.drawText(0, 63, _hasDpad ? dpadHint : singleButtonHint);
}

void SettingsMenu::drawRightAligned(int16_t y, const char *text) {
  _d.drawText(_d.width() - _d.textWidth(text), y, text);
}

void SettingsMenu::draw() {
  _d.clear();
  switch (_mode) {
    case Mode::Nav: drawNav(); break;
    case Mode::EditTarget: drawEditTarget(); break;
    case Mode::EditTxPower: drawEditTxPower(); break;
    case Mode::EditRxGain: drawEditRxGain(); break;
    case Mode::EditRssiDisplay: drawEditRssiDisplay(); break;
  }
  _d.send();
}

void SettingsMenu::drawNav() {
  drawTitle("RÉGLAGES");
  char value[12];
  for (uint8_t i = 0; i < kItemCount; i++) {
    int16_t y = 23 + i * 10;  // interligne serré : cinq items sur 64 px
    if (i == _cursor) {
      _d.drawText(0, y, ">");
    }
    switch ((Item)i) {
      case Item::Target:
        _d.drawText(8, y, "Répéteur");
        snprintf(value, sizeof(value), "%02X%02X", _settings.targetPrefix[0],
                 _settings.targetPrefix[1]);
        drawRightAligned(y, value);
        break;
      case Item::TxPower:
        _d.drawText(8, y, "Puiss. TX");
        snprintf(value, sizeof(value), "%ddBm", _settings.txPowerDbm);
        drawRightAligned(y, value);
        break;
      case Item::RxGain:
        _d.drawText(8, y, "Gain RX");
        drawRightAligned(y, rxGainLabel(_settings.rxGainMode));
        break;
      case Item::RssiDisplay:
        _d.drawText(8, y, "Affichage");
        drawRightAligned(y, rssiDisplayLabel(_settings.rssiDisplay));
        break;
      case Item::Back:
        _d.drawText(8, y, "Retour");
        break;
    }
  }
}

void SettingsMenu::drawEditTarget() {
  drawTitle("Répéteur cible");
  _d.setFont(Font::kBig);
  int16_t pitch = _d.textWidth("0") + 6;
  int16_t x0 = (_d.width() - kTargetDigits * pitch + 6) / 2;
  for (uint8_t i = 0; i < kTargetDigits; i++) {
    char digit[2] = {kHexDigits[nibble(i)], '\0'};
    _d.drawText(x0 + i * pitch, 45, digit);
  }
  // Soulignement du digit en cours d'édition
  _d.drawBox(x0 + _digit * pitch, 49, pitch - 6, 2);
  drawHint("OK: suivant/valider", "clic: +1  long: suiv.");
}

void SettingsMenu::drawEditTxPower() {
  drawTitle("Puissance TX");
  char value[8];
  snprintf(value, sizeof(value), "%d", _settings.txPowerDbm);
  _d.setFont(Font::kBig);
  int16_t w = _d.textWidth(value);
  int16_t x = (_d.width() - w) / 2;
  _d.drawText(x, 45, value);
  _d.setFont(Font::kSmall);
  _d.drawText(x + w + 3, 45, "dBm");
  drawHint("OK: valider", "clic: -1  long: OK");
}

void SettingsMenu::drawEditRxGain() {
  drawTitle("Gain RX");
  const char *label = rxGainLabel(_settings.rxGainMode);
  _d.setFont(Font::kMenu);
  _d.drawText((_d.width() - _d.textWidth(label)) / 2, 42, label);
  drawHint("OK: valider", "clic: suivant  long: OK");
}

void SettingsMenu::drawEditRssiDisplay() {
  drawTitle("Affichage");
  const char *label = rssiDisplayLabel(_settings.rssiDisplay);
  _d.setFont(Font::kMenu);
  _d.drawText((_d.width() - _d.textWidth(label)) / 2, 42, label);
  drawHint("OK: valider", "clic: suivant  long: OK");
}
