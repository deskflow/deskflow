/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "server/Config.h"

#include "server/Server.h"
#include "synergy/KeyMap.h"
#include "synergy/key_types.h"
#include "net/XSocket.h"
#include "base/IEventQueue.h"
#include "common/stdistream.h"
#include "common/stdostream.h"

#include <cstdlib>

using namespace synergy::string;

//
// Config
//

Config::Config (IEventQueue* events)
    : m_inputFilter (events),
      m_hasLockToScreenAction (false),
      m_events (events) {
    // do nothing
}

Config::~Config () {
    // do nothing
}

bool
Config::addScreen (const String& name) {
    // alias name must not exist
    if (m_nameToCanonicalName.find (name) != m_nameToCanonicalName.end ()) {
        return false;
    }

    // add cell
    m_map.insert (std::make_pair (name, Cell ()));

    // add name
    m_nameToCanonicalName.insert (std::make_pair (name, name));

    return true;
}

bool
Config::renameScreen (const String& oldName, const String& newName) {
    // get canonical name and find cell
    String oldCanonical     = getCanonicalName (oldName);
    CellMap::iterator index = m_map.find (oldCanonical);
    if (index == m_map.end ()) {
        return false;
    }

    // accept if names are equal but replace with new name to maintain
    // case.  otherwise, the new name must not exist.
    if (!CaselessCmp::equal (oldName, newName) &&
        m_nameToCanonicalName.find (newName) != m_nameToCanonicalName.end ()) {
        return false;
    }

    // update cell
    Cell tmpCell = index->second;
    m_map.erase (index);
    m_map.insert (std::make_pair (newName, tmpCell));

    // update name
    m_nameToCanonicalName.erase (oldCanonical);
    m_nameToCanonicalName.insert (std::make_pair (newName, newName));

    // update connections
    Name oldNameObj (this, oldName);
    for (index = m_map.begin (); index != m_map.end (); ++index) {
        index->second.rename (oldNameObj, newName);
    }

    // update alias targets
    if (CaselessCmp::equal (oldName, oldCanonical)) {
        for (NameMap::iterator iter = m_nameToCanonicalName.begin ();
             iter != m_nameToCanonicalName.end ();
             ++iter) {
            if (CaselessCmp::equal (iter->second, oldCanonical)) {
                iter->second = newName;
            }
        }
    }

    return true;
}

void
Config::removeScreen (const String& name) {
    // get canonical name and find cell
    String canonical        = getCanonicalName (name);
    CellMap::iterator index = m_map.find (canonical);
    if (index == m_map.end ()) {
        return;
    }

    // remove from map
    m_map.erase (index);

    // disconnect
    Name nameObj (this, name);
    for (index = m_map.begin (); index != m_map.end (); ++index) {
        index->second.remove (nameObj);
    }

    // remove aliases (and canonical name)
    for (NameMap::iterator iter = m_nameToCanonicalName.begin ();
         iter != m_nameToCanonicalName.end ();) {
        if (iter->second == canonical) {
            m_nameToCanonicalName.erase (iter++);
        } else {
            ++index;
        }
    }
}

void
Config::removeAllScreens () {
    m_map.clear ();
    m_nameToCanonicalName.clear ();
}

bool
Config::addAlias (const String& canonical, const String& alias) {
    // alias name must not exist
    if (m_nameToCanonicalName.find (alias) != m_nameToCanonicalName.end ()) {
        return false;
    }

    // canonical name must be known
    if (m_map.find (canonical) == m_map.end ()) {
        return false;
    }

    // insert alias
    m_nameToCanonicalName.insert (std::make_pair (alias, canonical));

    return true;
}

bool
Config::removeAlias (const String& alias) {
    // must not be a canonical name
    if (m_map.find (alias) != m_map.end ()) {
        return false;
    }

    // find alias
    NameMap::iterator index = m_nameToCanonicalName.find (alias);
    if (index == m_nameToCanonicalName.end ()) {
        return false;
    }

    // remove alias
    m_nameToCanonicalName.erase (index);

    return true;
}

bool
Config::removeAliases (const String& canonical) {
    // must be a canonical name
    if (m_map.find (canonical) == m_map.end ()) {
        return false;
    }

    // find and removing matching aliases
    for (NameMap::iterator index = m_nameToCanonicalName.begin ();
         index != m_nameToCanonicalName.end ();) {
        if (index->second == canonical && index->first != canonical) {
            m_nameToCanonicalName.erase (index++);
        } else {
            ++index;
        }
    }

    return true;
}

void
Config::removeAllAliases () {
    // remove all names
    m_nameToCanonicalName.clear ();

    // put the canonical names back in
    for (CellMap::iterator index = m_map.begin (); index != m_map.end ();
         ++index) {
        m_nameToCanonicalName.insert (
            std::make_pair (index->first, index->first));
    }
}

bool
Config::connect (const String& srcName, EDirection srcSide, float srcStart,
                 float srcEnd, const String& dstName, float dstStart,
                 float dstEnd) {
    assert (srcSide >= kFirstDirection && srcSide <= kLastDirection);

    // find source cell
    CellMap::iterator index = m_map.find (getCanonicalName (srcName));
    if (index == m_map.end ()) {
        return false;
    }

    // add link
    CellEdge srcEdge (srcSide, Interval (srcStart, srcEnd));
    CellEdge dstEdge (dstName, srcSide, Interval (dstStart, dstEnd));
    return index->second.add (srcEdge, dstEdge);
}

bool
Config::disconnect (const String& srcName, EDirection srcSide) {
    assert (srcSide >= kFirstDirection && srcSide <= kLastDirection);

    // find source cell
    CellMap::iterator index = m_map.find (srcName);
    if (index == m_map.end ()) {
        return false;
    }

    // disconnect side
    index->second.remove (srcSide);

    return true;
}

bool
Config::disconnect (const String& srcName, EDirection srcSide, float position) {
    assert (srcSide >= kFirstDirection && srcSide <= kLastDirection);

    // find source cell
    CellMap::iterator index = m_map.find (srcName);
    if (index == m_map.end ()) {
        return false;
    }

    // disconnect side
    index->second.remove (srcSide, position);

    return true;
}

void
Config::setSynergyAddress (const NetworkAddress& addr) {
    m_synergyAddress = addr;
}

bool
Config::addOption (const String& name, OptionID option, OptionValue value) {
    // find options
    ScreenOptions* options = NULL;
    if (name.empty ()) {
        options = &m_globalOptions;
    } else {
        CellMap::iterator index = m_map.find (name);
        if (index != m_map.end ()) {
            options = &index->second.m_options;
        }
    }
    if (options == NULL) {
        return false;
    }

    // add option
    options->insert (std::make_pair (option, value));
    return true;
}

bool
Config::removeOption (const String& name, OptionID option) {
    // find options
    ScreenOptions* options = NULL;
    if (name.empty ()) {
        options = &m_globalOptions;
    } else {
        CellMap::iterator index = m_map.find (name);
        if (index != m_map.end ()) {
            options = &index->second.m_options;
        }
    }
    if (options == NULL) {
        return false;
    }

    // remove option
    options->erase (option);
    return true;
}

