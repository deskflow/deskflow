/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "server/Config.h"

#include "base/IEventQueue.h"
#include "deskflow/DeskflowException.h"
#include "deskflow/KeyMap.h"
#include "deskflow/KeyTypes.h"
#include "deskflow/OptionTypes.h"
#include "deskflow/ProtocolTypes.h"
#include "net/SocketException.h"
#include "server/Server.h"

#include <assert.h>
#include <cstdlib>
#include <istream>
#include <ostream>

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
  m_map.insert(std::make_pair(name, Cell()));

  // add name
  m_nameToCanonicalName.insert(std::make_pair(name, name));

  return true;
}

bool Config::renameScreen(const std::string &oldName, const std::string &newName)
{
  // get canonical name and find cell
  std::string oldCanonical = getCanonicalName(oldName);
  CellMap::iterator index = m_map.find(oldCanonical);
  if (index == m_map.end()) {
    return false;
  }

  // accept if names are equal but replace with new name to maintain
  // case.  otherwise, the new name must not exist.
  if (!CaselessCmp::equal(oldName, newName) && m_nameToCanonicalName.contains(newName)) {
    return false;
  }

  // update cell
  Cell tmpCell = index->second;
  m_map.erase(index);
  m_map.insert(std::make_pair(newName, tmpCell));

  // update name
  m_nameToCanonicalName.erase(oldCanonical);
  m_nameToCanonicalName.insert(std::make_pair(newName, newName));

  // update connections
  Name oldNameObj(this, oldName);
  for (index = m_map.begin(); index != m_map.end(); ++index) {
    index->second.rename(oldNameObj, newName);
  }

  // update alias targets
  if (CaselessCmp::equal(oldName, oldCanonical)) {
    for (auto iter = m_nameToCanonicalName.begin(); iter != m_nameToCanonicalName.end(); ++iter) {
      if (CaselessCmp::equal(iter->second, oldCanonical)) {
        iter->second = newName;
      }
    }
  }

  return true;
}

void Config::removeScreen(const std::string &name)
{
  // get canonical name and find cell
  std::string canonical = getCanonicalName(name);
  CellMap::iterator index = m_map.find(canonical);
  if (index == m_map.end()) {
    return;
  }

  // remove from map
  m_map.erase(index);

  // disconnect
  Name nameObj(this, name);
  for (index = m_map.begin(); index != m_map.end(); ++index) {
    index->second.remove(nameObj);
  }

  // remove aliases (and canonical name)
  for (auto iter = m_nameToCanonicalName.begin(); iter != m_nameToCanonicalName.end();) {
    if (iter->second == canonical) {
      m_nameToCanonicalName.erase(iter++);
    } else {
      ++iter;
    }
  }
}

void Config::removeAllScreens()
{
  m_map.clear();
  m_nameToCanonicalName.clear();
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
  m_nameToCanonicalName.insert(std::make_pair(alias, canonical));

  return true;
}

bool Config::removeAlias(const std::string &alias)
{
  // must not be a canonical name
  if (m_map.contains(alias)) {
    return false;
  }

  // find alias
  NameMap::iterator index = m_nameToCanonicalName.find(alias);
  if (index == m_nameToCanonicalName.end()) {
    return false;
  }

  // remove alias
  m_nameToCanonicalName.erase(index);

  return true;
}

bool Config::removeAliases(const std::string &canonical)
{
  // must be a canonical name
  if (!m_map.contains(canonical)) {
    return false;
  }

  // find and removing matching aliases
  for (auto index = m_nameToCanonicalName.begin(); index != m_nameToCanonicalName.end();) {
    if (index->second == canonical && index->first != canonical) {
      m_nameToCanonicalName.erase(index++);
    } else {
      ++index;
    }
  }

  return true;
}

