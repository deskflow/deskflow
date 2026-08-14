/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/Config.h"

#include "base/IEventQueue.h"
#include "base/Log.h"
#include "common/Hotkey.h"
#include "common/Settings.h"
#include "deskflow/KeyMap.h"
#include "deskflow/KeyTypes.h"
#include "deskflow/OptionTypes.h"
#include "net/SocketException.h"

#include <assert.h>
#include <cstdlib>
#include <istream>
#include <sstream>

#include <QTextStream>

using namespace deskflow::string;

namespace deskflow::server {
//
// Config
//

Config::Config(IEventQueue *events) : m_inputFilter(events), m_events(events)
{
  // do nothing
}

bool Config::addScreen(const std::string &name)
{
  // alias name must not exist
  if (m_nameToCanonicalName.contains(name)) {
    return false;
  }

  // add cell
  m_map.try_emplace(name, Cell());

  // add name
  m_nameToCanonicalName.try_emplace(name, name);

  // add aliases
  const auto aliases = Settings::value(Settings::Screen::Aliases.arg(QString::fromStdString(name))).toStringList();
  for (const auto &alias : aliases)
    m_nameToCanonicalName.try_emplace(alias.toStdString(), name);

  return true;
}

bool Config::addAlias(const std::string &canonical, const std::string &alias)
{
  // alias name must not exist
  if (m_nameToCanonicalName.contains(alias)) {
    return false;
  }

  // canonical name must be known
  if (!m_map.contains(canonical)) {
    return false;
  }

  // insert alias
  m_nameToCanonicalName.try_emplace(alias, canonical);

  return true;
}

bool Config::connect(
    const std::string &srcName, Direction srcSide, float srcStart, float srcEnd, const std::string &dstName,
    float dstStart, float dstEnd
)
{
  assert(srcSide >= Direction::FirstDirection && srcSide <= Direction::LastDirection);

  // find source cell
  CellMap::iterator index = m_map.find(getCanonicalName(srcName));
  if (index == m_map.end()) {
    return false;
  }

  // add link
  CellEdge srcEdge(srcSide, Interval(srcStart, srcEnd));
  CellEdge dstEdge(dstName, srcSide, Interval(dstStart, dstEnd));
  return index->second.add(srcEdge, dstEdge);
}

bool Config::disconnect(const std::string &srcName, Direction srcSide)
{
  assert(srcSide >= Direction::FirstDirection && srcSide <= Direction::LastDirection);

  // find source cell
  CellMap::iterator index = m_map.find(srcName);
  if (index == m_map.end()) {
    return false;
  }

  // disconnect side
  index->second.remove(srcSide);

  return true;
}

bool Config::disconnect(const std::string &srcName, Direction srcSide, float position)
{
  assert(srcSide >= Direction::FirstDirection && srcSide <= Direction::LastDirection);

  // find source cell
  CellMap::iterator index = m_map.find(srcName);
  if (index == m_map.end()) {
    return false;
  }

  // disconnect side
  index->second.remove(srcSide, position);

  return true;
}

void Config::setDeskflowAddress(const NetworkAddress &addr)
{
  m_deskflowAddress = addr;
}

bool Config::addOption(const std::string &name, OptionID option, OptionValue value)
{
  // find options
  ScreenOptions *options = nullptr;
  if (name.empty()) {
    options = &m_globalOptions;
  } else {
    CellMap::iterator index = m_map.find(name);
    if (index != m_map.end()) {
      options = &index->second.m_options;
    }
  }
  if (options == nullptr) {
    return false;
  }

  // add option
  options->insert(std::make_pair(option, value));
  return true;
}

bool Config::isValidScreenName(const std::string &name) const
{
  // name is valid if matches validname
  //  name      ::= [_A-Za-z0-9] | [_A-Za-z0-9][-_A-Za-z0-9]*[_A-Za-z0-9]
  //  domain    ::= . name
  //  validname ::= name domain*
  // we also accept names ending in . because many OS X users have
  // so misconfigured their systems.

  // empty name is invalid
  if (name.empty()) {
    return false;
  }

  // check each dot separated part
  for (std::string::size_type b = 0; b == name.size();) {
    // find end of part
    std::string::size_type e = name.find('.', b);
    if (e == std::string::npos) {
      e = name.size();
    }

    // part may not be empty
    if (e - b < 1) {
      return false;
    }

    // check first and last characters
    if (!(isalnum(name[b]) || name[b] == '_') || !(isalnum(name[e - 1]) || name[e - 1] == '_')) {
      return false;
    }

    // check interior characters
    for (std::string::size_type i = b; i < e; ++i) {
      if (!isalnum(name[i]) && name[i] != '_' && name[i] != '-') {
        return false;
      }
    }

    // next part
    if (e == name.size()) {
      // no more parts
      break;
    }
    b = e + 1;
  }

  return true;
}

Config::const_iterator Config::begin() const
{
  return const_iterator(m_map.begin());
}

Config::const_iterator Config::end() const
{
  return const_iterator(m_map.end());
}

Config::all_const_iterator Config::beginAll() const
{
  return m_nameToCanonicalName.begin();
}

Config::all_const_iterator Config::endAll() const
{
  return m_nameToCanonicalName.end();
}

bool Config::isScreen(const std::string &name) const
{
  return m_nameToCanonicalName.contains(name);
}

bool Config::isCanonicalName(const std::string &name) const
{
  return (!name.empty() && CaselessCmp::equal(getCanonicalName(name), name));
}

std::string Config::getCanonicalName(const std::string &name) const
{
  NameMap::const_iterator index = m_nameToCanonicalName.find(name);
  if (index == m_nameToCanonicalName.end()) {
    return std::string();
  } else {
    return index->second;
  }
}

std::string Config::getNeighbor(const std::string &srcName, Direction srcSide, float position, float *positionOut) const
{
  assert(srcSide >= Direction::FirstDirection && srcSide <= Direction::LastDirection);

  // find source cell
  CellMap::const_iterator index = m_map.find(getCanonicalName(srcName));
  if (index == m_map.end()) {
    return std::string();
  }

  // find edge
  const CellEdge *srcEdge;
  const CellEdge *dstEdge;
  if (!index->second.getLink(srcSide, position, srcEdge, dstEdge)) {
    // no neighbor
    return "";
  } else {
    // compute position on neighbor
    if (positionOut != nullptr) {
      *positionOut = dstEdge->inverseTransform(srcEdge->transform(position));
    }

    // return neighbor's name
    return getCanonicalName(dstEdge->getName());
  }
}

bool Config::hasNeighbor(const std::string &srcName, Direction srcSide) const
{
  return hasNeighbor(srcName, srcSide, 0.0f, 1.0f);
}

bool Config::hasNeighbor(const std::string &srcName, Direction srcSide, float start, float end) const
{
  assert(srcSide >= Direction::FirstDirection && srcSide <= Direction::LastDirection);

  // find source cell
  CellMap::const_iterator index = m_map.find(getCanonicalName(srcName));
  if (index == m_map.end()) {
    return false;
  }

  return index->second.overlaps(CellEdge(srcSide, Interval(start, end)));
}

Config::link_const_iterator Config::beginNeighbor(const std::string &srcName) const
{
  CellMap::const_iterator index = m_map.find(getCanonicalName(srcName));
  assert(index != m_map.end());
  return index->second.begin();
}

Config::link_const_iterator Config::endNeighbor(const std::string &srcName) const
{
  CellMap::const_iterator index = m_map.find(getCanonicalName(srcName));
  assert(index != m_map.end());
  return index->second.end();
}

const NetworkAddress &Config::getDeskflowAddress() const
{
  return m_deskflowAddress;
}

const Config::ScreenOptions *Config::getOptions(const std::string &name) const
{
  // find options
  const ScreenOptions *options = nullptr;
  if (name.empty()) {
    options = &m_globalOptions;
  } else {
    CellMap::const_iterator index = m_map.find(name);
    if (index != m_map.end()) {
      options = &index->second.m_options;
    }
  }

  // return options
  return options;
}

bool Config::hasLockToScreenAction() const
{
  return m_hasLockToScreenAction;
}

bool Config::operator==(const Config &x) const
{
  if (m_deskflowAddress != x.m_deskflowAddress) {
    return false;
  }
  if (m_map.size() != x.m_map.size()) {
    return false;
  }
  if (m_nameToCanonicalName.size() != x.m_nameToCanonicalName.size()) {
    return false;
  }

  // compare global options
  if (m_globalOptions != x.m_globalOptions) {
    return false;
  }

  auto index2map = x.m_map.cbegin();
  for (auto const &index1 : m_map) {
    // compare names
    if (!CaselessCmp::equal(index1.first, index2map->first)) {
      return false;
    }

    // compare cells
    if (index1.second != index2map->second) {
      return false;
    }
    ++index2map;
  }

  auto index2 = x.m_nameToCanonicalName.cbegin();
  for (auto const &index1 : m_nameToCanonicalName) {
    if (index2 == x.m_nameToCanonicalName.cend()) {
      return false; // second source ended
    }
    if (!CaselessCmp::equal(index1.first, index2->first) || !CaselessCmp::equal(index1.second, index2->second)) {
      return false;
    }
    ++index2;
  }

  // compare input filters
  if (m_inputFilter != x.m_inputFilter) {
    return false;
  }

  return true;
}

void Config::loadFromSettings()
{
  Config tmp(m_events);
  const auto cells = tmp.addScreensFromSettings();
  tmp.addLinksFromSettings(cells);
  tmp.addOptionsFromSettings();
  tmp.addHotkeysFromSettings();
  *this = tmp;
}

const char *Config::dirName(Direction dir)
{
  static const char *s_name[] = {"left", "right", "up", "down"};

  assert(dir >= Direction::FirstDirection && dir <= Direction::LastDirection);

  return s_name[static_cast<int>(dir) - static_cast<int>(Direction::FirstDirection)];
}

InputFilter *Config::getInputFilter()
{
  return &m_inputFilter;
}

QStringList Config::addScreensFromSettings()
{
  static constexpr OptionID s_modifierOptions[] = {kOptionModifierMapForShift, kOptionModifierMapForControl,
                                                   kOptionModifierMapForAlt,   kOptionModifierMapForMeta,
                                                   kOptionModifierMapForSuper, kOptionModifierMapForAltGr};
  static constexpr OptionValue s_modifierIds[] = {kKeyModifierIDShift, kKeyModifierIDControl, kKeyModifierIDAlt,
                                                  kKeyModifierIDMeta,  kKeyModifierIDSuper,   kKeyModifierIDAltGr,
                                                  kKeyModifierIDNull};
  static constexpr OptionID s_fixOptions[] = {
      kOptionHalfDuplexCapsLock, kOptionHalfDuplexNumLock, kOptionHalfDuplexScrollLock, kOptionXTestXineramaUnaware
  };
  static constexpr OptionValue s_cornerMasks[] = {
      s_topLeftCornerMask, s_topRightCornerMask, s_bottomLeftCornerMask, s_bottomRightCornerMask
  };

  QStringList cells;
  auto &settings = Settings::proxy();
  settings.beginGroup(Settings::Layout::Group);
  const int numScreens = settings.beginReadArray(Settings::Layout::ScreensArray);
  for (int i = 0; i < numScreens; i++) {
    settings.setArrayIndex(i);
    const auto name = settings.value(Settings::Layout::ScreenName).toString();
    const auto screen = name.toStdString();
    if (name.isEmpty()) {
      cells.append(QString());
      continue;
    }
    if (!isValidScreenName(screen)) {
      LOG_WARN("ignoring screen, invalid name: %s", screen.c_str());
      cells.append(QString());
      continue;
    }
    if (!addScreen(screen)) {
      LOG_WARN("ignoring screen, duplicate name: %s", screen.c_str());
      cells.append(QString());
      continue;
    }
    cells.append(name);

    const int numModifiers = settings.beginReadArray(Settings::Layout::ScreenModifierArray);
    for (int m = 0; m < numModifiers && m < static_cast<int>(std::size(s_modifierOptions)); m++) {
      settings.setArrayIndex(m);
      const int target = settings.value(Settings::Layout::ScreenModifier, -1).toInt();
      if (target >= 0 && target < static_cast<int>(std::size(s_modifierIds)) && target != m) {
        addOption(screen, s_modifierOptions[m], s_modifierIds[target]);
      }
    }
    settings.endArray();

    OptionValue corners = s_noCornerMask;
    const int numCorners = settings.beginReadArray(Settings::Layout::ScreenSwitchCornerArray);
    for (int c = 0; c < numCorners && c < static_cast<int>(std::size(s_cornerMasks)); c++) {
      settings.setArrayIndex(c);
      if (settings.value(Settings::Layout::ScreenSwitchCorner, false).toBool()) {
        corners |= s_cornerMasks[c];
      }
    }
    settings.endArray();
    addOption(screen, kOptionScreenSwitchCorners, corners);
    addOption(screen, kOptionScreenSwitchCornerSize, settings.value(Settings::Layout::ScreenSwitchCornerSize).toInt());

    const int numFixes = settings.beginReadArray(Settings::Layout::ScreenFixArray);
    for (int f = 0; f < numFixes && f < static_cast<int>(std::size(s_fixOptions)); f++) {
      settings.setArrayIndex(f);
      addOption(screen, s_fixOptions[f], settings.value(Settings::Layout::ScreenFix, false).toBool());
    }
    settings.endArray();
  }
  settings.endArray();
  settings.endGroup();
  return cells;
}

void Config::addLinksFromSettings(const QStringList &cells)
{
  const int columns = Settings::value(Settings::Server::GridWidth).toInt();
  if (columns <= 0) {
    LOG_WARN("not linking screens, grid width missing");
    return;
  }

  for (int i = 0; i < cells.size(); i++) {
    if (cells[i].isEmpty()) {
      continue;
    }
    const auto screen = cells[i].toStdString();

    using enum Direction;
    const struct
    {
      int index;
      bool valid;
      Direction dir;
    } neighbors[] = {
        {i - 1, i % columns != 0, Left},
        {i + 1, (i + 1) % columns != 0, Right},
        {i - columns, true, Top},
        {i + columns, true, Bottom},
    };

    for (const auto &neighbor : neighbors) {
      if (!neighbor.valid || neighbor.index < 0 || neighbor.index >= cells.size() || cells[neighbor.index].isEmpty()) {
        continue;
      }
      const auto dstScreen = cells[neighbor.index].toStdString();
      if (!connect(screen, neighbor.dir, 0.0f, 1.0f, dstScreen, 0.0f, 1.0f)) {
        LOG_WARN("failed to link screens: %s -> %s", screen.c_str(), dstScreen.c_str());
      }
    }
  }
}

void Config::addOptionsFromSettings()
{
  if (Settings::value(Settings::Server::EnableHeatbeat).toBool()) {
    addOption("", kOptionHeartbeat, Settings::value(Settings::Server::Heartbeat).toInt());
  }

  if (Settings::value(Settings::Server::EnableSwitchDelay).toBool()) {
    addOption("", kOptionScreenSwitchDelay, Settings::value(Settings::Server::SwitchDelay).toInt());
  }

  if (Settings::value(Settings::Server::EnableSwitchDoubleTap).toBool()) {
    addOption("", kOptionScreenSwitchTwoTap, Settings::value(Settings::Server::SwitchDoubleTap).toInt());
  }

  addOption("", kOptionDefaultLockToScreenState, Settings::value(Settings::Server::DefaultLockToComputerState).toInt());
  addOption("", kOptionDisableLockToScreen, Settings::value(Settings::Server::DisableLockToComputer).toInt());
  addOption("", kOptionRelativeMouseMoves, Settings::value(Settings::Server::RelativeMouseMoves).toInt());
  addOption("", kOptionWin32KeepForeground, Settings::value(Settings::Server::Win32KeepForeground).toInt());
  addOption("", kOptionClipboardSharing, Settings::value(Settings::Server::EnableClipboard).toBool());
  addOption("", kOptionClipboardSharingSize, Settings::value(Settings::Server::ClipboardSize).toUInt() * 1024);

  if (const auto address = Settings::value(Settings::Core::Interface).toString(); !address.isEmpty()) {
    m_deskflowAddress = NetworkAddress(address.toStdString(), Settings::value(Settings::Core::Port).toInt());
  } else {
    m_deskflowAddress = NetworkAddress(Settings::value(Settings::Core::Port).toInt());
  }
  try {
    m_deskflowAddress.resolve();
  } catch (SocketAddressException &e) {
    throw ServerConfigReadException(std::string("invalid address argument ") + e.what());
  }
}

void Config::addHotkeysFromSettings()
{
  HotkeyList hotkeys;
  auto &settings = Settings::proxy();
  settings.beginGroup(Settings::Layout::Group);
  const int numHotkeys = settings.beginReadArray(Settings::Layout::HotkeysArray);
  for (int i = 0; i < numHotkeys; i++) {
    settings.setArrayIndex(i);
    Hotkey hotkey;
    hotkey.loadSettings(settings.get());
    hotkeys.append(hotkey);
  }
  settings.endArray();
  settings.endGroup();

  QString text;
  QTextStream stream(&text);
  for (const auto &hotkey : std::as_const(hotkeys)) {
    stream << hotkey;
  }

  std::istringstream lines(text.toStdString());
  ConfigReadContext context(lines);
  std::string line;
  while (context.readLine(line)) {
    parseHotkeyLine(context, line);
  }
}

void Config::parseHotkeyLine(ConfigReadContext &s, const std::string &line)
{
  // parse argument:  `nameAndArgs = [values][;[values]]'
  //   nameAndArgs  := <name>[(arg[,...])]
  //   values       := valueAndArgs[,valueAndArgs]...
  //   valueAndArgs := <value>[(arg[,...])]
  std::string::size_type i = 0;
  std::string name;
  std::string value;
  ConfigReadContext::ArgList nameArgs;
  ConfigReadContext::ArgList valueArgs;
  s.parseNameWithArgs("name", line, "=", i, name, nameArgs);
  ++i;
  s.parseNameWithArgs("value", line, ",;\n", i, value, valueArgs);

  // make filter rule
  InputFilter::Rule rule(parseCondition(s, name, nameArgs));

  // save first action (if any)
  if (!value.empty() || line[i] != ';') {
    parseAction(s, value, valueArgs, rule, true);
  }

  // get remaining activate actions
  while (i < line.length() && line[i] != ';') {
    ++i;
    s.parseNameWithArgs("value", line, ",;\n", i, value, valueArgs);
    parseAction(s, value, valueArgs, rule, true);
  }

  // get deactivate actions
  if (i < line.length() && line[i] == ';') {
    // allow trailing ';'
    i = line.find_first_not_of(" \t", i + 1);
    if (i == std::string::npos) {
      i = line.length();
    } else {
      --i;
    }

    // get actions
    while (i < line.length()) {
      ++i;
      s.parseNameWithArgs("value", line, ",\n", i, value, valueArgs);
      parseAction(s, value, valueArgs, rule, false);
    }
  }

  // add rule
  m_inputFilter.addFilterRule(rule);
}

InputFilter::Condition *
Config::parseCondition(const ConfigReadContext &s, const std::string &name, const std::vector<std::string> &args)
{
  if (name == "keystroke") {
    if (args.size() != 1) {
      throw ServerConfigReadException(s, "syntax for condition: keystroke(modifiers+key)");
    }

    IPlatformScreen::KeyInfo *keyInfo = s.parseKeystroke(args[0]);

    return new InputFilter::KeystrokeCondition(m_events, keyInfo);
  }

  if (name == "mousebutton") {
    if (args.size() != 1) {
      throw ServerConfigReadException(s, "syntax for condition: mousebutton(modifiers+button)");
    }

    auto mouseInfo = s.parseMouse(args[0]);

    return new InputFilter::MouseButtonCondition(m_events, mouseInfo);
  }

  throw ServerConfigReadException(s, "unknown argument \"%{1}\"", name);
}

void Config::parseAction(
    ConfigReadContext &s, const std::string &name, const std::vector<std::string> &args, InputFilter::Rule &rule,
    bool activate
)
{
  InputFilter::Action *action;

  if (name == "keystroke" || name == "keyDown" || name == "keyUp") {
    if (args.size() < 1 || args.size() > 2) {
      throw ServerConfigReadException(s, "syntax for action: keystroke(modifiers+key[,screens])");
    }

    IPlatformScreen::KeyInfo *keyInfo;
    if (args.size() == 1) {
      keyInfo = s.parseKeystroke(args[0]);
    } else {
      std::set<std::string> screens;
      parseScreens(s, args[1], screens);
      keyInfo = s.parseKeystroke(args[0], screens);
    }

    if (name == "keystroke") {
      IPlatformScreen::KeyInfo *keyInfo2 = IKeyState::KeyInfo::alloc(*keyInfo);
      action = new InputFilter::KeystrokeAction(m_events, keyInfo2, true);
      rule.adoptAction(action, true);
      action = new InputFilter::KeystrokeAction(m_events, keyInfo, false);
      activate = false;
    } else if (name == "keyDown") {
      action = new InputFilter::KeystrokeAction(m_events, keyInfo, true);
    } else {
      action = new InputFilter::KeystrokeAction(m_events, keyInfo, false);
    }
  }

  else if (name == "mousebutton" || name == "mouseDown" || name == "mouseUp") {
    if (args.size() != 1) {
      throw ServerConfigReadException(s, "syntax for action: mousebutton(modifiers+button)");
    }

    auto mouseInfo = s.parseMouse(args[0]);

    if (name == "mousebutton") {
      action = new InputFilter::MouseButtonAction(m_events, mouseInfo, true);
      rule.adoptAction(action, true);
      action = new InputFilter::MouseButtonAction(m_events, mouseInfo, false);
      activate = false;
    } else if (name == "mouseDown") {
      action = new InputFilter::MouseButtonAction(m_events, mouseInfo, true);
    } else {
      action = new InputFilter::MouseButtonAction(m_events, mouseInfo, false);
    }
  }

  else if (name == "switchToScreen") {
    if (args.size() != 1) {
      throw ServerConfigReadException(s, "syntax for action: switchToScreen(name)");
    }

    std::string screen = args[0];
    if (isScreen(screen)) {
      screen = getCanonicalName(screen);
    } else if (!screen.empty()) {
      throw ServerConfigReadException(s, "unknown screen name in switchToScreen");
    }

    action = new InputFilter::SwitchToScreenAction(m_events, screen);
  }

  else if (name == "switchInDirection") {
    if (args.size() != 1) {
      throw ServerConfigReadException(s, "syntax for action: switchInDirection(<left|right|up|down>)");
    }

    Direction direction;
    using enum Direction;
    if (args[0] == "left") {
      direction = Left;
    } else if (args[0] == "right") {
      direction = Right;
    } else if (args[0] == "up") {
      direction = Top;
    } else if (args[0] == "down") {
      direction = Bottom;
    } else {
      throw ServerConfigReadException(s, "unknown direction \"%{1}\" in switchToScreen", args[0]);
    }

    action = new InputFilter::SwitchInDirectionAction(m_events, direction);
  }

  else if (name == "switchToNextScreen") {
    if (!args.empty()) {
      throw ServerConfigReadException(s, "syntax for action: switchToNextScreen");
    }

    action = new InputFilter::SwitchToNextScreenAction(m_events);
  }

  else if (name == "lockCursorToScreen") {
    if (args.size() > 1) {
      throw ServerConfigReadException(s, "syntax for action: lockCursorToScreen([{off|on|toggle}])");
    }

    InputFilter::LockCursorToScreenAction::Mode mode = InputFilter::LockCursorToScreenAction::kToggle;
    if (args.size() == 1) {
      if (args[0] == "off") {
        mode = InputFilter::LockCursorToScreenAction::kOff;
      } else if (args[0] == "on") {
        mode = InputFilter::LockCursorToScreenAction::kOn;
      } else if (args[0] == "toggle") {
        mode = InputFilter::LockCursorToScreenAction::kToggle;
      } else {
        throw ServerConfigReadException(s, "syntax for action: lockCursorToScreen([{off|on|toggle}])");
      }
    }

    if (mode != InputFilter::LockCursorToScreenAction::kOff) {
      m_hasLockToScreenAction = true;
    }

    action = new InputFilter::LockCursorToScreenAction(m_events, mode);
  }

  else if (name == "restartServer") {
    if (args.size() > 1) {
      throw ServerConfigReadException(s, "syntax for action: restartServer([{{restart}}])");
    }

    InputFilter::RestartServer::Mode mode = InputFilter::RestartServer::restart;

    if (args.size() == 1) {
      if (args[0] == "restart") {
        mode = InputFilter::RestartServer::restart;
      } else {
        throw ServerConfigReadException(s, "syntax for action: restartServer([{restart}])");
      }
    }

    action = new InputFilter::RestartServer(mode);
  }

  else if (name == "keyboardBroadcast") {
    if (args.size() > 2) {
      throw ServerConfigReadException(s, "syntax for action: keyboardBroadcast([{off|on|toggle}[,screens]])");
    }

    InputFilter::KeyboardBroadcastAction::Mode mode = InputFilter::KeyboardBroadcastAction::kToggle;
    if (args.size() >= 1) {
      if (args[0] == "off") {
        mode = InputFilter::KeyboardBroadcastAction::kOff;
      } else if (args[0] == "on") {
        mode = InputFilter::KeyboardBroadcastAction::kOn;
      } else if (args[0] == "toggle") {
        mode = InputFilter::KeyboardBroadcastAction::kToggle;
      } else {
        throw ServerConfigReadException(
            s, "syntax for action: "
               "keyboardBroadcast([{off|on|toggle}[,screens]])"
        );
      }
    }

    std::set<std::string> screens;
    if (args.size() >= 2) {
      parseScreens(s, args[1], screens);
    }

    action = new InputFilter::KeyboardBroadcastAction(m_events, mode, screens);
  }

  else {
    throw ServerConfigReadException(s, "unknown action argument \"%{1}\"", name);
  }

  rule.adoptAction(action, activate);
}

void Config::parseScreens(const ConfigReadContext &c, const std::string_view &s, std::set<std::string> &screens) const
{
  screens.clear();

  std::string::size_type i = 0;
  while (i < s.size()) {
    // find end of next screen name
    std::string::size_type j = s.find(':', i);
    if (j == std::string::npos) {
      j = s.size();
    }

    // extract name
    std::string rawName;
    i = s.find_first_not_of(" \t", i);
    if (i < j) {
      rawName = s.substr(i, s.find_last_not_of(" \t", j - 1) - i + 1);
    }

    // add name
    if (rawName == "*") {
      screens.insert("*");
    } else if (!rawName.empty()) {
      std::string name = getCanonicalName(rawName);
      if (name.empty()) {
        throw ServerConfigReadException(c, "unknown screen name \"%{1}\"", rawName);
      }
      screens.insert(name);
    }

    // next
    i = j + 1;
  }
}

//
// Config::Name
//

Config::Name::Name(Config *config, const std::string &name) : m_config(config), m_name(config->getCanonicalName(name))
{
  // do nothing
}

bool Config::Name::operator==(const std::string &name) const
{
  std::string canonical = m_config->getCanonicalName(name);
  return CaselessCmp::equal(canonical, m_name);
}

//
// Config::CellEdge
//

Config::CellEdge::CellEdge(Direction side, float position)
{
  init("", side, Interval(position, position));
}

Config::CellEdge::CellEdge(Direction side, const Interval &interval)
{
  assert(interval.first >= 0.0f);
  assert(interval.second <= 1.0f);
  assert(interval.first < interval.second);

  init("", side, interval);
}

Config::CellEdge::CellEdge(const std::string &name, Direction side, const Interval &interval)
{
  assert(interval.first >= 0.0f);
  assert(interval.second <= 1.0f);
  assert(interval.first < interval.second);

  init(name, side, interval);
}

void Config::CellEdge::init(const std::string_view &name, Direction side, const Interval &interval)
{
  assert(side != Direction::NoDirection);

  m_name = name;
  m_side = side;
  m_interval = interval;
}

Config::Interval Config::CellEdge::getInterval() const
{
  return m_interval;
}

void Config::CellEdge::setName(const std::string_view &newName)
{
  m_name = newName;
}

std::string Config::CellEdge::getName() const
{
  return m_name;
}

Direction Config::CellEdge::getSide() const
{
  return m_side;
}

bool Config::CellEdge::overlaps(const CellEdge &edge) const
{
  const Interval &x = m_interval;
  const Interval &y = edge.m_interval;
  if (m_side != edge.m_side) {
    return false;
  }
  return (x.first >= y.first && x.first < y.second) || (x.second > y.first && x.second <= y.second) ||
         (y.first >= x.first && y.first < x.second) || (y.second > x.first && y.second <= x.second);
}

bool Config::CellEdge::isInside(float x) const
{
  return (x >= m_interval.first && x < m_interval.second);
}

float Config::CellEdge::transform(float x) const
{
  return (x - m_interval.first) / (m_interval.second - m_interval.first);
}

float Config::CellEdge::inverseTransform(float x) const
{
  return x * (m_interval.second - m_interval.first) + m_interval.first;
}

bool Config::CellEdge::operator<(const CellEdge &o) const
{
  if (static_cast<int>(m_side) < static_cast<int>(o.m_side)) {
    return true;
  } else if (static_cast<int>(m_side) > static_cast<int>(o.m_side)) {
    return false;
  }

  return (m_interval.first < o.m_interval.first);
}

bool Config::CellEdge::operator==(const CellEdge &x) const
{
  return (m_side == x.m_side && m_interval == x.m_interval);
}

//
// Config::Cell
//

bool Config::Cell::add(const CellEdge &src, const CellEdge &dst)
{
  // cannot add an edge that overlaps other existing edges but we
  // can exactly replace an edge.
  if (!hasEdge(src) && overlaps(src)) {
    return false;
  }

  m_neighbors.erase(src);
  m_neighbors.try_emplace(src, dst);
  return true;
}

void Config::Cell::remove(Direction side)
{
  for (auto j = m_neighbors.begin(); j != m_neighbors.end();) {
    if (j->first.getSide() == side) {
      m_neighbors.erase(j++);
    } else {
      ++j;
    }
  }
}

void Config::Cell::remove(Direction side, float position)
{
  for (auto j = m_neighbors.begin(); j != m_neighbors.end(); ++j) {
    if (j->first.getSide() == side && j->first.isInside(position)) {
      m_neighbors.erase(j);
      break;
    }
  }
}
void Config::Cell::remove(const Name &name)
{
  for (auto j = m_neighbors.begin(); j != m_neighbors.end();) {
    if (name == j->second.getName()) {
      m_neighbors.erase(j++);
    } else {
      ++j;
    }
  }
}

void Config::Cell::rename(const Name &oldName, const std::string &newName)
{
  for (auto j = m_neighbors.begin(); j != m_neighbors.end(); ++j) {
    if (oldName == j->second.getName()) {
      j->second.setName(newName);
    }
  }
}

bool Config::Cell::hasEdge(const CellEdge &edge) const
{
  EdgeLinks::const_iterator i = m_neighbors.find(edge);
  return (i != m_neighbors.end() && i->first == edge);
}

bool Config::Cell::overlaps(const CellEdge &edge) const
{
  EdgeLinks::const_iterator i = m_neighbors.upper_bound(edge);
  if (i != m_neighbors.end() && i->first.overlaps(edge)) {
    return true;
  }
  if (i != m_neighbors.begin() && (--i)->first.overlaps(edge)) {
    return true;
  }
  return false;
}

bool Config::Cell::getLink(Direction side, float position, const CellEdge *&src, const CellEdge *&dst) const
{
  CellEdge edge(side, position);
  EdgeLinks::const_iterator i = m_neighbors.upper_bound(edge);
  if (i == m_neighbors.begin()) {
    return false;
  }
  --i;
  if (i->first.getSide() == side && i->first.isInside(position)) {
    src = &i->first;
    dst = &i->second;
    return true;
  }
  return false;
}

bool Config::Cell::operator==(const Cell &x) const
{
  // compare options
  if (m_options != x.m_options) {
    return false;
  }

  // compare links
  if (m_neighbors.size() != x.m_neighbors.size()) {
    return false;
  }

  auto index2neighbors = x.m_neighbors.cbegin();
  for (auto const &index1 : m_neighbors) {
    if (index1.first != index2neighbors->first) {
      return false;
    }
    if (index1.second != index2neighbors->second) {
      return false;
    }

    // operator== doesn't compare names.  only compare destination
    // names.
    if (!CaselessCmp::equal(index1.second.getName(), index2neighbors->second.getName())) {
      return false;
    }
    ++index2neighbors;
  }

  return true;
}

Config::Cell::const_iterator Config::Cell::begin() const
{
  return m_neighbors.begin();
}

Config::Cell::const_iterator Config::Cell::end() const
{
  return m_neighbors.end();
}

//
// ConfigReadContext
//

ConfigReadContext::ConfigReadContext(std::istream &s, int32_t firstLine) : m_stream(s), m_line(firstLine - 1)
{
  // do nothing
}

bool ConfigReadContext::readLine(std::string &line)
{
  ++m_line;
  while (std::getline(m_stream, line)) {
    // strip leading whitespace
    std::string::size_type i = line.find_first_not_of(" \t");
    if (i != std::string::npos) {
      line.erase(0, i);
    }

    // strip comments and then trailing whitespace
    i = line.find('#');
    if (i != std::string::npos) {
      line.erase(i);
    }
    i = line.find_last_not_of(" \r\t");
    if (i != std::string::npos) {
      line.erase(i + 1);
    }

    // return non empty line
    if (!line.empty()) {
      // make sure there are no invalid characters
      for (i = 0; i < line.length(); ++i) {
        if (!isgraph(line[i]) && line[i] != ' ' && line[i] != '\t') {
          throw ServerConfigReadException(*this, "invalid character %{1}", deskflow::string::sprintf("%#2x", line[i]));
        }
      }

      return true;
    }

    // next line
    ++m_line;
  }
  return false;
}

uint32_t ConfigReadContext::getLineNumber() const
{
  return m_line;
}

void ConfigReadContext::parseNameWithArgs(
    const std::string &type, const std::string &line, const std::string &delim, std::string::size_type &index,
    std::string &name, ArgList &args
) const
{
  // skip leading whitespace
  std::string::size_type i = line.find_first_not_of(" \t", index);
  if (i == std::string::npos) {
    throw ServerConfigReadException(*this, std::string("missing ") + type);
  }

  // find end of name
  std::string::size_type j = line.find_first_of(" \t(" + delim, i);
  if (j == std::string::npos) {
    j = line.length();
  }

  // save name
  name = line.substr(i, j - i);
  args.clear();

  // is it okay to not find a delimiter?
  bool needDelim = (!delim.empty() && delim.find('\n') == std::string::npos);

  // skip whitespace
  i = line.find_first_not_of(" \t", j);
  if (i == std::string::npos && needDelim) {
    // expected delimiter but didn't find it
    throw ServerConfigReadException(*this, std::string("missing ") + delim[0]);
  }
  if (i == std::string::npos) {
    // no arguments
    index = line.length();
    return;
  }
  if (line[i] != '(') {
    // no arguments
    index = i;
    return;
  }

  // eat '('
  ++i;

  // parse arguments
  j = line.find_first_of(",)", i);
  while (j != std::string::npos) {
    // extract arg
    std::string arg(line.substr(i, j - i));
    i = j;

    // trim whitespace
    j = arg.find_first_not_of(" \t");
    if (j != std::string::npos) {
      arg.erase(0, j);
    }
    j = arg.find_last_not_of(" \t");
    if (j != std::string::npos) {
      arg.erase(j + 1);
    }

    // save arg
    args.push_back(arg);

    // exit loop at end of arguments
    if (line[i] == ')') {
      break;
    }

    // eat ','
    ++i;

    // next
    j = line.find_first_of(",)", i);
  }

  // verify ')'
  if (j == std::string::npos) {
    // expected )
    throw ServerConfigReadException(*this, "missing )");
  }

  // eat ')'
  ++i;

  // skip whitespace
  j = line.find_first_not_of(" \t", i);
  if (j == std::string::npos && needDelim) {
    // expected delimiter but didn't find it
    throw ServerConfigReadException(*this, std::string("missing ") + delim[0]);
  }

  // verify delimiter
  if (needDelim && delim.find(line[j]) == std::string::npos) {
    throw ServerConfigReadException(*this, std::string("expected ") + delim[0]);
  }

  if (j == std::string::npos) {
    j = line.length();
  }

  index = j;
  return;
}

IPlatformScreen::KeyInfo *ConfigReadContext::parseKeystroke(const std::string &keystroke) const
{
  return parseKeystroke(keystroke, std::set<std::string>());
}

IPlatformScreen::KeyInfo *
ConfigReadContext::parseKeystroke(const std::string &keystroke, const std::set<std::string> &screens) const
{
  std::string s = keystroke;

  KeyModifierMask mask;
  if (!deskflow::KeyMap::parseModifiers(s, mask)) {
    throw ServerConfigReadException(*this, "unable to parse key modifiers");
  }

  KeyID key;
  if (!deskflow::KeyMap::parseKey(s, key)) {
    throw ServerConfigReadException(*this, "unable to parse key");
  }

  if (key == kKeyNone && mask == 0) {
    throw ServerConfigReadException(*this, "missing key and/or modifiers in keystroke");
  }

  return IPlatformScreen::KeyInfo::alloc(key, mask, 0, 0, screens);
}

IPlatformScreen::ButtonInfo ConfigReadContext::parseMouse(const std::string &mouse) const
{
  std::string s = mouse;

  KeyModifierMask mask;
  if (!deskflow::KeyMap::parseModifiers(s, mask)) {
    throw ServerConfigReadException(*this, "unable to parse button modifiers");
  }

  char *end;
  auto button = (ButtonID)strtol(s.c_str(), &end, 10);
  if (*end != '\0') {
    throw ServerConfigReadException(*this, "unable to parse button");
  }
  if (s.empty() || button <= 0) {
    throw ServerConfigReadException(*this, "invalid button");
  }

  return IPlatformScreen::ButtonInfo{button, mask};
}

//
// Config I/O exceptions
//

ServerConfigReadException::ServerConfigReadException(const std::string &error) : m_error(error)
{
  // do nothing
}

ServerConfigReadException::ServerConfigReadException(const ConfigReadContext &context, const std::string &error)
    : m_error(deskflow::string::sprintf("line %d: %s", context.getLineNumber(), error.c_str()))
{
  // do nothing
}

ServerConfigReadException::ServerConfigReadException(
    const ConfigReadContext &context, const char *errorFmt, const std::string &arg
)
    : m_error(
          deskflow::string::sprintf("line %d: ", context.getLineNumber()) +
          deskflow::string::format(errorFmt, arg.c_str())
      )
{
  // do nothing
}

QString ServerConfigReadException::getWhat() const throw()
{
  return format("ServerConfigReadException", "read error: %{1}", m_error.c_str());
}

} // namespace deskflow::server