bool
Config::removeOptions (const String& name) {
    // find options
    ScreenOptions* options = NULL;
    if (name.empty ()) {
        options = &m_globalOptions;
    } else {
        CellMap::iterator index = m_map.find (name);
        if (index != m_map.end ()) {
            options = &index->second.m_options;
        }
    }
    if (options == NULL) {
        return false;
    }

    // remove options
    options->clear ();
    return true;
}

bool
Config::isValidScreenName (const String& name) const {
    // name is valid if matches validname
    //  name      ::= [_A-Za-z0-9] | [_A-Za-z0-9][-_A-Za-z0-9]*[_A-Za-z0-9]
    //  domain    ::= . name
    //  validname ::= name domain*
    // we also accept names ending in . because many OS X users have
    // so misconfigured their systems.

    // empty name is invalid
    if (name.empty ()) {
        return false;
    }

    // check each dot separated part
    String::size_type b = 0;
    for (;;) {
        // accept trailing .
        if (b == name.size ()) {
            break;
        }

        // find end of part
        String::size_type e = name.find ('.', b);
        if (e == String::npos) {
            e = name.size ();
        }

        // part may not be empty
        if (e - b < 1) {
            return false;
        }

        // check first and last characters
        if (!(isalnum (name[b]) || name[b] == '_') ||
            !(isalnum (name[e - 1]) || name[e - 1] == '_')) {
            return false;
        }

        // check interior characters
        for (String::size_type i = b; i < e; ++i) {
            if (!isalnum (name[i]) && name[i] != '_' && name[i] != '-') {
                return false;
            }
        }

        // next part
        if (e == name.size ()) {
            // no more parts
            break;
        }
        b = e + 1;
    }

    return true;
}

Config::const_iterator
Config::begin () const {
    return const_iterator (m_map.begin ());
}

Config::const_iterator
Config::end () const {
    return const_iterator (m_map.end ());
}

Config::all_const_iterator
Config::beginAll () const {
    return m_nameToCanonicalName.begin ();
}

Config::all_const_iterator
Config::endAll () const {
    return m_nameToCanonicalName.end ();
}

bool
Config::isScreen (const String& name) const {
    return (m_nameToCanonicalName.count (name) > 0);
}

bool
Config::isCanonicalName (const String& name) const {
    return (!name.empty () &&
            CaselessCmp::equal (getCanonicalName (name), name));
}

String
Config::getCanonicalName (const String& name) const {
    NameMap::const_iterator index = m_nameToCanonicalName.find (name);
    if (index == m_nameToCanonicalName.end ()) {
        return String ();
    } else {
        return index->second;
    }
}

String
Config::getNeighbor (const String& srcName, EDirection srcSide, float position,
                     float* positionOut) const {
    assert (srcSide >= kFirstDirection && srcSide <= kLastDirection);

    // find source cell
    CellMap::const_iterator index = m_map.find (getCanonicalName (srcName));
    if (index == m_map.end ()) {
        return String ();
    }

    // find edge
    const CellEdge *srcEdge, *dstEdge;
    if (!index->second.getLink (srcSide, position, srcEdge, dstEdge)) {
        // no neighbor
        return "";
    } else {
        // compute position on neighbor
        if (positionOut != NULL) {
            *positionOut =
                dstEdge->inverseTransform (srcEdge->transform (position));
        }

        // return neighbor's name
        return getCanonicalName (dstEdge->getName ());
    }
}

bool
Config::hasNeighbor (const String& srcName, EDirection srcSide) const {
    return hasNeighbor (srcName, srcSide, 0.0f, 1.0f);
}

bool
Config::hasNeighbor (const String& srcName, EDirection srcSide, float start,
                     float end) const {
    assert (srcSide >= kFirstDirection && srcSide <= kLastDirection);

    // find source cell
    CellMap::const_iterator index = m_map.find (getCanonicalName (srcName));
    if (index == m_map.end ()) {
        return false;
    }

    return index->second.overlaps (CellEdge (srcSide, Interval (start, end)));
}

Config::link_const_iterator
Config::beginNeighbor (const String& srcName) const {
    CellMap::const_iterator index = m_map.find (getCanonicalName (srcName));
    assert (index != m_map.end ());
    return index->second.begin ();
}

Config::link_const_iterator
Config::endNeighbor (const String& srcName) const {
    CellMap::const_iterator index = m_map.find (getCanonicalName (srcName));
    assert (index != m_map.end ());
    return index->second.end ();
}

const NetworkAddress&
Config::getSynergyAddress () const {
    return m_synergyAddress;
}

const Config::ScreenOptions*
Config::getOptions (const String& name) const {
    // find options
    const ScreenOptions* options = NULL;
    if (name.empty ()) {
        options = &m_globalOptions;
    } else {
        CellMap::const_iterator index = m_map.find (name);
        if (index != m_map.end ()) {
            options = &index->second.m_options;
        }
    }

    // return options
    return options;
}

bool
Config::hasLockToScreenAction () const {
    return m_hasLockToScreenAction;
}

bool
Config::operator== (const Config& x) const {
    if (m_synergyAddress != x.m_synergyAddress) {
        return false;
    }
    if (m_map.size () != x.m_map.size ()) {
        return false;
    }
    if (m_nameToCanonicalName.size () != x.m_nameToCanonicalName.size ()) {
        return false;
    }

    // compare global options
    if (m_globalOptions != x.m_globalOptions) {
        return false;
    }

    for (CellMap::const_iterator index1 = m_map.begin (),
                                 index2 = x.m_map.begin ();
         index1 != m_map.end ();
         ++index1, ++index2) {
        // compare names
        if (!CaselessCmp::equal (index1->first, index2->first)) {
            return false;
        }

        // compare cells
        if (index1->second != index2->second) {
            return false;
        }
    }

    for (NameMap::const_iterator index1 = m_nameToCanonicalName.begin (),
                                 index2 = x.m_nameToCanonicalName.begin ();
         index1 != m_nameToCanonicalName.end ();
         ++index1, ++index2) {
        if (!CaselessCmp::equal (index1->first, index2->first) ||
            !CaselessCmp::equal (index1->second, index2->second)) {
            return false;
        }
    }

    // compare input filters
    if (m_inputFilter != x.m_inputFilter) {
        return false;
    }

    return true;
}

bool
Config::operator!= (const Config& x) const {
    return !operator== (x);
}

void
Config::read (ConfigReadContext& context) {
    Config tmp (m_events);
    while (context.getStream ()) {
        tmp.readSection (context);
    }
    *this = tmp;
}

const char*
Config::dirName (EDirection dir) {
    static const char* s_name[] = {"left", "right", "up", "down"};

    assert (dir >= kFirstDirection && dir <= kLastDirection);

    return s_name[dir - kFirstDirection];
}

InputFilter*
Config::getInputFilter () {
    return &m_inputFilter;
}

String
Config::formatInterval (const Interval& x) {
    if (x.first == 0.0f && x.second == 1.0f) {
        return "";
    }
    return synergy::string::sprintf ("(%d,%d)",
                                     (int) (x.first * 100.0f + 0.5f),
                                     (int) (x.second * 100.0f + 0.5f));
}