void Config::removeAllAliases()
{
  // remove all names
  m_nameToCanonicalName.clear();

  // put the canonical names back in
  for (auto index = m_map.begin(); index != m_map.end(); ++index) {
    m_nameToCanonicalName.insert(std::make_pair(index->first, index->first));
  }
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

bool Config::removeOption(const std::string &name, OptionID option)
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

  // remove option
  options->erase(option);
  return true;
}

bool Config::removeOptions(const std::string &name)
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

  // remove options
  options->clear();
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

void Config::read(ConfigReadContext &context)
{
  Config tmp(m_events);
  while (context.getStream()) {
    tmp.readSection(context);
  }
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

std::string Config::formatInterval(const Interval &x)
{
  if (x.first == 0.0f && x.second == 1.0f) {
    return "";
  }
  return deskflow::string::sprintf("(%d,%d)", (int)(x.first * 100.0f + 0.5f), (int)(x.second * 100.0f + 0.5f));
}

void Config::readSection(ConfigReadContext &s)
{
  static const char s_section[] = "section:";
  static const char s_options[] = "options";
  static const char s_screens[] = "screens";
  static const char s_links[] = "links";
  static const char s_aliases[] = "aliases";

  std::string line;
  if (!s.readLine(line)) {
    // no more sections
    return;
  }

  // should be a section header
  if (line.find(s_section) != 0) {
    throw ServerConfigReadException(s, "found data outside section");
  }

  // get section name
  std::string::size_type i = line.find_first_not_of(" \t", sizeof(s_section) - 1);
  if (i == std::string::npos) {
    throw ServerConfigReadException(s, "section name is missing");
  }
  std::string name = line.substr(i);
  i = name.find_first_of(" \t");
  if (i != std::string::npos) {
    throw ServerConfigReadException(s, "unexpected data after section name");
  }

  // read section
  if (name == s_options) {
    readSectionOptions(s);
  } else if (name == s_screens) {
    readSectionScreens(s);
  } else if (name == s_links) {
    readSectionLinks(s);
  } else if (name == s_aliases) {
    readSectionAliases(s);
  } else {
    throw ServerConfigReadException(s, "unknown section name \"%{1}\"", name);
  }
}

void Config::readSectionOptions(ConfigReadContext &s)
{
  std::string line;
  while (s.readLine(line)) {
    // check for end of section
    if (line == "end") {
      return;
    }

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

    bool handled = true;
    if (name == "address") {
      try {
        m_deskflowAddress = NetworkAddress(value, kDefaultPort);
        m_deskflowAddress.resolve();
      } catch (SocketAddressException &e) {
        throw ServerConfigReadException(s, std::string("invalid address argument ") + e.what());
      }
    } else if (name == "heartbeat") {
      addOption("", kOptionHeartbeat, s.parseInt(value));
    } else if (name == "protocol") {
      addOption("", kOptionProtocol, s.parseProtocol(value));
    } else if (name == "switchCorners") {
      addOption("", kOptionScreenSwitchCorners, s.parseCorners(value));
    } else if (name == "switchCornerSize") {
      addOption("", kOptionScreenSwitchCornerSize, s.parseInt(value));
    } else if (name == "switchDelay") {
      addOption("", kOptionScreenSwitchDelay, s.parseInt(value));
    } else if (name == "switchDoubleTap") {
      addOption("", kOptionScreenSwitchTwoTap, s.parseInt(value));
    } else if (name == "switchNeedsShift") {
      addOption("", kOptionScreenSwitchNeedsShift, s.parseBoolean(value));
    } else if (name == "switchNeedsControl") {
      addOption("", kOptionScreenSwitchNeedsControl, s.parseBoolean(value));
    } else if (name == "switchNeedsAlt") {
      addOption("", kOptionScreenSwitchNeedsAlt, s.parseBoolean(value));
    } else if (name == "relativeMouseMoves") {
      addOption("", kOptionRelativeMouseMoves, s.parseBoolean(value));
    } else if (name == "win32KeepForeground") {
      addOption("", kOptionWin32KeepForeground, s.parseBoolean(value));
    } else if (name == "defaultLockToScreenState") {
      addOption("", kOptionDefaultLockToScreenState, s.parseBoolean(value));
    } else if (name == "disableLockToScreen") {
      addOption("", kOptionDisableLockToScreen, s.parseBoolean(value));
    } else if (name == "clipboardSharing") {
      addOption("", kOptionClipboardSharing, s.parseBoolean(value));
    } else if (name == "clipboardSharingSize") {
      addOption("", kOptionClipboardSharingSize, s.parseInt(value));
    } else {
      handled = false;
    }

    if (handled) {
      // make sure handled options aren't followed by more values
      if (i < line.size() && (line[i] == ',' || line[i] == ';')) {
        throw ServerConfigReadException(s, std::string("too many arguments for: ").append(name));
      }
    } else {
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
  }
  throw ServerConfigReadException(s, "unexpected end of options section");
}

void Config::readSectionScreens(ConfigReadContext &s)
{
  std::string line;
  std::string screen;
  while (s.readLine(line)) {
    // check for end of section
    if (line == "end") {
      return;
    }

    // see if it's the next screen
    if (line[line.size() - 1] == ':') {
      // strip :
      screen = line.substr(0, line.size() - 1);

      // verify validity of screen name
      if (!isValidScreenName(screen)) {
        throw ServerConfigReadException(s, "invalid screen name \"%{1}\"", screen);
      }

      // add the screen to the configuration
      if (!addScreen(screen)) {
        throw ServerConfigReadException(s, "duplicate screen name \"%{1}\"", screen);
      }
    } else if (screen.empty()) {
      throw ServerConfigReadException(s, "argument before first screen");
    } else {
      // parse argument:  `<name>=<value>'
      std::string::size_type i = line.find_first_of(" \t=");
      if (i == 0) {
        throw ServerConfigReadException(s, "missing argument name");
      }
      if (i == std::string::npos) {
        throw ServerConfigReadException(s, "missing =");
      }
      std::string name = line.substr(0, i);
      i = line.find_first_not_of(" \t", i);
      if (i == std::string::npos || line[i] != '=') {
        throw ServerConfigReadException(s, "missing =");
      }
      i = line.find_first_not_of(" \t", i + 1);
      std::string value;
      if (i != std::string::npos) {
        value = line.substr(i);
      }

      // handle argument
      if (name == "halfDuplexCapsLock") {
        addOption(screen, kOptionHalfDuplexCapsLock, s.parseBoolean(value));
      } else if (name == "halfDuplexNumLock") {
        addOption(screen, kOptionHalfDuplexNumLock, s.parseBoolean(value));
      } else if (name == "halfDuplexScrollLock") {
        addOption(screen, kOptionHalfDuplexScrollLock, s.parseBoolean(value));
      } else if (name == "shift") {
        addOption(screen, kOptionModifierMapForShift, s.parseModifierKey(value));
      } else if (name == "ctrl") {
        addOption(screen, kOptionModifierMapForControl, s.parseModifierKey(value));
      } else if (name == "alt") {
        addOption(screen, kOptionModifierMapForAlt, s.parseModifierKey(value));
      } else if (name == "altgr") {
        addOption(screen, kOptionModifierMapForAltGr, s.parseModifierKey(value));
      } else if (name == "meta") {
        addOption(screen, kOptionModifierMapForMeta, s.parseModifierKey(value));
      } else if (name == "super") {
        addOption(screen, kOptionModifierMapForSuper, s.parseModifierKey(value));
      } else if (name == "xtestIsXineramaUnaware") {
        addOption(screen, kOptionXTestXineramaUnaware, s.parseBoolean(value));
      } else if (name == "switchCorners") {
        addOption(screen, kOptionScreenSwitchCorners, s.parseCorners(value));
      } else if (name == "switchCornerSize") {
        addOption(screen, kOptionScreenSwitchCornerSize, s.parseInt(value));
      } else if (name == "preserveFocus") {
        addOption(screen, kOptionScreenPreserveFocus, s.parseBoolean(value));
      } else {
        // unknown argument
        throw ServerConfigReadException(s, "unknown argument \"%{1}\"", name);
      }
    }
  }
  throw ServerConfigReadException(s, "unexpected end of screens section");
}

void Config::readSectionLinks(ConfigReadContext &s)
{
  std::string line;
  std::string screen;
  while (s.readLine(line)) {
    // check for end of section
    if (line == "end") {
      return;
    }

    // see if it's the next screen
    if (line[line.size() - 1] == ':') {
      // strip :
      screen = line.substr(0, line.size() - 1);

      // verify we know about the screen
      if (!isScreen(screen)) {
        throw ServerConfigReadException(s, "unknown screen name \"%{1}\"", screen);
      }
      if (!isCanonicalName(screen)) {
        throw ServerConfigReadException(s, "cannot use screen name alias here");
      }
    } else if (screen.empty()) {
      throw ServerConfigReadException(s, "argument before first screen");
    } else {
      // parse argument:  `<name>[(<s0>,<e0>)]=<value>[(<s1>,<e1>)]'
      // the stuff in brackets is optional.  interval values must be
      // in the range [0,100] and start < end.  if not given the
      // interval is taken to be (0,100).
      std::string::size_type i = 0;
      std::string side;
      std::string dstScreen;
      std::string srcArgString;
      std::string dstArgString;
      ConfigReadContext::ArgList srcArgs;
      ConfigReadContext::ArgList dstArgs;
      s.parseNameWithArgs("link", line, "=", i, side, srcArgs);
      ++i;
      s.parseNameWithArgs("screen", line, "", i, dstScreen, dstArgs);
      Interval srcInterval(s.parseInterval(srcArgs));
      Interval dstInterval(s.parseInterval(dstArgs));

      // handle argument
      using enum Direction;
      Direction dir;
      if (side == "left") {
        dir = Left;
      } else if (side == "right") {
        dir = Right;
      } else if (side == "up") {
        dir = Top;
      } else if (side == "down") {
        dir = Bottom;
      } else {
        // unknown argument
        throw ServerConfigReadException(s, "unknown side \"%{1}\" in link", side);
      }
      if (!isScreen(dstScreen)) {
        throw ServerConfigReadException(s, "unknown screen name \"%{1}\"", dstScreen);
      }
      if (!connect(
              screen, dir, srcInterval.first, srcInterval.second, dstScreen, dstInterval.first, dstInterval.second
          )) {
        throw ServerConfigReadException(s, "overlapping range");
      }
    }
  }
  throw ServerConfigReadException(s, "unexpected end of links section");
}

void Config::readSectionAliases(ConfigReadContext &s)
{
  std::string line;
  std::string screen;
  while (s.readLine(line)) {
    // check for end of section
    if (line == "end") {
      return;
    }

    // see if it's the next screen
    if (line[line.size() - 1] == ':') {
      // strip :
      screen = line.substr(0, line.size() - 1);

      // verify we know about the screen
      if (!isScreen(screen)) {
        throw ServerConfigReadException(s, "unknown screen name \"%{1}\"", screen);
      }
      if (!isCanonicalName(screen)) {
        throw ServerConfigReadException(s, "cannot use screen name alias here");
      }
    } else if (screen.empty()) {
      throw ServerConfigReadException(s, "argument before first screen");
    } else {
      // verify validity of screen name
      if (!isValidScreenName(line)) {
        throw ServerConfigReadException(s, "invalid screen alias \"%{1}\"", line);
      }

      // add alias
      if (!addAlias(screen, line)) {
        throw ServerConfigReadException(s, "alias \"%{1}\" is already used", line);
      }
    }
  }
  throw ServerConfigReadException(s, "unexpected end of aliases section");
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

  if (name == "connect") {
    if (args.size() != 1) {
      throw ServerConfigReadException(s, "syntax for condition: connect([screen])");
    }

    std::string screen = args[0];
    if (isScreen(screen)) {
      screen = getCanonicalName(screen);
    } else if (!screen.empty()) {
      throw ServerConfigReadException(s, "unknown screen name \"%{1}\" in connect", screen);
    }

    return new InputFilter::ScreenConnectedCondition(m_events, screen);
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

  /* XXX -- not supported
          else if (name == "modifier") {
                  if (args.size() != 1) {
                          throw ServerConfigReadException(s, "syntax for action:
     modifier(modifiers)");
                  }

                  KeyModifierMask mask = s.parseModifier(args[0]);

                  action = new InputFilter::ModifierAction(mask, ~mask);
          }
  */

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

const char *Config::getOptionName(OptionID id)
{
  if (id == kOptionHalfDuplexCapsLock) {
    return "halfDuplexCapsLock";
  }
  if (id == kOptionHalfDuplexNumLock) {
    return "halfDuplexNumLock";
  }
  if (id == kOptionHalfDuplexScrollLock) {
    return "halfDuplexScrollLock";
  }
  if (id == kOptionModifierMapForShift) {
    return "shift";
  }
  if (id == kOptionModifierMapForControl) {
    return "ctrl";
  }
  if (id == kOptionModifierMapForAlt) {
    return "alt";
  }
  if (id == kOptionModifierMapForAltGr) {
    return "altgr";
  }
  if (id == kOptionModifierMapForMeta) {
    return "meta";
  }
  if (id == kOptionModifierMapForSuper) {
    return "super";
  }
  if (id == kOptionHeartbeat) {
    return "heartbeat";
  }
  if (id == kOptionScreenSwitchCorners) {
    return "switchCorners";
  }
  if (id == kOptionScreenSwitchCornerSize) {
    return "switchCornerSize";
  }
  if (id == kOptionScreenSwitchDelay) {
    return "switchDelay";
  }
  if (id == kOptionScreenSwitchTwoTap) {
    return "switchDoubleTap";
  }
  if (id == kOptionScreenSwitchNeedsShift) {
    return "switchNeedsShift";
  }
  if (id == kOptionScreenSwitchNeedsControl) {
    return "switchNeedsControl";
  }
  if (id == kOptionScreenSwitchNeedsAlt) {
    return "switchNeedsAlt";
  }
  if (id == kOptionXTestXineramaUnaware) {
    return "xtestIsXineramaUnaware";
  }
  if (id == kOptionRelativeMouseMoves) {
    return "relativeMouseMoves";
  }
  if (id == kOptionWin32KeepForeground) {
    return "win32KeepForeground";
  }
  if (id == kOptionScreenPreserveFocus) {
    return "preserveFocus";
  }
  if (id == kOptionDefaultLockToScreenState) {
    return "defaultLockToScreenState";
  }
  if (id == kOptionDisableLockToScreen) {
    return "disableLockToScreen";
  }
  if (id == kOptionClipboardSharing) {
    return "clipboardSharing";
  }
  if (id == kOptionClipboardSharingSize) {
    return "clipboardSharingSize";
  }
  return nullptr;
}

std::string Config::getOptionValue(OptionID id, OptionValue value)
{
  if (id == kOptionHalfDuplexCapsLock || id == kOptionHalfDuplexNumLock || id == kOptionHalfDuplexScrollLock ||
      id == kOptionScreenSwitchNeedsShift || id == kOptionScreenSwitchNeedsControl ||
      id == kOptionScreenSwitchNeedsAlt || id == kOptionXTestXineramaUnaware || id == kOptionRelativeMouseMoves ||
      id == kOptionWin32KeepForeground || id == kOptionScreenPreserveFocus || id == kOptionClipboardSharing ||
      id == kOptionClipboardSharingSize) {
    return (value != 0) ? "true" : "false";
  }
  if (id == kOptionModifierMapForShift || id == kOptionModifierMapForControl || id == kOptionModifierMapForAlt ||
      id == kOptionModifierMapForAltGr || id == kOptionModifierMapForMeta || id == kOptionModifierMapForSuper) {
    switch (value) {
    case kKeyModifierIDShift:
      return "shift";

    case kKeyModifierIDControl:
      return "ctrl";

    case kKeyModifierIDAlt:
      return "alt";

    case kKeyModifierIDAltGr:
      return "altgr";

    case kKeyModifierIDMeta:
      return "meta";

    case kKeyModifierIDSuper:
      return "super";

    default:
      return "none";
    }
  }
  if (id == kOptionHeartbeat || id == kOptionScreenSwitchCornerSize || id == kOptionScreenSwitchDelay ||
      id == kOptionScreenSwitchTwoTap) {
    return deskflow::string::sprintf("%d", value);
  }
  if (id == kOptionScreenSwitchCorners) {
    std::string result("none");
    if ((value & s_topLeftCornerMask) != 0) {
      result += " +top-left";
    }
    if ((value & s_topRightCornerMask) != 0) {
      result += " +top-right";
    }
    if ((value & s_bottomLeftCornerMask) != 0) {
      result += " +bottom-left";
    }
    if ((value & s_bottomRightCornerMask) != 0) {
      result += " +bottom-right";
    }
    return result;
  }
  if (id == kOptionProtocol) {
    const auto enumValue = networkProtocolFromInt(value);
    if (enumValue == NetworkProtocol::Unknown)
      throw InvalidProtocolException();
    return networkProtocolToOption(enumValue).toStdString();
  }

  return "";
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
  m_neighbors.insert(std::make_pair(src, dst));
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
// Config I/O
//

std::istream &operator>>(std::istream &s, Config &config)
{
  ConfigReadContext context(s);
  config.read(context);
  return s;
}

std::ostream &operator<<(std::ostream &s, const Config &config)
{
  // screens section
  s << "section: screens" << std::endl;
  for (const auto &screen : config) {
    s << "\t" << screen.c_str() << ":" << std::endl;
    const auto options = config.getOptions(screen);
    if (options != nullptr && options->size() > 0) {
      for (auto [optionId, optionValue] : *options) {
        const char *name = Config::getOptionName(optionId);
        std::string value = Config::getOptionValue(optionId, optionValue);
        if (name != nullptr && !value.empty()) {
          s << "\t\t" << name << " = " << value << std::endl;
        }
      }
    }
  }
  s << "end" << std::endl;

  // links section
  std::string neighbor;
  s << "section: links" << std::endl;
  for (const auto &screen : config) {
    s << "\t" << screen.c_str() << ":" << std::endl;

    for (Config::link_const_iterator link = config.beginNeighbor(screen), nend = config.endNeighbor(screen);
         link != nend; ++link) {
      s << "\t\t" << Config::dirName(link->first.getSide()) << Config::formatInterval(link->first.getInterval())
        << " = " << link->second.getName().c_str() << Config::formatInterval(link->second.getInterval()) << std::endl;
    }
  }
  s << "end" << std::endl;

  // aliases section (if there are any)
  if (config.m_map.size() != config.m_nameToCanonicalName.size()) {
    // map canonical to alias
    using CMNameMap = std::multimap<std::string, std::string, CaselessCmp>;
    CMNameMap aliases;
    for (auto index = config.m_nameToCanonicalName.begin(); index != config.m_nameToCanonicalName.end(); ++index) {
      if (index->first != index->second) {
        aliases.insert(std::make_pair(index->second, index->first));
      }
    }

    // dump it
    std::string screen;
    s << "section: aliases" << std::endl;
    for (CMNameMap::const_iterator index = aliases.begin(); index != aliases.end(); ++index) {
      if (index->first != screen) {
        screen = index->first;
        s << "\t" << screen.c_str() << ":" << std::endl;
      }
      s << "\t\t" << index->second.c_str() << std::endl;
    }
    s << "end" << std::endl;
  }

  // options section
  s << "section: options" << std::endl;
  if (const Config::ScreenOptions *options = config.getOptions(""); options && options->size() > 0) {
    for (auto [optionId, optionValue] : *options) {
      const char *name = Config::getOptionName(optionId);
      std::string value = Config::getOptionValue(optionId, optionValue);
      if (name != nullptr && !value.empty()) {
        s << "\t" << name << " = " << value << std::endl;
      }
    }
  }
  if (config.m_deskflowAddress.isValid()) {
    s << "\taddress = " << config.m_deskflowAddress.getHostname().c_str() << std::endl;
  }
  s << config.m_inputFilter.format("\t");
  s << "end" << std::endl;

  return s;
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

bool ConfigReadContext::operator!() const
{
  return !m_stream;
}

OptionValue ConfigReadContext::parseBoolean(const std::string &arg) const
{
  if (CaselessCmp::equal(arg, "true")) {
    return static_cast<OptionValue>(true);
  }
  if (CaselessCmp::equal(arg, "false")) {
    return static_cast<OptionValue>(false);
  }
  throw ServerConfigReadException(*this, "invalid boolean argument \"%{1}\"", arg);
}

OptionValue ConfigReadContext::parseInt(const std::string &arg) const
{
  const char *s = arg.c_str();
  char *end;
  long tmp = strtol(s, &end, 10);
  if (*end != '\0') {
    // invalid characters
    throw ServerConfigReadException(*this, "invalid integer argument \"%{1}\"", arg);
  }
  auto value = static_cast<OptionValue>(tmp);
  if (value != tmp) {
    // out of range
    throw ServerConfigReadException(*this, "integer argument \"%{1}\" out of range", arg);
  }
  return value;
}

OptionValue ConfigReadContext::parseModifierKey(const std::string &arg) const
{
  if (CaselessCmp::equal(arg, "shift")) {
    return static_cast<OptionValue>(kKeyModifierIDShift);
  }
  if (CaselessCmp::equal(arg, "ctrl")) {
    return static_cast<OptionValue>(kKeyModifierIDControl);
  }
  if (CaselessCmp::equal(arg, "alt")) {
    return static_cast<OptionValue>(kKeyModifierIDAlt);
  }
  if (CaselessCmp::equal(arg, "altgr")) {
    return static_cast<OptionValue>(kKeyModifierIDAltGr);
  }
  if (CaselessCmp::equal(arg, "meta")) {
    return static_cast<OptionValue>(kKeyModifierIDMeta);
  }
  if (CaselessCmp::equal(arg, "super")) {
    return static_cast<OptionValue>(kKeyModifierIDSuper);
  }
  if (CaselessCmp::equal(arg, "none")) {
    return static_cast<OptionValue>(kKeyModifierIDNull);
  }
  throw ServerConfigReadException(*this, "invalid argument \"%{1}\"", arg);
}

OptionValue ConfigReadContext::parseCorner(const std::string &arg) const
{
  if (CaselessCmp::equal(arg, "left")) {
    return s_topLeftCornerMask | s_bottomLeftCornerMask;
  } else if (CaselessCmp::equal(arg, "right")) {
    return s_topRightCornerMask | s_bottomRightCornerMask;
  } else if (CaselessCmp::equal(arg, "top")) {
    return s_topLeftCornerMask | s_topRightCornerMask;
  } else if (CaselessCmp::equal(arg, "bottom")) {
    return s_bottomLeftCornerMask | s_bottomRightCornerMask;
  } else if (CaselessCmp::equal(arg, "top-left")) {
    return s_topLeftCornerMask;
  } else if (CaselessCmp::equal(arg, "top-right")) {
    return s_topRightCornerMask;
  } else if (CaselessCmp::equal(arg, "bottom-left")) {
    return s_bottomLeftCornerMask;
  } else if (CaselessCmp::equal(arg, "bottom-right")) {
    return s_bottomRightCornerMask;
  } else if (CaselessCmp::equal(arg, "none")) {
    return s_noCornerMask;
  } else if (CaselessCmp::equal(arg, "all")) {
    return s_allCornersMask;
  }
  throw ServerConfigReadException(*this, "invalid argument \"%{1}\"", arg);
}

OptionValue ConfigReadContext::parseProtocol(const std::string &args) const
{
  const auto protoValue = networkProtocolFromString(QString::fromStdString(args));
  if (protoValue == NetworkProtocol::Unknown)
    throw ServerConfigReadException(*this, "invalid protocol argument \"%{1}\"", args);
  return static_cast<OptionValue>(protoValue);
}

OptionValue ConfigReadContext::parseCorners(const std::string &args) const
{
  // find first token
  std::string::size_type i = args.find_first_not_of(" \t", 0);
  if (i == std::string::npos) {
    throw ServerConfigReadException(*this, "missing corner argument");
  }
  std::string::size_type j = args.find_first_of(" \t", i);

  // parse first corner token
  OptionValue corners = parseCorner(args.substr(i, j - i));

  // get +/-
  i = args.find_first_not_of(" \t", j);
  while (i != std::string::npos) {
    // parse +/-
    bool add;
    if (args[i] == '-') {
      add = false;
    } else if (args[i] == '+') {
      add = true;
    } else {
      throw ServerConfigReadException(*this, "invalid corner operator \"%{1}\"", std::string(args.c_str() + i, 1));
    }

    // get next corner token
    i = args.find_first_not_of(" \t", i + 1);
    j = args.find_first_of(" \t", i);
    if (i == std::string::npos) {
      throw ServerConfigReadException(*this, "missing corner argument");
    }

    // parse next corner token
    if (add) {
      corners |= parseCorner(args.substr(i, j - i));
    } else {
      corners &= ~parseCorner(args.substr(i, j - i));
    }
    i = args.find_first_not_of(" \t", j);
  }

  return corners;
}

Config::Interval ConfigReadContext::parseInterval(const ArgList &args) const
{
  if (args.size() == 0) {
    return Config::Interval(0.0f, 1.0f);
  }
  if (args.size() != 2 || args[0].empty() || args[1].empty()) {
    throw ServerConfigReadException(*this, "invalid interval \"%{1}\"", concatArgs(args));
  }

  char *end;
  double startValue = strtod(args[0].c_str(), &end);
  if (end[0] != '\0') {
    throw ServerConfigReadException(*this, "invalid interval \"%{1}\"", concatArgs(args));
  }
  double endValue = strtod(args[1].c_str(), &end);
  if (end[0] != '\0') {
    throw ServerConfigReadException(*this, "invalid interval \"%{1}\"", concatArgs(args));
  }

  if (startValue < 0 || startValue > 100 || endValue < 0 || endValue > 100 || startValue >= endValue) {
    throw ServerConfigReadException(*this, "invalid interval \"%{1}\"", concatArgs(args));
  }

  auto startInterval = static_cast<float>(startValue / 100.0f);
  auto endInterval = static_cast<float>(endValue / 100.0f);
  return Config::Interval(startInterval, endInterval);
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

KeyModifierMask ConfigReadContext::parseModifier(const std::string &modifiers) const
{
  std::string s = modifiers;

  KeyModifierMask mask;
  if (!deskflow::KeyMap::parseModifiers(s, mask)) {
    throw ServerConfigReadException(*this, "unable to parse modifiers");
  }

  if (mask == 0) {
    throw ServerConfigReadException(*this, "no modifiers specified");
  }

  return mask;
}

std::string ConfigReadContext::concatArgs(const ArgList &args)
{
  std::string s("(");
  for (size_t i = 0; i < args.size(); ++i) {
    if (i != 0) {
      s += ",";
    }
    s += args[i];
  }
  s += ")";
  return s;
}

//
// Config I/O exceptions
//

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

std::string ServerConfigReadException::getWhat() const throw()
{
  return format("ServerConfigReadException", "read error: %{1}", m_error.c_str());
}

} // namespace deskflow::server