void
Config::readSection (ConfigReadContext& s) {
    static const char s_section[] = "section:";
    static const char s_options[] = "options";
    static const char s_screens[] = "screens";
    static const char s_links[]   = "links";
    static const char s_aliases[] = "aliases";

    String line;
    if (!s.readLine (line)) {
        // no more sections
        return;
    }

    // should be a section header
    if (line.find (s_section) != 0) {
        throw XConfigRead (s, "found data outside section");
    }

    // get section name
    String::size_type i =
        line.find_first_not_of (" \t", sizeof (s_section) - 1);
    if (i == String::npos) {
        throw XConfigRead (s, "section name is missing");
    }
    String name = line.substr (i);
    i           = name.find_first_of (" \t");
    if (i != String::npos) {
        throw XConfigRead (s, "unexpected data after section name");
    }

    // read section
    if (name == s_options) {
        readSectionOptions (s);
    } else if (name == s_screens) {
        readSectionScreens (s);
    } else if (name == s_links) {
        readSectionLinks (s);
    } else if (name == s_aliases) {
        readSectionAliases (s);
    } else {
        throw XConfigRead (s, "unknown section name \"%{1}\"", name);
    }
}

void
Config::readSectionOptions (ConfigReadContext& s) {
    String line;
    while (s.readLine (line)) {
        // check for end of section
        if (line == "end") {
            return;
        }

        // parse argument:  `nameAndArgs = [values][;[values]]'
        //   nameAndArgs  := <name>[(arg[,...])]
        //   values       := valueAndArgs[,valueAndArgs]...
        //   valueAndArgs := <value>[(arg[,...])]
        String::size_type i = 0;
        String name, value;
        ConfigReadContext::ArgList nameArgs, valueArgs;
        s.parseNameWithArgs ("name", line, "=", i, name, nameArgs);
        ++i;
        s.parseNameWithArgs ("value", line, ",;\n", i, value, valueArgs);

        bool handled = true;
        if (name == "address") {
            try {
                m_synergyAddress = NetworkAddress (value, kDefaultPort);
                m_synergyAddress.resolve ();
            } catch (XSocketAddress& e) {
                throw XConfigRead (
                    s, String ("invalid address argument ") + e.what ());
            }
        } else if (name == "heartbeat") {
            addOption ("", kOptionHeartbeat, s.parseInt (value));
        } else if (name == "switchCorners") {
            addOption ("", kOptionScreenSwitchCorners, s.parseCorners (value));
        } else if (name == "switchCornerSize") {
            addOption ("", kOptionScreenSwitchCornerSize, s.parseInt (value));
        } else if (name == "switchDelay") {
            addOption ("", kOptionScreenSwitchDelay, s.parseInt (value));
        } else if (name == "switchDoubleTap") {
            addOption ("", kOptionScreenSwitchTwoTap, s.parseInt (value));
        } else if (name == "switchNeedsShift") {
            addOption (
                "", kOptionScreenSwitchNeedsShift, s.parseBoolean (value));
        } else if (name == "switchNeedsControl") {
            addOption (
                "", kOptionScreenSwitchNeedsControl, s.parseBoolean (value));
        } else if (name == "switchNeedsAlt") {
            addOption ("", kOptionScreenSwitchNeedsAlt, s.parseBoolean (value));
        } else if (name == "screenSaverSync") {
            addOption ("", kOptionScreenSaverSync, s.parseBoolean (value));
        } else if (name == "relativeMouseMoves") {
            addOption ("", kOptionRelativeMouseMoves, s.parseBoolean (value));
        } else if (name == "win32KeepForeground") {
            addOption ("", kOptionWin32KeepForeground, s.parseBoolean (value));
        } else if (name == "clipboardSharing") {
            addOption ("", kOptionClipboardSharing, s.parseBoolean (value));
        }

        else {
            handled = false;
        }

        if (handled) {
            // make sure handled options aren't followed by more values
            if (i < line.size () && (line[i] == ',' || line[i] == ';')) {
                throw XConfigRead (s, "to many arguments to %s", name.c_str ());
            }
        } else {
            // make filter rule
            InputFilter::Rule rule (parseCondition (s, name, nameArgs));

            // save first action (if any)
            if (!value.empty () || line[i] != ';') {
                parseAction (s, value, valueArgs, rule, true);
            }

            // get remaining activate actions
            while (i < line.length () && line[i] != ';') {
                ++i;
                s.parseNameWithArgs (
                    "value", line, ",;\n", i, value, valueArgs);
                parseAction (s, value, valueArgs, rule, true);
            }

            // get deactivate actions
            if (i < line.length () && line[i] == ';') {
                // allow trailing ';'
                i = line.find_first_not_of (" \t", i + 1);
                if (i == String::npos) {
                    i = line.length ();
                } else {
                    --i;
                }

                // get actions
                while (i < line.length ()) {
                    ++i;
                    s.parseNameWithArgs (
                        "value", line, ",\n", i, value, valueArgs);
                    parseAction (s, value, valueArgs, rule, false);
                }
            }

            // add rule
            m_inputFilter.addFilterRule (rule);
        }
    }
    throw XConfigRead (s, "unexpected end of options section");
}

void
Config::readSectionScreens (ConfigReadContext& s) {
    String line;
    String screen;
    while (s.readLine (line)) {
        // check for end of section
        if (line == "end") {
            return;
        }

        // see if it's the next screen
        if (line[line.size () - 1] == ':') {
            // strip :
            screen = line.substr (0, line.size () - 1);

            // verify validity of screen name
            if (!isValidScreenName (screen)) {
                throw XConfigRead (s, "invalid screen name \"%{1}\"", screen);
            }

            // add the screen to the configuration
            if (!addScreen (screen)) {
                throw XConfigRead (s, "duplicate screen name \"%{1}\"", screen);
            }
        } else if (screen.empty ()) {
            throw XConfigRead (s, "argument before first screen");
        } else {
            // parse argument:  `<name>=<value>'
            String::size_type i = line.find_first_of (" \t=");
            if (i == 0) {
                throw XConfigRead (s, "missing argument name");
            }
            if (i == String::npos) {
                throw XConfigRead (s, "missing =");
            }
            String name = line.substr (0, i);
            i           = line.find_first_not_of (" \t", i);
            if (i == String::npos || line[i] != '=') {
                throw XConfigRead (s, "missing =");
            }
            i = line.find_first_not_of (" \t", i + 1);
            String value;
            if (i != String::npos) {
                value = line.substr (i);
            }

            // handle argument
            if (name == "halfDuplexCapsLock") {
                addOption (
                    screen, kOptionHalfDuplexCapsLock, s.parseBoolean (value));
            } else if (name == "halfDuplexNumLock") {
                addOption (
                    screen, kOptionHalfDuplexNumLock, s.parseBoolean (value));
            } else if (name == "halfDuplexScrollLock") {
                addOption (screen,
                           kOptionHalfDuplexScrollLock,
                           s.parseBoolean (value));
            } else if (name == "shift") {
                addOption (screen,
                           kOptionModifierMapForShift,
                           s.parseModifierKey (value));
            } else if (name == "ctrl") {
                addOption (screen,
                           kOptionModifierMapForControl,
                           s.parseModifierKey (value));
            } else if (name == "alt") {
                addOption (screen,
                           kOptionModifierMapForAlt,
                           s.parseModifierKey (value));
            } else if (name == "altgr") {
                addOption (screen,
                           kOptionModifierMapForAltGr,
                           s.parseModifierKey (value));
            } else if (name == "meta") {
                addOption (screen,
                           kOptionModifierMapForMeta,
                           s.parseModifierKey (value));
            } else if (name == "super") {
                addOption (screen,
                           kOptionModifierMapForSuper,
                           s.parseModifierKey (value));
            } else if (name == "xtestIsXineramaUnaware") {
                addOption (screen,
                           kOptionXTestXineramaUnaware,
                           s.parseBoolean (value));
            } else if (name == "switchCorners") {
                addOption (
                    screen, kOptionScreenSwitchCorners, s.parseCorners (value));
            } else if (name == "switchCornerSize") {
                addOption (
                    screen, kOptionScreenSwitchCornerSize, s.parseInt (value));
            } else if (name == "preserveFocus") {
                addOption (
                    screen, kOptionScreenPreserveFocus, s.parseBoolean (value));
            } else {
                // unknown argument
                throw XConfigRead (s, "unknown argument \"%{1}\"", name);
            }
        }
    }
    throw XConfigRead (s, "unexpected end of screens section");
}

void
Config::readSectionLinks (ConfigReadContext& s) {
    String line;
    String screen;
    while (s.readLine (line)) {
        // check for end of section
        if (line == "end") {
            return;
        }

        // see if it's the next screen
        if (line[line.size () - 1] == ':') {
            // strip :
            screen = line.substr (0, line.size () - 1);

            // verify we know about the screen
            if (!isScreen (screen)) {
                throw XConfigRead (s, "unknown screen name \"%{1}\"", screen);
            }
            if (!isCanonicalName (screen)) {
                throw XConfigRead (s, "cannot use screen name alias here");
            }
        } else if (screen.empty ()) {
            throw XConfigRead (s, "argument before first screen");
        } else {
            // parse argument:  `<name>[(<s0>,<e0>)]=<value>[(<s1>,<e1>)]'
            // the stuff in brackets is optional.  interval values must be
            // in the range [0,100] and start < end.  if not given the
            // interval is taken to be (0,100).
            String::size_type i = 0;
            String side, dstScreen, srcArgString, dstArgString;
            ConfigReadContext::ArgList srcArgs, dstArgs;
            s.parseNameWithArgs ("link", line, "=", i, side, srcArgs);
            ++i;
            s.parseNameWithArgs ("screen", line, "", i, dstScreen, dstArgs);
            Interval srcInterval (s.parseInterval (srcArgs));
            Interval dstInterval (s.parseInterval (dstArgs));

            // handle argument
            EDirection dir;
            if (side == "left") {
                dir = kLeft;
            } else if (side == "right") {
                dir = kRight;
            } else if (side == "up") {
                dir = kTop;
            } else if (side == "down") {
                dir = kBottom;
            } else {
                // unknown argument
                throw XConfigRead (s, "unknown side \"%{1}\" in link", side);
            }
            if (!isScreen (dstScreen)) {
                throw XConfigRead (
                    s, "unknown screen name \"%{1}\"", dstScreen);
            }
            if (!connect (screen,
                          dir,
                          srcInterval.first,
                          srcInterval.second,
                          dstScreen,
                          dstInterval.first,
                          dstInterval.second)) {
                throw XConfigRead (s, "overlapping range");
            }
        }
    }
    throw XConfigRead (s, "unexpected end of links section");
}

void
Config::readSectionAliases (ConfigReadContext& s) {
    String line;
    String screen;
    while (s.readLine (line)) {
        // check for end of section
        if (line == "end") {
            return;
        }

        // see if it's the next screen
        if (line[line.size () - 1] == ':') {
            // strip :
            screen = line.substr (0, line.size () - 1);

            // verify we know about the screen
            if (!isScreen (screen)) {
                throw XConfigRead (s, "unknown screen name \"%{1}\"", screen);
            }
            if (!isCanonicalName (screen)) {
                throw XConfigRead (s, "cannot use screen name alias here");
            }
        } else if (screen.empty ()) {
            throw XConfigRead (s, "argument before first screen");
        } else {
            // verify validity of screen name
            if (!isValidScreenName (line)) {
                throw XConfigRead (s, "invalid screen alias \"%{1}\"", line);
            }

            // add alias
            if (!addAlias (screen, line)) {
                throw XConfigRead (s, "alias \"%{1}\" is already used", line);
            }
        }
    }
    throw XConfigRead (s, "unexpected end of aliases section");
}


InputFilter::Condition*
Config::parseCondition (ConfigReadContext& s, const String& name,
                        const std::vector<String>& args) {
    if (name == "keystroke") {
        if (args.size () != 1) {
            throw XConfigRead (
                s, "syntax for condition: keystroke(modifiers+key)");
        }

        IPlatformScreen::KeyInfo* keyInfo = s.parseKeystroke (args[0]);

        return new InputFilter::KeystrokeCondition (m_events, keyInfo);
    }

    if (name == "mousebutton") {
        if (args.size () != 1) {
            throw XConfigRead (
                s, "syntax for condition: mousebutton(modifiers+button)");
        }

        IPlatformScreen::ButtonInfo* mouseInfo = s.parseMouse (args[0]);

        return new InputFilter::MouseButtonCondition (m_events, mouseInfo);
    }

    if (name == "connect") {
        if (args.size () != 1) {
            throw XConfigRead (s, "syntax for condition: connect([screen])");
        }

        String screen = args[0];
        if (isScreen (screen)) {
            screen = getCanonicalName (screen);
        } else if (!screen.empty ()) {
            throw XConfigRead (
                s, "unknown screen name \"%{1}\" in connect", screen);
        }

        return new InputFilter::ScreenConnectedCondition (m_events, screen);
    }

    throw XConfigRead (s, "unknown argument \"%{1}\"", name);
}

void
Config::parseAction (ConfigReadContext& s, const String& name,
                     const std::vector<String>& args, InputFilter::Rule& rule,
                     bool activate) {
    InputFilter::Action* action;

    if (name == "keystroke" || name == "keyDown" || name == "keyUp") {
        if (args.size () < 1 || args.size () > 2) {
            throw XConfigRead (
                s, "syntax for action: keystroke(modifiers+key[,screens])");
        }

        IPlatformScreen::KeyInfo* keyInfo;
        if (args.size () == 1) {
            keyInfo = s.parseKeystroke (args[0]);
        } else {
            std::set<String> screens;
            parseScreens (s, args[1], screens);
            keyInfo = s.parseKeystroke (args[0], screens);
        }

        if (name == "keystroke") {
            IPlatformScreen::KeyInfo* keyInfo2 =
                IKeyState::KeyInfo::alloc (*keyInfo);
            action =
                new InputFilter::KeystrokeAction (m_events, keyInfo2, true);
            rule.adoptAction (action, true);
            action =
                new InputFilter::KeystrokeAction (m_events, keyInfo, false);
            activate = false;
        } else if (name == "keyDown") {
            action = new InputFilter::KeystrokeAction (m_events, keyInfo, true);
        } else {
            action =
                new InputFilter::KeystrokeAction (m_events, keyInfo, false);
        }
    }

    else if (name == "mousebutton" || name == "mouseDown" ||
             name == "mouseUp") {
        if (args.size () != 1) {
            throw XConfigRead (
                s, "syntax for action: mousebutton(modifiers+button)");
        }

        IPlatformScreen::ButtonInfo* mouseInfo = s.parseMouse (args[0]);

        if (name == "mousebutton") {
            IPlatformScreen::ButtonInfo* mouseInfo2 =
                IPlatformScreen::ButtonInfo::alloc (*mouseInfo);
            action =
                new InputFilter::MouseButtonAction (m_events, mouseInfo2, true);
            rule.adoptAction (action, true);
            action =
                new InputFilter::MouseButtonAction (m_events, mouseInfo, false);
            activate = false;
        } else if (name == "mouseDown") {
            action =
                new InputFilter::MouseButtonAction (m_events, mouseInfo, true);
        } else {
            action =
                new InputFilter::MouseButtonAction (m_events, mouseInfo, false);
        }
    }

    /* XXX -- not supported
        else if (name == "modifier") {
            if (args.size() != 1) {
                throw XConfigRead(s, "syntax for action: modifier(modifiers)");
            }

            KeyModifierMask mask = s.parseModifier(args[0]);

            action = new InputFilter::ModifierAction(mask, ~mask);
        }
    */

    else if (name == "switchToScreen") {
        if (args.size () != 1) {
            throw XConfigRead (s, "syntax for action: switchToScreen(name)");
        }

        String screen = args[0];
        if (isScreen (screen)) {
            screen = getCanonicalName (screen);
        } else if (!screen.empty ()) {
            throw XConfigRead (s, "unknown screen name in switchToScreen");
        }

        action = new InputFilter::SwitchToScreenAction (m_events, screen);
    }

    else if (name == "switchInDirection") {
        if (args.size () != 1) {
            throw XConfigRead (
                s,
                "syntax for action: switchInDirection(<left|right|up|down>)");
        }

        EDirection direction;
        if (args[0] == "left") {
            direction = kLeft;
        } else if (args[0] == "right") {
            direction = kRight;
        } else if (args[0] == "up") {
            direction = kTop;
        } else if (args[0] == "down") {
            direction = kBottom;
        } else {
            throw XConfigRead (
                s, "unknown direction \"%{1}\" in switchToScreen", args[0]);
        }

        action = new InputFilter::SwitchInDirectionAction (m_events, direction);
    }

    else if (name == "lockCursorToScreen") {
        if (args.size () > 1) {
            throw XConfigRead (
                s, "syntax for action: lockCursorToScreen([{off|on|toggle}])");
        }

        InputFilter::LockCursorToScreenAction::Mode mode =
            InputFilter::LockCursorToScreenAction::kToggle;
        if (args.size () == 1) {
            if (args[0] == "off") {
                mode = InputFilter::LockCursorToScreenAction::kOff;
            } else if (args[0] == "on") {
                mode = InputFilter::LockCursorToScreenAction::kOn;
            } else if (args[0] == "toggle") {
                mode = InputFilter::LockCursorToScreenAction::kToggle;
            } else {
                throw XConfigRead (
                    s,
                    "syntax for action: lockCursorToScreen([{off|on|toggle}])");
            }
        }

        if (mode != InputFilter::LockCursorToScreenAction::kOff) {
            m_hasLockToScreenAction = true;
        }

        action = new InputFilter::LockCursorToScreenAction (m_events, mode);
    }

    else if (name == "keyboardBroadcast") {
        if (args.size () > 2) {
            throw XConfigRead (s, "syntax for action: "
                                  "keyboardBroadcast([{off|on|toggle}[,screens]"
                                  "])");
        }

        InputFilter::KeyboardBroadcastAction::Mode mode =
            InputFilter::KeyboardBroadcastAction::kToggle;
        if (args.size () >= 1) {
            if (args[0] == "off") {
                mode = InputFilter::KeyboardBroadcastAction::kOff;
            } else if (args[0] == "on") {
                mode = InputFilter::KeyboardBroadcastAction::kOn;
            } else if (args[0] == "toggle") {
                mode = InputFilter::KeyboardBroadcastAction::kToggle;
            } else {
                throw XConfigRead (s, "syntax for action: "
                                      "keyboardBroadcast([{off|on|toggle}[,"
                                      "screens]])");
            }
        }

        std::set<String> screens;
        if (args.size () >= 2) {
            parseScreens (s, args[1], screens);
        }

        action =
            new InputFilter::KeyboardBroadcastAction (m_events, mode, screens);
    }

    else {
        throw XConfigRead (s, "unknown action argument \"%{1}\"", name);
    }

    rule.adoptAction (action, activate);
}

void
Config::parseScreens (ConfigReadContext& c, const String& s,
                      std::set<String>& screens) const {
    screens.clear ();

    String::size_type i = 0;
    while (i < s.size ()) {
        // find end of next screen name
        String::size_type j = s.find (':', i);
        if (j == String::npos) {
            j = s.size ();
        }

        // extract name
        String rawName;
        i = s.find_first_not_of (" \t", i);
        if (i < j) {
            rawName = s.substr (i, s.find_last_not_of (" \t", j - 1) - i + 1);
        }

        // add name
        if (rawName == "*") {
            screens.insert ("*");
        } else if (!rawName.empty ()) {
            String name = getCanonicalName (rawName);
            if (name.empty ()) {
                throw XConfigRead (c, "unknown screen name \"%{1}\"", rawName);
            }
            screens.insert (name);
        }

        // next
        i = j + 1;
    }
}

const char*
Config::getOptionName (OptionID id) {
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
    if (id == kOptionScreenSaverSync) {
        return "screenSaverSync";
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
    if (id == kOptionClipboardSharing) {
        return "clipboardSharing";
    }
    return NULL;
}

String
Config::getOptionValue (OptionID id, OptionValue value) {
    if (id == kOptionHalfDuplexCapsLock || id == kOptionHalfDuplexNumLock ||
        id == kOptionHalfDuplexScrollLock ||
        id == kOptionScreenSwitchNeedsShift ||
        id == kOptionScreenSwitchNeedsControl ||
        id == kOptionScreenSwitchNeedsAlt || id == kOptionScreenSaverSync ||
        id == kOptionXTestXineramaUnaware || id == kOptionRelativeMouseMoves ||
        id == kOptionWin32KeepForeground || id == kOptionScreenPreserveFocus ||
        id == kOptionClipboardSharing) {
        return (value != 0) ? "true" : "false";
    }
    if (id == kOptionModifierMapForShift ||
        id == kOptionModifierMapForControl || id == kOptionModifierMapForAlt ||
        id == kOptionModifierMapForAltGr || id == kOptionModifierMapForMeta ||
        id == kOptionModifierMapForSuper) {
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
    if (id == kOptionHeartbeat || id == kOptionScreenSwitchCornerSize ||
        id == kOptionScreenSwitchDelay || id == kOptionScreenSwitchTwoTap) {
        return synergy::string::sprintf ("%d", value);
    }
    if (id == kOptionScreenSwitchCorners) {
        std::string result ("none");
        if ((value & kTopLeftMask) != 0) {
            result += " +top-left";
        }
        if ((value & kTopRightMask) != 0) {
            result += " +top-right";
        }
        if ((value & kBottomLeftMask) != 0) {
            result += " +bottom-left";
        }
        if ((value & kBottomRightMask) != 0) {
            result += " +bottom-right";
        }
        return result;
    }

    return "";
}


//
// Config::Name
//

Config::Name::Name (Config* config, const String& name)
    : m_config (config), m_name (config->getCanonicalName (name)) {
    // do nothing
}

bool
Config::Name::operator== (const String& name) const {
    String canonical = m_config->getCanonicalName (name);
    return CaselessCmp::equal (canonical, m_name);
}


//
// Config::CellEdge
//

Config::CellEdge::CellEdge (EDirection side, float position) {
    init ("", side, Interval (position, position));
}

Config::CellEdge::CellEdge (EDirection side, const Interval& interval) {
    assert (interval.first >= 0.0f);
    assert (interval.second <= 1.0f);
    assert (interval.first < interval.second);

    init ("", side, interval);
}

Config::CellEdge::CellEdge (const String& name, EDirection side,
                            const Interval& interval) {
    assert (interval.first >= 0.0f);
    assert (interval.second <= 1.0f);
    assert (interval.first < interval.second);

    init (name, side, interval);
}

Config::CellEdge::~CellEdge () {
    // do nothing
}

void
Config::CellEdge::init (const String& name, EDirection side,
                        const Interval& interval) {
    assert (side != kNoDirection);

    m_name     = name;
    m_side     = side;
    m_interval = interval;
}

Config::Interval
Config::CellEdge::getInterval () const {
    return m_interval;
}

void
Config::CellEdge::setName (const String& newName) {
    m_name = newName;
}

String
Config::CellEdge::getName () const {
    return m_name;
}

EDirection
Config::CellEdge::getSide () const {
    return m_side;
}

bool
Config::CellEdge::overlaps (const CellEdge& edge) const {
    const Interval& x = m_interval;
    const Interval& y = edge.m_interval;
    if (m_side != edge.m_side) {
        return false;
    }
    return (x.first >= y.first && x.first < y.second) ||
           (x.second > y.first && x.second <= y.second) ||
           (y.first >= x.first && y.first < x.second) ||
           (y.second > x.first && y.second <= x.second);
}

bool
Config::CellEdge::isInside (float x) const {
    return (x >= m_interval.first && x < m_interval.second);
}

float
Config::CellEdge::transform (float x) const {
    return (x - m_interval.first) / (m_interval.second - m_interval.first);
}


float
Config::CellEdge::inverseTransform (float x) const {
    return x * (m_interval.second - m_interval.first) + m_interval.first;
}

bool
Config::CellEdge::operator< (const CellEdge& o) const {
    if (static_cast<int> (m_side) < static_cast<int> (o.m_side)) {
        return true;
    } else if (static_cast<int> (m_side) > static_cast<int> (o.m_side)) {
        return false;
    }

    return (m_interval.first < o.m_interval.first);
}

bool
Config::CellEdge::operator== (const CellEdge& x) const {
    return (m_side == x.m_side && m_interval == x.m_interval);
}

bool
Config::CellEdge::operator!= (const CellEdge& x) const {
    return !operator== (x);
}


//
// Config::Cell
//

bool
Config::Cell::add (const CellEdge& src, const CellEdge& dst) {
    // cannot add an edge that overlaps other existing edges but we
    // can exactly replace an edge.
    if (!hasEdge (src) && overlaps (src)) {
        return false;
    }

    m_neighbors.erase (src);
    m_neighbors.insert (std::make_pair (src, dst));
    return true;
}

void
Config::Cell::remove (EDirection side) {
    for (EdgeLinks::iterator j = m_neighbors.begin ();
         j != m_neighbors.end ();) {
        if (j->first.getSide () == side) {
            m_neighbors.erase (j++);
        } else {
            ++j;
        }
    }
}

void
Config::Cell::remove (EDirection side, float position) {
    for (EdgeLinks::iterator j = m_neighbors.begin (); j != m_neighbors.end ();
         ++j) {
        if (j->first.getSide () == side && j->first.isInside (position)) {
            m_neighbors.erase (j);
            break;
        }
    }
}
void
Config::Cell::remove (const Name& name) {
    for (EdgeLinks::iterator j = m_neighbors.begin ();
         j != m_neighbors.end ();) {
        if (name == j->second.getName ()) {
            m_neighbors.erase (j++);
        } else {
            ++j;
        }
    }
}

void
Config::Cell::rename (const Name& oldName, const String& newName) {
    for (EdgeLinks::iterator j = m_neighbors.begin (); j != m_neighbors.end ();
         ++j) {
        if (oldName == j->second.getName ()) {
            j->second.setName (newName);
        }
    }
}

bool
Config::Cell::hasEdge (const CellEdge& edge) const {
    EdgeLinks::const_iterator i = m_neighbors.find (edge);
    return (i != m_neighbors.end () && i->first == edge);
}

bool
Config::Cell::overlaps (const CellEdge& edge) const {
    EdgeLinks::const_iterator i = m_neighbors.upper_bound (edge);
    if (i != m_neighbors.end () && i->first.overlaps (edge)) {
        return true;
    }
    if (i != m_neighbors.begin () && (--i)->first.overlaps (edge)) {
        return true;
    }
    return false;
}

bool
Config::Cell::getLink (EDirection side, float position, const CellEdge*& src,
                       const CellEdge*& dst) const {
    CellEdge edge (side, position);
    EdgeLinks::const_iterator i = m_neighbors.upper_bound (edge);
    if (i == m_neighbors.begin ()) {
        return false;
    }
    --i;
    if (i->first.getSide () == side && i->first.isInside (position)) {
        src = &i->first;
        dst = &i->second;
        return true;
    }
    return false;
}

bool
Config::Cell::operator== (const Cell& x) const {
    // compare options
    if (m_options != x.m_options) {
        return false;
    }

    // compare links
    if (m_neighbors.size () != x.m_neighbors.size ()) {
        return false;
    }
    for (EdgeLinks::const_iterator index1 = m_neighbors.begin (),
                                   index2 = x.m_neighbors.begin ();
         index1 != m_neighbors.end ();
         ++index1, ++index2) {
        if (index1->first != index2->first) {
            return false;
        }
        if (index1->second != index2->second) {
            return false;
        }

        // operator== doesn't compare names.  only compare destination
        // names.
        if (!CaselessCmp::equal (index1->second.getName (),
                                 index2->second.getName ())) {
            return false;
        }
    }
    return true;
}

bool
Config::Cell::operator!= (const Cell& x) const {
    return !operator== (x);
}

Config::Cell::const_iterator
Config::Cell::begin () const {
    return m_neighbors.begin ();
}

Config::Cell::const_iterator
Config::Cell::end () const {
    return m_neighbors.end ();
}


//
// Config I/O
//

std::istream&
operator>> (std::istream& s, Config& config) {
    ConfigReadContext context (s);
    config.read (context);
    return s;
}

std::ostream&
operator<< (std::ostream& s, const Config& config) {
    // screens section
    s << "section: screens" << std::endl;
    for (Config::const_iterator screen = config.begin ();
         screen != config.end ();
         ++screen) {
        s << "\t" << screen->c_str () << ":" << std::endl;
        const Config::ScreenOptions* options = config.getOptions (*screen);
        if (options != NULL && options->size () > 0) {
            for (Config::ScreenOptions::const_iterator option =
                     options->begin ();
                 option != options->end ();
                 ++option) {
                const char* name = Config::getOptionName (option->first);
                String value =
                    Config::getOptionValue (option->first, option->second);
                if (name != NULL && !value.empty ()) {
                    s << "\t\t" << name << " = " << value << std::endl;
                }
            }
        }
    }
    s << "end" << std::endl;

    // links section
    String neighbor;
    s << "section: links" << std::endl;
    for (Config::const_iterator screen = config.begin ();
         screen != config.end ();
         ++screen) {
        s << "\t" << screen->c_str () << ":" << std::endl;

        for (Config::link_const_iterator link = config.beginNeighbor (*screen),
                                         nend = config.endNeighbor (*screen);
             link != nend;
             ++link) {
            s << "\t\t" << Config::dirName (link->first.getSide ())
              << Config::formatInterval (link->first.getInterval ()) << " = "
              << link->second.getName ().c_str ()
              << Config::formatInterval (link->second.getInterval ())
              << std::endl;
        }
    }
    s << "end" << std::endl;

    // aliases section (if there are any)
    if (config.m_map.size () != config.m_nameToCanonicalName.size ()) {
        // map canonical to alias
        typedef std::multimap<String, String, CaselessCmp> CMNameMap;
        CMNameMap aliases;
        for (Config::NameMap::const_iterator index =
                 config.m_nameToCanonicalName.begin ();
             index != config.m_nameToCanonicalName.end ();
             ++index) {
            if (index->first != index->second) {
                aliases.insert (std::make_pair (index->second, index->first));
            }
        }

        // dump it
        String screen;
        s << "section: aliases" << std::endl;
        for (CMNameMap::const_iterator index = aliases.begin ();
             index != aliases.end ();
             ++index) {
            if (index->first != screen) {
                screen = index->first;
                s << "\t" << screen.c_str () << ":" << std::endl;
            }
            s << "\t\t" << index->second.c_str () << std::endl;
        }
        s << "end" << std::endl;
    }

    // options section
    s << "section: options" << std::endl;
    const Config::ScreenOptions* options = config.getOptions ("");
    if (options != NULL && options->size () > 0) {
        for (Config::ScreenOptions::const_iterator option = options->begin ();
             option != options->end ();
             ++option) {
            const char* name = Config::getOptionName (option->first);
            String value =
                Config::getOptionValue (option->first, option->second);
            if (name != NULL && !value.empty ()) {
                s << "\t" << name << " = " << value << std::endl;
            }
        }
    }
    if (config.m_synergyAddress.isValid ()) {
        s << "\taddress = " << config.m_synergyAddress.getHostname ().c_str ()
          << std::endl;
    }
    s << config.m_inputFilter.format ("\t");
    s << "end" << std::endl;

    return s;
}


//
// ConfigReadContext
//

ConfigReadContext::ConfigReadContext (std::istream& s, SInt32 firstLine)
    : m_stream (s), m_line (firstLine - 1) {
    // do nothing
}

ConfigReadContext::~ConfigReadContext () {
    // do nothing
}

bool
ConfigReadContext::readLine (String& line) {
    ++m_line;
    while (std::getline (m_stream, line)) {
        // strip leading whitespace
        String::size_type i = line.find_first_not_of (" \t");
        if (i != String::npos) {
            line.erase (0, i);
        }

        // strip comments and then trailing whitespace
        i = line.find ('#');
        if (i != String::npos) {
            line.erase (i);
        }
        i = line.find_last_not_of (" \r\t");
        if (i != String::npos) {
            line.erase (i + 1);
        }

        // return non empty line
        if (!line.empty ()) {
            // make sure there are no invalid characters
            for (i = 0; i < line.length (); ++i) {
                if (!isgraph (line[i]) && line[i] != ' ' && line[i] != '\t') {
                    throw XConfigRead (
                        *this,
                        "invalid character %{1}",
                        synergy::string::sprintf ("%#2x", line[i]));
                }
            }

            return true;
        }

        // next line
        ++m_line;
    }
    return false;
}

UInt32
ConfigReadContext::getLineNumber () const {
    return m_line;
}

bool ConfigReadContext::operator! () const {
    return !m_stream;
}

OptionValue
ConfigReadContext::parseBoolean (const String& arg) const {
    if (CaselessCmp::equal (arg, "true")) {
        return static_cast<OptionValue> (true);
    }
    if (CaselessCmp::equal (arg, "false")) {
        return static_cast<OptionValue> (false);
    }
    throw XConfigRead (*this, "invalid boolean argument \"%{1}\"", arg);
}

OptionValue
ConfigReadContext::parseInt (const String& arg) const {
    const char* s = arg.c_str ();
    char* end;
    long tmp = strtol (s, &end, 10);
    if (*end != '\0') {
        // invalid characters
        throw XConfigRead (*this, "invalid integer argument \"%{1}\"", arg);
    }
    OptionValue value = static_cast<OptionValue> (tmp);
    if (value != tmp) {
        // out of range
        throw XConfigRead (
            *this, "integer argument \"%{1}\" out of range", arg);
    }
    return value;
}

OptionValue
ConfigReadContext::parseModifierKey (const String& arg) const {
    if (CaselessCmp::equal (arg, "shift")) {
        return static_cast<OptionValue> (kKeyModifierIDShift);
    }
    if (CaselessCmp::equal (arg, "ctrl")) {
        return static_cast<OptionValue> (kKeyModifierIDControl);
    }
    if (CaselessCmp::equal (arg, "alt")) {
        return static_cast<OptionValue> (kKeyModifierIDAlt);
    }
    if (CaselessCmp::equal (arg, "altgr")) {
        return static_cast<OptionValue> (kKeyModifierIDAltGr);
    }
    if (CaselessCmp::equal (arg, "meta")) {
        return static_cast<OptionValue> (kKeyModifierIDMeta);
    }
    if (CaselessCmp::equal (arg, "super")) {
        return static_cast<OptionValue> (kKeyModifierIDSuper);
    }
    if (CaselessCmp::equal (arg, "none")) {
        return static_cast<OptionValue> (kKeyModifierIDNull);
    }
    throw XConfigRead (*this, "invalid argument \"%{1}\"", arg);
}

OptionValue
ConfigReadContext::parseCorner (const String& arg) const {
    if (CaselessCmp::equal (arg, "left")) {
        return kTopLeftMask | kBottomLeftMask;
    } else if (CaselessCmp::equal (arg, "right")) {
        return kTopRightMask | kBottomRightMask;
    } else if (CaselessCmp::equal (arg, "top")) {
        return kTopLeftMask | kTopRightMask;
    } else if (CaselessCmp::equal (arg, "bottom")) {
        return kBottomLeftMask | kBottomRightMask;
    } else if (CaselessCmp::equal (arg, "top-left")) {
        return kTopLeftMask;
    } else if (CaselessCmp::equal (arg, "top-right")) {
        return kTopRightMask;
    } else if (CaselessCmp::equal (arg, "bottom-left")) {
        return kBottomLeftMask;
    } else if (CaselessCmp::equal (arg, "bottom-right")) {
        return kBottomRightMask;
    } else if (CaselessCmp::equal (arg, "none")) {
        return kNoCornerMask;
    } else if (CaselessCmp::equal (arg, "all")) {
        return kAllCornersMask;
    }
    throw XConfigRead (*this, "invalid argument \"%{1}\"", arg);
}

OptionValue
ConfigReadContext::parseCorners (const String& args) const {
    // find first token
    String::size_type i = args.find_first_not_of (" \t", 0);
    if (i == String::npos) {
        throw XConfigRead (*this, "missing corner argument");
    }
    String::size_type j = args.find_first_of (" \t", i);

    // parse first corner token
    OptionValue corners = parseCorner (args.substr (i, j - i));

    // get +/-
    i = args.find_first_not_of (" \t", j);
    while (i != String::npos) {
        // parse +/-
        bool add;
        if (args[i] == '-') {
            add = false;
        } else if (args[i] == '+') {
            add = true;
        } else {
            throw XConfigRead (*this,
                               "invalid corner operator \"%{1}\"",
                               String (args.c_str () + i, 1));
        }

        // get next corner token
        i = args.find_first_not_of (" \t", i + 1);
        j = args.find_first_of (" \t", i);
        if (i == String::npos) {
            throw XConfigRead (*this, "missing corner argument");
        }

        // parse next corner token
        if (add) {
            corners |= parseCorner (args.substr (i, j - i));
        } else {
            corners &= ~parseCorner (args.substr (i, j - i));
        }
        i = args.find_first_not_of (" \t", j);
    }

    return corners;
}

Config::Interval
ConfigReadContext::parseInterval (const ArgList& args) const {
    if (args.size () == 0) {
        return Config::Interval (0.0f, 1.0f);
    }
    if (args.size () != 2 || args[0].empty () || args[1].empty ()) {
        throw XConfigRead (
            *this, "invalid interval \"%{1}\"", concatArgs (args));
    }

    char* end;
    double startValue = strtod (args[0].c_str (), &end);
    if (end[0] != '\0') {
        throw XConfigRead (
            *this, "invalid interval \"%{1}\"", concatArgs (args));
    }
    double endValue = strtod (args[1].c_str (), &end);
    if (end[0] != '\0') {
        throw XConfigRead (
            *this, "invalid interval \"%{1}\"", concatArgs (args));
    }

    if (startValue < 0 || startValue > 100 || endValue < 0 || endValue > 100 ||
        startValue >= endValue) {
        throw XConfigRead (
            *this, "invalid interval \"%{1}\"", concatArgs (args));
    }

    return Config::Interval (startValue / 100.0f, endValue / 100.0f);
}

void
ConfigReadContext::parseNameWithArgs (const String& type, const String& line,
                                      const String& delim,
                                      String::size_type& index, String& name,
                                      ArgList& args) const {
    // skip leading whitespace
    String::size_type i = line.find_first_not_of (" \t", index);
    if (i == String::npos) {
        throw XConfigRead (*this, String ("missing ") + type);
    }

    // find end of name
    String::size_type j = line.find_first_of (" \t(" + delim, i);
    if (j == String::npos) {
        j = line.length ();
    }

    // save name
    name = line.substr (i, j - i);
    args.clear ();

    // is it okay to not find a delimiter?
    bool needDelim = (!delim.empty () && delim.find ('\n') == String::npos);

    // skip whitespace
    i = line.find_first_not_of (" \t", j);
    if (i == String::npos && needDelim) {
        // expected delimiter but didn't find it
        throw XConfigRead (*this, String ("missing ") + delim[0]);
    }
    if (i == String::npos) {
        // no arguments
        index = line.length ();
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
    j = line.find_first_of (",)", i);
    while (j != String::npos) {
        // extract arg
        String arg (line.substr (i, j - i));
        i = j;

        // trim whitespace
        j = arg.find_first_not_of (" \t");
        if (j != String::npos) {
            arg.erase (0, j);
        }
        j = arg.find_last_not_of (" \t");
        if (j != String::npos) {
            arg.erase (j + 1);
        }

        // save arg
        args.push_back (arg);

        // exit loop at end of arguments
        if (line[i] == ')') {
            break;
        }

        // eat ','
        ++i;

        // next
        j = line.find_first_of (",)", i);
    }

    // verify ')'
    if (j == String::npos) {
        // expected )
        throw XConfigRead (*this, "missing )");
    }

    // eat ')'
    ++i;

    // skip whitespace
    j = line.find_first_not_of (" \t", i);
    if (j == String::npos && needDelim) {
        // expected delimiter but didn't find it
        throw XConfigRead (*this, String ("missing ") + delim[0]);
    }

    // verify delimiter
    if (needDelim && delim.find (line[j]) == String::npos) {
        throw XConfigRead (*this, String ("expected ") + delim[0]);
    }

    if (j == String::npos) {
        j = line.length ();
    }

    index = j;
    return;
}

IPlatformScreen::KeyInfo*
ConfigReadContext::parseKeystroke (const String& keystroke) const {
    return parseKeystroke (keystroke, std::set<String> ());
}

IPlatformScreen::KeyInfo*
ConfigReadContext::parseKeystroke (const String& keystroke,
                                   const std::set<String>& screens) const {
    String s = keystroke;

    KeyModifierMask mask;
    if (!synergy::KeyMap::parseModifiers (s, mask)) {
        throw XConfigRead (*this, "unable to parse key modifiers");
    }

    KeyID key;
    if (!synergy::KeyMap::parseKey (s, key)) {
        throw XConfigRead (*this, "unable to parse key");
    }

    if (key == kKeyNone && mask == 0) {
        throw XConfigRead (*this, "missing key and/or modifiers in keystroke");
    }

    return IPlatformScreen::KeyInfo::alloc (key, mask, 0, 0, screens);
}

IPlatformScreen::ButtonInfo*
ConfigReadContext::parseMouse (const String& mouse) const {
    String s = mouse;

    KeyModifierMask mask;
    if (!synergy::KeyMap::parseModifiers (s, mask)) {
        throw XConfigRead (*this, "unable to parse button modifiers");
    }

    char* end;
    ButtonID button = (ButtonID) strtol (s.c_str (), &end, 10);
    if (*end != '\0') {
        throw XConfigRead (*this, "unable to parse button");
    }
    if (s.empty () || button <= 0) {
        throw XConfigRead (*this, "invalid button");
    }

    return IPlatformScreen::ButtonInfo::alloc (button, mask);
}

KeyModifierMask
ConfigReadContext::parseModifier (const String& modifiers) const {
    String s = modifiers;

    KeyModifierMask mask;
    if (!synergy::KeyMap::parseModifiers (s, mask)) {
        throw XConfigRead (*this, "unable to parse modifiers");
    }

    if (mask == 0) {
        throw XConfigRead (*this, "no modifiers specified");
    }

    return mask;
}

String
ConfigReadContext::concatArgs (const ArgList& args) {
    String s ("(");
    for (size_t i = 0; i < args.size (); ++i) {
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

XConfigRead::XConfigRead (const ConfigReadContext& context, const String& error)
    : m_error (synergy::string::sprintf (
          "line %d: %s", context.getLineNumber (), error.c_str ())) {
    // do nothing
}

XConfigRead::XConfigRead (const ConfigReadContext& context,
                          const char* errorFmt, const String& arg)
    : m_error (
          synergy::string::sprintf ("line %d: ", context.getLineNumber ()) +
          synergy::string::format (errorFmt, arg.c_str ())) {
    // do nothing
}

XConfigRead::~XConfigRead () _NOEXCEPT {
    // do nothing
}

String
XConfigRead::getWhat () const throw () {
    return format ("XConfigRead", "read error: %{1}", m_error.c_str ());
}
