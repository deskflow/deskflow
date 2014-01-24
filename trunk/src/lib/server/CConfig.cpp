/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CConfig.h"
#include "CServer.h"
#include "CKeyMap.h"
#include "KeyTypes.h"
#include "XSocket.h"
#include "stdistream.h"
#include "stdostream.h"
#include <cstdlib>
#include "IEventQueue.h"

//
// CConfig
//

CConfig::CConfig(IEventQueue* events) :
	m_events(events),
	m_hasLockToScreenAction(false),
	m_inputFilter(events)
{
	// do nothing
}

CConfig::~CConfig()
{
	// do nothing
}

bool
CConfig::addScreen(const CString& name)
{
	// alias name must not exist
	if (m_nameToCanonicalName.find(name) != m_nameToCanonicalName.end()) {
		return false;
	}

	// add cell
	m_map.insert(std::make_pair(name, CCell()));

	// add name
	m_nameToCanonicalName.insert(std::make_pair(name, name));

	return true;
}

bool
CConfig::renameScreen(const CString& oldName,
							const CString& newName)
{
	// get canonical name and find cell
	CString oldCanonical = getCanonicalName(oldName);
	CCellMap::iterator index = m_map.find(oldCanonical);
	if (index == m_map.end()) {
		return false;
	}

	// accept if names are equal but replace with new name to maintain
	// case.  otherwise, the new name must not exist.
	if (!CStringUtil::CaselessCmp::equal(oldName, newName) &&
		m_nameToCanonicalName.find(newName) != m_nameToCanonicalName.end()) {
		return false;
	}

	// update cell
	CCell tmpCell = index->second;
	m_map.erase(index);
	m_map.insert(std::make_pair(newName, tmpCell));

	// update name
	m_nameToCanonicalName.erase(oldCanonical);
	m_nameToCanonicalName.insert(std::make_pair(newName, newName));

	// update connections
	CName oldNameObj(this, oldName);
	for (index = m_map.begin(); index != m_map.end(); ++index) {
		index->second.rename(oldNameObj, newName);
	}

	// update alias targets
	if (CStringUtil::CaselessCmp::equal(oldName, oldCanonical)) {
		for (CNameMap::iterator iter = m_nameToCanonicalName.begin();
							iter != m_nameToCanonicalName.end(); ++iter) {
			if (CStringUtil::CaselessCmp::equal(
							iter->second, oldCanonical)) {
				iter->second = newName;
			}
		}
	}

	return true;
}

void
CConfig::removeScreen(const CString& name)
{
	// get canonical name and find cell
	CString canonical = getCanonicalName(name);
	CCellMap::iterator index = m_map.find(canonical);
	if (index == m_map.end()) {
		return;
	}

	// remove from map
	m_map.erase(index);

	// disconnect
	CName nameObj(this, name);
	for (index = m_map.begin(); index != m_map.end(); ++index) {
		index->second.remove(nameObj);
	}

	// remove aliases (and canonical name)
	for (CNameMap::iterator iter = m_nameToCanonicalName.begin();
								iter != m_nameToCanonicalName.end(); ) {
		if (iter->second == canonical) {
			m_nameToCanonicalName.erase(iter++);
		}
		else {
			++index;
		}
	}
}

void
CConfig::removeAllScreens()
{
	m_map.clear();
	m_nameToCanonicalName.clear();
}

bool
CConfig::addAlias(const CString& canonical, const CString& alias)
{
	// alias name must not exist
	if (m_nameToCanonicalName.find(alias) != m_nameToCanonicalName.end()) {
		return false;
	}

	// canonical name must be known
	if (m_map.find(canonical) == m_map.end()) {
		return false;
	}

	// insert alias
	m_nameToCanonicalName.insert(std::make_pair(alias, canonical));

	return true;
}

bool
CConfig::removeAlias(const CString& alias)
{
	// must not be a canonical name
	if (m_map.find(alias) != m_map.end()) {
		return false;
	}

	// find alias
	CNameMap::iterator index = m_nameToCanonicalName.find(alias);
	if (index == m_nameToCanonicalName.end()) {
		return false;
	}

	// remove alias
	m_nameToCanonicalName.erase(index);

	return true;
}

bool
CConfig::removeAliases(const CString& canonical)
{
	// must be a canonical name
	if (m_map.find(canonical) == m_map.end()) {
		return false;
	}

	// find and removing matching aliases
	for (CNameMap::iterator index = m_nameToCanonicalName.begin();
							index != m_nameToCanonicalName.end(); ) {
		if (index->second == canonical && index->first != canonical) {
			m_nameToCanonicalName.erase(index++);
		}
		else {
			++index;
		}
	}

	return true;
}

void
CConfig::removeAllAliases()
{
	// remove all names
	m_nameToCanonicalName.clear();

	// put the canonical names back in
	for (CCellMap::iterator index = m_map.begin();
								index != m_map.end(); ++index) {
		m_nameToCanonicalName.insert(
								std::make_pair(index->first, index->first));
	}
}

bool
CConfig::connect(const CString& srcName,
				EDirection srcSide,
				float srcStart, float srcEnd,
				const CString& dstName,
				float dstStart, float dstEnd)
{
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// find source cell
	CCellMap::iterator index = m_map.find(getCanonicalName(srcName));
	if (index == m_map.end()) {
		return false;
	}

	// add link
	CCellEdge srcEdge(srcSide, CInterval(srcStart, srcEnd));
	CCellEdge dstEdge(dstName, srcSide, CInterval(dstStart, dstEnd));
	return index->second.add(srcEdge, dstEdge);
}

bool
CConfig::disconnect(const CString& srcName, EDirection srcSide)
{
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// find source cell
	CCellMap::iterator index = m_map.find(srcName);
	if (index == m_map.end()) {
		return false;
	}

	// disconnect side
	index->second.remove(srcSide);

	return true;
}

bool
CConfig::disconnect(const CString& srcName, EDirection srcSide, float position)
{
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// find source cell
	CCellMap::iterator index = m_map.find(srcName);
	if (index == m_map.end()) {
		return false;
	}

	// disconnect side
	index->second.remove(srcSide, position);

	return true;
}

void
CConfig::setSynergyAddress(const CNetworkAddress& addr)
{
	m_synergyAddress = addr;
}

bool
CConfig::addOption(const CString& name, OptionID option, OptionValue value)
{
	// find options
	CScreenOptions* options = NULL;
	if (name.empty()) {
		options = &m_globalOptions;
	}
	else {
		CCellMap::iterator index = m_map.find(name);
		if (index != m_map.end()) {
			options = &index->second.m_options;
		}
	}
	if (options == NULL) {
		return false;
	}

	// add option
	options->insert(std::make_pair(option, value));
	return true;
}

bool
CConfig::removeOption(const CString& name, OptionID option)
{
	// find options
	CScreenOptions* options = NULL;
	if (name.empty()) {
		options = &m_globalOptions;
	}
	else {
		CCellMap::iterator index = m_map.find(name);
		if (index != m_map.end()) {
			options = &index->second.m_options;
		}
	}
	if (options == NULL) {
		return false;
	}

	// remove option
	options->erase(option);
	return true;
}

bool
CConfig::removeOptions(const CString& name)
{
	// find options
	CScreenOptions* options = NULL;
	if (name.empty()) {
		options = &m_globalOptions;
	}
	else {
		CCellMap::iterator index = m_map.find(name);
		if (index != m_map.end()) {
			options = &index->second.m_options;
		}
	}
	if (options == NULL) {
		return false;
	}

	// remove options
	options->clear();
	return true;
}

bool
CConfig::isValidScreenName(const CString& name) const
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
	CString::size_type b = 0;
	for (;;) {
		// accept trailing .
		if (b == name.size()) {
			break;
		}

		// find end of part
		CString::size_type e = name.find('.', b);
		if (e == CString::npos) {
			e = name.size();
		}

		// part may not be empty
		if (e - b < 1) {
			return false;
		}

		// check first and last characters
		if (!(isalnum(name[b]) || name[b] == '_') ||
			!(isalnum(name[e - 1]) || name[e - 1] == '_')) {
			return false;
		}

		// check interior characters
		for (CString::size_type i = b; i < e; ++i) {
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

CConfig::const_iterator
CConfig::begin() const
{
	return const_iterator(m_map.begin());
}

CConfig::const_iterator
CConfig::end() const
{
	return const_iterator(m_map.end());
}

CConfig::all_const_iterator
CConfig::beginAll() const
{
	return m_nameToCanonicalName.begin();
}

CConfig::all_const_iterator
CConfig::endAll() const
{
	return m_nameToCanonicalName.end();
}

bool
CConfig::isScreen(const CString& name) const
{
	return (m_nameToCanonicalName.count(name) > 0);
}

bool
CConfig::isCanonicalName(const CString& name) const
{
	return (!name.empty() &&
			CStringUtil::CaselessCmp::equal(getCanonicalName(name), name));
}

CString
CConfig::getCanonicalName(const CString& name) const
{
	CNameMap::const_iterator index = m_nameToCanonicalName.find(name);
	if (index == m_nameToCanonicalName.end()) {
		return CString();
	}
	else {
		return index->second;
	}
}

CString
CConfig::getNeighbor(const CString& srcName, EDirection srcSide,
				float position, float* positionOut) const
{
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// find source cell
	CCellMap::const_iterator index = m_map.find(getCanonicalName(srcName));
	if (index == m_map.end()) {
		return CString();
	}

	// find edge
	const CCellEdge* srcEdge, *dstEdge;
	if (!index->second.getLink(srcSide, position, srcEdge, dstEdge)) {
		// no neighbor
		return "";
	}
	else {
		// compute position on neighbor
		if (positionOut != NULL) {
			*positionOut =
				dstEdge->inverseTransform(srcEdge->transform(position));
		}

		// return neighbor's name
		return getCanonicalName(dstEdge->getName());
	}
}

bool
CConfig::hasNeighbor(const CString& srcName, EDirection srcSide) const
{
	return hasNeighbor(srcName, srcSide, 0.0f, 1.0f);
}

bool
CConfig::hasNeighbor(const CString& srcName, EDirection srcSide,
							float start, float end) const
{
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// find source cell
	CCellMap::const_iterator index = m_map.find(getCanonicalName(srcName));
	if (index == m_map.end()) {
		return false;
	}

	return index->second.overlaps(CCellEdge(srcSide, CInterval(start, end)));
}

CConfig::link_const_iterator
CConfig::beginNeighbor(const CString& srcName) const
{
	CCellMap::const_iterator index = m_map.find(getCanonicalName(srcName));
	assert(index != m_map.end());
	return index->second.begin();
}

CConfig::link_const_iterator
CConfig::endNeighbor(const CString& srcName) const
{
	CCellMap::const_iterator index = m_map.find(getCanonicalName(srcName));
	assert(index != m_map.end());
	return index->second.end();
}

const CNetworkAddress&
CConfig::getSynergyAddress() const
{
	return m_synergyAddress;
}

const CConfig::CScreenOptions*
CConfig::getOptions(const CString& name) const
{
	// find options
	const CScreenOptions* options = NULL;
	if (name.empty()) {
		options = &m_globalOptions;
	}
	else {
		CCellMap::const_iterator index = m_map.find(name);
		if (index != m_map.end()) {
			options = &index->second.m_options;
		}
	}

	// return options
	return options;
}

bool
CConfig::hasLockToScreenAction() const
{
	return m_hasLockToScreenAction;
}

bool
CConfig::operator==(const CConfig& x) const
{
	if (m_synergyAddress != x.m_synergyAddress) {
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

	for (CCellMap::const_iterator index1 = m_map.begin(),
								index2 = x.m_map.begin();
								index1 != m_map.end(); ++index1, ++index2) {
		// compare names
		if (!CStringUtil::CaselessCmp::equal(index1->first, index2->first)) {
			return false;
		}

		// compare cells
		if (index1->second != index2->second) {
			return false;
		}
	}

	for (CNameMap::const_iterator index1 = m_nameToCanonicalName.begin(),
								index2 = x.m_nameToCanonicalName.begin();
								index1 != m_nameToCanonicalName.end();
								++index1, ++index2) {
		if (!CStringUtil::CaselessCmp::equal(index1->first,  index2->first) ||
			!CStringUtil::CaselessCmp::equal(index1->second, index2->second)) {
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
CConfig::operator!=(const CConfig& x) const
{
	return !operator==(x);
}

void
CConfig::read(CConfigReadContext& context)
{
	CConfig tmp(m_events);
	while (context.getStream()) {
		tmp.readSection(context);
	}
	*this = tmp;
}

const char*
CConfig::dirName(EDirection dir)
{
	static const char* s_name[] = { "left", "right", "up", "down" };

	assert(dir >= kFirstDirection && dir <= kLastDirection);

	return s_name[dir - kFirstDirection];
}

CInputFilter*
CConfig::getInputFilter()
{
	return &m_inputFilter;
}

CString
CConfig::formatInterval(const CInterval& x)
{
	if (x.first == 0.0f && x.second == 1.0f) {
		return "";
	}
	return CStringUtil::print("(%d,%d)", (int)(x.first * 100.0f + 0.5f),
										(int)(x.second * 100.0f + 0.5f));
}

void
CConfig::readSection(CConfigReadContext& s)
{
	static const char s_section[] = "section:";
	static const char s_options[] = "options";
	static const char s_screens[] = "screens";
	static const char s_links[]   = "links";
	static const char s_aliases[] = "aliases";

	CString line;
	if (!s.readLine(line)) {
		// no more sections
		return;
	}

	// should be a section header
	if (line.find(s_section) != 0) {
		throw XConfigRead(s, "found data outside section");
	}

	// get section name
	CString::size_type i = line.find_first_not_of(" \t", sizeof(s_section) - 1);
	if (i == CString::npos) {
		throw XConfigRead(s, "section name is missing");
	}
	CString name = line.substr(i);
	i = name.find_first_of(" \t");
	if (i != CString::npos) {
		throw XConfigRead(s, "unexpected data after section name");
	}

	// read section
	if (name == s_options) {
		readSectionOptions(s);
	}
	else if (name == s_screens) {
		readSectionScreens(s);
	}
	else if (name == s_links) {
		readSectionLinks(s);
	}
	else if (name == s_aliases) {
		readSectionAliases(s);
	}
	else {
		throw XConfigRead(s, "unknown section name \"%{1}\"", name);
	}
}

void
CConfig::readSectionOptions(CConfigReadContext& s)
{
	CString line;
	while (s.readLine(line)) {
		// check for end of section
		if (line == "end") {
			return;
		}

		// parse argument:  `nameAndArgs = [values][;[values]]'
		//   nameAndArgs  := <name>[(arg[,...])]
		//   values       := valueAndArgs[,valueAndArgs]...
		//   valueAndArgs := <value>[(arg[,...])]
		CString::size_type i = 0;
		CString name, value;
		CConfigReadContext::ArgList nameArgs, valueArgs;
		s.parseNameWithArgs("name", line, "=", i, name, nameArgs);
		++i;
		s.parseNameWithArgs("value", line, ",;\n", i, value, valueArgs);

		bool handled = true;
		if (name == "address") {
			try {
				m_synergyAddress = CNetworkAddress(value, kDefaultPort);
				m_synergyAddress.resolve();
			}
			catch (XSocketAddress& e) {
				throw XConfigRead(s,
							CString("invalid address argument ") + e.what());
			}
		}
		else if (name == "heartbeat") {
			addOption("", kOptionHeartbeat, s.parseInt(value));
		}
		else if (name == "switchCorners") {
			addOption("", kOptionScreenSwitchCorners, s.parseCorners(value));
		}
		else if (name == "switchCornerSize") {
			addOption("", kOptionScreenSwitchCornerSize, s.parseInt(value));
		}
		else if (name == "switchDelay") {
			addOption("", kOptionScreenSwitchDelay, s.parseInt(value));
		}
		else if (name == "switchDoubleTap") {
			addOption("", kOptionScreenSwitchTwoTap, s.parseInt(value));
		}
		else if (name == "switchNeedsShift") {
			addOption("", kOptionScreenSwitchNeedsShift, s.parseBoolean(value));
		}
		else if (name == "switchNeedsControl") {
			addOption("", kOptionScreenSwitchNeedsControl, s.parseBoolean(value));
		}
		else if (name == "switchNeedsAlt") {
			addOption("", kOptionScreenSwitchNeedsAlt, s.parseBoolean(value));
		}
		else if (name == "screenSaverSync") {
			addOption("", kOptionScreenSaverSync, s.parseBoolean(value));
		}
		else if (name == "relativeMouseMoves") {
			addOption("", kOptionRelativeMouseMoves, s.parseBoolean(value));
		}
		else if (name == "win32KeepForeground") {
			addOption("", kOptionWin32KeepForeground, s.parseBoolean(value));
		}
		else {
			handled = false;
		}

		if (handled) {
			// make sure handled options aren't followed by more values
			if (i < line.size() && (line[i] == ',' || line[i] == ';')) {
				throw XConfigRead(s, "to many arguments to %s", name.c_str());
			}
		}
		else {
			// make filter rule
			CInputFilter::CRule rule(parseCondition(s, name, nameArgs));

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
				if (i == CString::npos) {
					i = line.length();
				}
				else {
					--i;
				}

				// get actions
				while (i < line.length()) {
					++i;
					s.parseNameWithArgs("value", line, ",\n",
								i, value, valueArgs);
					parseAction(s, value, valueArgs, rule, false);
				}
			}

			// add rule
			m_inputFilter.addFilterRule(rule);
		}
	}
	throw XConfigRead(s, "unexpected end of options section");
}

void
CConfig::readSectionScreens(CConfigReadContext& s)
{
	CString line;
	CString screen;
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
				throw XConfigRead(s, "invalid screen name \"%{1}\"", screen);
			}

			// add the screen to the configuration
			if (!addScreen(screen)) {
				throw XConfigRead(s, "duplicate screen name \"%{1}\"", screen);
			}
		}
		else if (screen.empty()) {
			throw XConfigRead(s, "argument before first screen");
		}
		else {
			// parse argument:  `<name>=<value>'
			CString::size_type i = line.find_first_of(" \t=");
			if (i == 0) {
				throw XConfigRead(s, "missing argument name");
			}
			if (i == CString::npos) {
				throw XConfigRead(s, "missing =");
			}
			CString name = line.substr(0, i);
			i = line.find_first_not_of(" \t", i);
			if (i == CString::npos || line[i] != '=') {
				throw XConfigRead(s, "missing =");
			}
			i = line.find_first_not_of(" \t", i + 1);
			CString value;
			if (i != CString::npos) {
				value = line.substr(i);
			}

			// handle argument
			if (name == "halfDuplexCapsLock") {
				addOption(screen, kOptionHalfDuplexCapsLock,
					s.parseBoolean(value));
			}
			else if (name == "halfDuplexNumLock") {
				addOption(screen, kOptionHalfDuplexNumLock,
					s.parseBoolean(value));
			}
			else if (name == "halfDuplexScrollLock") {
				addOption(screen, kOptionHalfDuplexScrollLock,
					s.parseBoolean(value));
			}
			else if (name == "shift") {
				addOption(screen, kOptionModifierMapForShift,
					s.parseModifierKey(value));
			}
			else if (name == "ctrl") {
				addOption(screen, kOptionModifierMapForControl,
					s.parseModifierKey(value));
			}
			else if (name == "alt") {
				addOption(screen, kOptionModifierMapForAlt,
					s.parseModifierKey(value));
			}
			else if (name == "altgr") {
				addOption(screen, kOptionModifierMapForAltGr,
					s.parseModifierKey(value));
			}
			else if (name == "meta") {
				addOption(screen, kOptionModifierMapForMeta,
					s.parseModifierKey(value));
			}
			else if (name == "super") {
				addOption(screen, kOptionModifierMapForSuper,
					s.parseModifierKey(value));
			}
			else if (name == "xtestIsXineramaUnaware") {
				addOption(screen, kOptionXTestXineramaUnaware,
					s.parseBoolean(value));
			}
			else if (name == "switchCorners") {
				addOption(screen, kOptionScreenSwitchCorners,
					s.parseCorners(value));
			}
			else if (name == "switchCornerSize") {
				addOption(screen, kOptionScreenSwitchCornerSize,
					s.parseInt(value));
			}
			else if (name == "preserveFocus") {
				addOption(screen, kOptionScreenPreserveFocus,
					s.parseBoolean(value));
			}
			else {
				// unknown argument
				throw XConfigRead(s, "unknown argument \"%{1}\"", name);
			}
		}
	}
	throw XConfigRead(s, "unexpected end of screens section");
}

void
CConfig::readSectionLinks(CConfigReadContext& s)
{
	CString line;
	CString screen;
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
				throw XConfigRead(s, "unknown screen name \"%{1}\"", screen);
			}
			if (!isCanonicalName(screen)) {
				throw XConfigRead(s, "cannot use screen name alias here");
			}
		}
		else if (screen.empty()) {
			throw XConfigRead(s, "argument before first screen");
		}
		else {
			// parse argument:  `<name>[(<s0>,<e0>)]=<value>[(<s1>,<e1>)]'
			// the stuff in brackets is optional.  interval values must be
			// in the range [0,100] and start < end.  if not given the
			// interval is taken to be (0,100).
			CString::size_type i = 0;
			CString side, dstScreen, srcArgString, dstArgString;
			CConfigReadContext::ArgList srcArgs, dstArgs;
			s.parseNameWithArgs("link", line, "=", i, side, srcArgs);
			++i;
			s.parseNameWithArgs("screen", line, "", i, dstScreen, dstArgs);
			CInterval srcInterval(s.parseInterval(srcArgs));
			CInterval dstInterval(s.parseInterval(dstArgs));

			// handle argument
			EDirection dir;
			if (side == "left") {
				dir = kLeft;
			}
			else if (side == "right") {
				dir = kRight;
			}
			else if (side == "up") {
				dir = kTop;
			}
			else if (side == "down") {
				dir = kBottom;
			}
			else {
				// unknown argument
				throw XConfigRead(s, "unknown side \"%{1}\" in link", side);
			}
			if (!isScreen(dstScreen)) {
				throw XConfigRead(s, "unknown screen name \"%{1}\"", dstScreen);
			}
			if (!connect(screen, dir,
						srcInterval.first, srcInterval.second,
						dstScreen,
						dstInterval.first, dstInterval.second)) {
				throw XConfigRead(s, "overlapping range");
			}
		}
	}
	throw XConfigRead(s, "unexpected end of links section");
}

void
CConfig::readSectionAliases(CConfigReadContext& s)
{
	CString line;
	CString screen;
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
				throw XConfigRead(s, "unknown screen name \"%{1}\"", screen);
			}
			if (!isCanonicalName(screen)) {
				throw XConfigRead(s, "cannot use screen name alias here");
			}
		}
		else if (screen.empty()) {
			throw XConfigRead(s, "argument before first screen");
		}
		else {
			// verify validity of screen name
			if (!isValidScreenName(line)) {
				throw XConfigRead(s, "invalid screen alias \"%{1}\"", line);
			}

			// add alias
			if (!addAlias(screen, line)) {
				throw XConfigRead(s, "alias \"%{1}\" is already used", line);
			}
		}
	}
	throw XConfigRead(s, "unexpected end of aliases section");
}


CInputFilter::CCondition*
CConfig::parseCondition(CConfigReadContext& s,
				const CString& name, const std::vector<CString>& args)
{
	if (name == "keystroke") {
		if (args.size() != 1) {
			throw XConfigRead(s, "syntax for condition: keystroke(modifiers+key)");
		}

		IPlatformScreen::CKeyInfo* keyInfo = s.parseKeystroke(args[0]);

		return new CInputFilter::CKeystrokeCondition(m_events, keyInfo);
	}

	if (name == "mousebutton") {
		if (args.size() != 1) {
			throw XConfigRead(s, "syntax for condition: mousebutton(modifiers+button)");
		}

		IPlatformScreen::CButtonInfo* mouseInfo = s.parseMouse(args[0]);

		return new CInputFilter::CMouseButtonCondition(m_events, mouseInfo);
	}

	if (name == "connect") {
		if (args.size() != 1) {
			throw XConfigRead(s, "syntax for condition: connect([screen])");
		}

		CString screen = args[0];
		if (isScreen(screen)) {
			screen = getCanonicalName(screen);
		}
		else if (!screen.empty()) {
			throw XConfigRead(s, "unknown screen name \"%{1}\" in connect", screen);
		}

		return new CInputFilter::CScreenConnectedCondition(m_events, screen);
	}

	throw XConfigRead(s, "unknown argument \"%{1}\"", name);
}

void
CConfig::parseAction(CConfigReadContext& s,
				const CString& name, const std::vector<CString>& args,
				CInputFilter::CRule& rule, bool activate)
{
	CInputFilter::CAction* action;

	if (name == "keystroke" || name == "keyDown" || name == "keyUp") {
		if (args.size() < 1 || args.size() > 2) {
			throw XConfigRead(s, "syntax for action: keystroke(modifiers+key[,screens])");
		}

		IPlatformScreen::CKeyInfo* keyInfo;
		if (args.size() == 1) {
			keyInfo = s.parseKeystroke(args[0]);
		}
		else {
			std::set<CString> screens;
			parseScreens(s, args[1], screens);
			keyInfo = s.parseKeystroke(args[0], screens);
		}

		if (name == "keystroke") {
			IPlatformScreen::CKeyInfo* keyInfo2 =
				IKeyState::CKeyInfo::alloc(*keyInfo);
			action = new CInputFilter::CKeystrokeAction(m_events, keyInfo2, true);
			rule.adoptAction(action, true);
			action   = new CInputFilter::CKeystrokeAction(m_events, keyInfo, false);
			activate = false;
		}
		else if (name == "keyDown") {
			action = new CInputFilter::CKeystrokeAction(m_events, keyInfo, true);
		}
		else {
			action = new CInputFilter::CKeystrokeAction(m_events, keyInfo, false);
		}
	}

	else if (name == "mousebutton" ||
				name == "mouseDown" || name == "mouseUp") {
		if (args.size() != 1) {
			throw XConfigRead(s, "syntax for action: mousebutton(modifiers+button)");
		}

		IPlatformScreen::CButtonInfo* mouseInfo = s.parseMouse(args[0]);

		if (name == "mousebutton") {
			IPlatformScreen::CButtonInfo* mouseInfo2 =
				IPlatformScreen::CButtonInfo::alloc(*mouseInfo);
			action = new CInputFilter::CMouseButtonAction(m_events, mouseInfo2, true);
			rule.adoptAction(action, true);
			action   = new CInputFilter::CMouseButtonAction(m_events, mouseInfo, false);
			activate = false;
		}
		else if (name == "mouseDown") {
			action = new CInputFilter::CMouseButtonAction(m_events, mouseInfo, true);
		}
		else {
			action = new CInputFilter::CMouseButtonAction(m_events, mouseInfo, false);
		}
	}

/* XXX -- not supported
	else if (name == "modifier") {
		if (args.size() != 1) {
			throw XConfigRead(s, "syntax for action: modifier(modifiers)");
		}

		KeyModifierMask mask = s.parseModifier(args[0]);

		action = new CInputFilter::CModifierAction(mask, ~mask);
	}
*/

	else if (name == "switchToScreen") {
		if (args.size() != 1) {
			throw XConfigRead(s, "syntax for action: switchToScreen(name)");
		}

		CString screen = args[0];
		if (isScreen(screen)) {
			screen = getCanonicalName(screen);
		}
		else if (!screen.empty()) {
			throw XConfigRead(s, "unknown screen name in switchToScreen");
		}

		action = new CInputFilter::CSwitchToScreenAction(m_events, screen);
	}

	else if (name == "switchInDirection") {
		if (args.size() != 1) {
			throw XConfigRead(s, "syntax for action: switchInDirection(<left|right|up|down>)");
		}

		EDirection direction;
		if (args[0] == "left") {
			direction = kLeft;
		}
		else if (args[0] == "right") {
			direction = kRight;
		}
		else if (args[0] == "up") {
			direction = kTop;
		}
		else if (args[0] == "down") {
			direction = kBottom;
		}
		else {
			throw XConfigRead(s, "unknown direction \"%{1}\" in switchToScreen", args[0]);
		}

		action = new CInputFilter::CSwitchInDirectionAction(m_events, direction);
	}

	else if (name == "lockCursorToScreen") {
		if (args.size() > 1) {
			throw XConfigRead(s, "syntax for action: lockCursorToScreen([{off|on|toggle}])");
		}

		CInputFilter::CLockCursorToScreenAction::Mode mode =
			CInputFilter::CLockCursorToScreenAction::kToggle;
		if (args.size() == 1) {
			if (args[0] == "off") {
				mode = CInputFilter::CLockCursorToScreenAction::kOff;
			}
			else if (args[0] == "on") {
				mode = CInputFilter::CLockCursorToScreenAction::kOn;
			}
			else if (args[0] == "toggle") {
				mode = CInputFilter::CLockCursorToScreenAction::kToggle;
			}
			else {
				throw XConfigRead(s, "syntax for action: lockCursorToScreen([{off|on|toggle}])");
			}
		}

		if (mode != CInputFilter::CLockCursorToScreenAction::kOff) {
			m_hasLockToScreenAction = true;
		}

		action = new CInputFilter::CLockCursorToScreenAction(m_events, mode);
	}

	else if (name == "keyboardBroadcast") {
		if (args.size() > 2) {
			throw XConfigRead(s, "syntax for action: keyboardBroadcast([{off|on|toggle}[,screens]])");
		}

		CInputFilter::CKeyboardBroadcastAction::Mode mode =
			CInputFilter::CKeyboardBroadcastAction::kToggle;
		if (args.size() >= 1) {
			if (args[0] == "off") {
				mode = CInputFilter::CKeyboardBroadcastAction::kOff;
			}
			else if (args[0] == "on") {
				mode = CInputFilter::CKeyboardBroadcastAction::kOn;
			}
			else if (args[0] == "toggle") {
				mode = CInputFilter::CKeyboardBroadcastAction::kToggle;
			}
			else {
				throw XConfigRead(s, "syntax for action: keyboardBroadcast([{off|on|toggle}[,screens]])");
			}
		}

		std::set<CString> screens;
		if (args.size() >= 2) {
			parseScreens(s, args[1], screens);
		}

		action = new CInputFilter::CKeyboardBroadcastAction(m_events, mode, screens);
	}

	else {
		throw XConfigRead(s, "unknown action argument \"%{1}\"", name);
	}

	rule.adoptAction(action, activate);
}

void
CConfig::parseScreens(CConfigReadContext& c,
				const CString& s, std::set<CString>& screens) const
{
	screens.clear();

	CString::size_type i = 0;
	while (i < s.size()) {
		// find end of next screen name
		CString::size_type j = s.find(':', i);
		if (j == CString::npos) {
			j = s.size();
		}

		// extract name
		CString rawName;
		i = s.find_first_not_of(" \t", i);
		if (i < j) {
			rawName = s.substr(i, s.find_last_not_of(" \t", j - 1) - i + 1);
		}

		// add name
		if (rawName == "*") {
			screens.insert("*");
		}
		else if (!rawName.empty()) {
			CString name = getCanonicalName(rawName);
			if (name.empty()) {
				throw XConfigRead(c, "unknown screen name \"%{1}\"", rawName);
			}
			screens.insert(name);
		}

		// next
		i = j + 1;
	}
}

const char*
CConfig::getOptionName(OptionID id)
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
	return NULL;
}

CString
CConfig::getOptionValue(OptionID id, OptionValue value)
{
	if (id == kOptionHalfDuplexCapsLock ||
		id == kOptionHalfDuplexNumLock ||
		id == kOptionHalfDuplexScrollLock ||
		id == kOptionScreenSwitchNeedsShift ||
		id == kOptionScreenSwitchNeedsControl ||
		id == kOptionScreenSwitchNeedsAlt ||
		id == kOptionScreenSaverSync ||
		id == kOptionXTestXineramaUnaware ||
		id == kOptionRelativeMouseMoves ||
		id == kOptionWin32KeepForeground ||
		id == kOptionScreenPreserveFocus) {
		return (value != 0) ? "true" : "false";
	}
	if (id == kOptionModifierMapForShift ||
		id == kOptionModifierMapForControl ||
		id == kOptionModifierMapForAlt ||
		id == kOptionModifierMapForAltGr ||
		id == kOptionModifierMapForMeta ||
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
	if (id == kOptionHeartbeat ||
		id == kOptionScreenSwitchCornerSize ||
		id == kOptionScreenSwitchDelay ||
		id == kOptionScreenSwitchTwoTap) {
		return CStringUtil::print("%d", value);
	}
	if (id == kOptionScreenSwitchCorners) {
		std::string result("none");
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
// CConfig::CName
//

CConfig::CName::CName(CConfig* config, const CString& name) :
	m_config(config),
	m_name(config->getCanonicalName(name))
{
	// do nothing
}

bool
CConfig::CName::operator==(const CString& name) const
{
	CString canonical = m_config->getCanonicalName(name);
	return CStringUtil::CaselessCmp::equal(canonical, m_name);
}


//
// CConfig::CCellEdge
//

CConfig::CCellEdge::CCellEdge(EDirection side, float position)
{
	init("", side, CInterval(position, position));
}

CConfig::CCellEdge::CCellEdge(EDirection side, const CInterval& interval)
{
	assert(interval.first >= 0.0f);
	assert(interval.second <= 1.0f);
	assert(interval.first < interval.second);

	init("", side, interval);
}

CConfig::CCellEdge::CCellEdge(const CString& name,
				EDirection side, const CInterval& interval)
{
	assert(interval.first >= 0.0f);
	assert(interval.second <= 1.0f);
	assert(interval.first < interval.second);

	init(name, side, interval);
}

CConfig::CCellEdge::~CCellEdge()
{
	// do nothing
}

void
CConfig::CCellEdge::init(const CString& name, EDirection side,
				const CInterval& interval)
{
	assert(side != kNoDirection);

	m_name     = name;
	m_side     = side;
	m_interval = interval;
}

CConfig::CInterval
CConfig::CCellEdge::getInterval() const
{
	return m_interval;
}

void
CConfig::CCellEdge::setName(const CString& newName)
{
	m_name = newName;
}

CString
CConfig::CCellEdge::getName() const
{
	return m_name;
}

EDirection
CConfig::CCellEdge::getSide() const
{
	return m_side;
}

bool
CConfig::CCellEdge::overlaps(const CCellEdge& edge) const
{
	const CInterval& x = m_interval;
	const CInterval& y = edge.m_interval;
	if (m_side != edge.m_side) {
		return false;
	}
	return  (x.first  >= y.first && x.first  <  y.second) ||
			(x.second >  y.first && x.second <= y.second) ||
			(y.first  >= x.first && y.first  <  x.second) ||
			(y.second >  x.first && y.second <= x.second);
}

bool
CConfig::CCellEdge::isInside(float x) const
{
	return (x >= m_interval.first && x < m_interval.second);
}

float
CConfig::CCellEdge::transform(float x) const
{
	return (x - m_interval.first) / (m_interval.second - m_interval.first);
}


float
CConfig::CCellEdge::inverseTransform(float x) const
{
	return x * (m_interval.second - m_interval.first) + m_interval.first;
}

bool
CConfig::CCellEdge::operator<(const CCellEdge& o) const
{
	if (static_cast<int>(m_side) < static_cast<int>(o.m_side)) {
		return true;
	}
	else if (static_cast<int>(m_side) > static_cast<int>(o.m_side)) {
		return false;
	}

	return (m_interval.first < o.m_interval.first);
}

bool
CConfig::CCellEdge::operator==(const CCellEdge& x) const
{
	return (m_side == x.m_side && m_interval == x.m_interval);
}

bool
CConfig::CCellEdge::operator!=(const CCellEdge& x) const
{
	return !operator==(x);
}


//
// CConfig::CCell
//

bool
CConfig::CCell::add(const CCellEdge& src, const CCellEdge& dst)
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

void
CConfig::CCell::remove(EDirection side)
{
	for (CEdgeLinks::iterator j = m_neighbors.begin();
							j != m_neighbors.end(); ) {
		if (j->first.getSide() == side) {
			m_neighbors.erase(j++);
		}
		else {
			++j;
		}
	}
}

void
CConfig::CCell::remove(EDirection side, float position)
{
	for (CEdgeLinks::iterator j = m_neighbors.begin();
							j != m_neighbors.end(); ++j) {
		if (j->first.getSide() == side && j->first.isInside(position)) {
			m_neighbors.erase(j);
			break;
		}
	}
}
void
CConfig::CCell::remove(const CName& name)
{
	for (CEdgeLinks::iterator j = m_neighbors.begin();
							j != m_neighbors.end(); ) {
		if (name == j->second.getName()) {
			m_neighbors.erase(j++);
		}
		else {
			++j;
		}
	}
}

void
CConfig::CCell::rename(const CName& oldName, const CString& newName)
{
	for (CEdgeLinks::iterator j = m_neighbors.begin();
							j != m_neighbors.end(); ++j) {
		if (oldName == j->second.getName()) {
			j->second.setName(newName);
		}
	}
}

bool
CConfig::CCell::hasEdge(const CCellEdge& edge) const
{
	CEdgeLinks::const_iterator i = m_neighbors.find(edge);
	return (i != m_neighbors.end() && i->first == edge);
}

bool
CConfig::CCell::overlaps(const CCellEdge& edge) const
{
	CEdgeLinks::const_iterator i = m_neighbors.upper_bound(edge);
	if (i != m_neighbors.end() && i->first.overlaps(edge)) {
		return true;
	}
	if (i != m_neighbors.begin() && (--i)->first.overlaps(edge)) {
		return true;
	}
	return false;
}

bool
CConfig::CCell::getLink(EDirection side, float position,
				const CCellEdge*& src, const CCellEdge*& dst) const
{
	CCellEdge edge(side, position);
	CEdgeLinks::const_iterator i = m_neighbors.upper_bound(edge);
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

bool
CConfig::CCell::operator==(const CCell& x) const
{
	// compare options
	if (m_options != x.m_options) {
		return false;
	}

	// compare links
	if (m_neighbors.size() != x.m_neighbors.size()) {
		return false;
	}
	for (CEdgeLinks::const_iterator index1 = m_neighbors.begin(),
								index2 = x.m_neighbors.begin();
								index1 != m_neighbors.end();
								++index1, ++index2) {
		if (index1->first != index2->first) {
			return false;
		}
		if (index1->second != index2->second) {
			return false;
		}

		// operator== doesn't compare names.  only compare destination
		// names.
		if (!CStringUtil::CaselessCmp::equal(index1->second.getName(),
								index2->second.getName())) {
			return false;
		}
	}
	return true;
}

bool
CConfig::CCell::operator!=(const CCell& x) const
{
	return !operator==(x);
}

CConfig::CCell::const_iterator
CConfig::CCell::begin() const
{
	return m_neighbors.begin();
}

CConfig::CCell::const_iterator
CConfig::CCell::end() const
{
	return m_neighbors.end();
}


//
// CConfig I/O
//

std::istream&
operator>>(std::istream& s, CConfig& config)
{
	CConfigReadContext context(s);
	config.read(context);
	return s;
}

std::ostream&
operator<<(std::ostream& s, const CConfig& config)
{
	// screens section
	s << "section: screens" << std::endl;
	for (CConfig::const_iterator screen = config.begin();
								screen != config.end(); ++screen) {
		s << "\t" << screen->c_str() << ":" << std::endl;
		const CConfig::CScreenOptions* options = config.getOptions(*screen);
		if (options != NULL && options->size() > 0) {
			for (CConfig::CScreenOptions::const_iterator
								option  = options->begin();
								option != options->end(); ++option) {
				const char* name = CConfig::getOptionName(option->first);
				CString value    = CConfig::getOptionValue(option->first,
															option->second);
				if (name != NULL && !value.empty()) {
					s << "\t\t" << name << " = " << value << std::endl;
				}
			}
		}
	}
	s << "end" << std::endl;

	// links section
	CString neighbor;
	s << "section: links" << std::endl;
	for (CConfig::const_iterator screen = config.begin();
								screen != config.end(); ++screen) {
		s << "\t" << screen->c_str() << ":" << std::endl;

		for (CConfig::link_const_iterator
				link = config.beginNeighbor(*screen),
				nend = config.endNeighbor(*screen); link != nend; ++link) {			
			s << "\t\t" << CConfig::dirName(link->first.getSide()) <<
				CConfig::formatInterval(link->first.getInterval()) <<
				" = " << link->second.getName().c_str() <<
				CConfig::formatInterval(link->second.getInterval()) <<
				std::endl;
		}
	}
	s << "end" << std::endl;

	// aliases section (if there are any)
	if (config.m_map.size() != config.m_nameToCanonicalName.size()) {
		// map canonical to alias
		typedef std::multimap<CString, CString,
								CStringUtil::CaselessCmp> CMNameMap;
		CMNameMap aliases;
		for (CConfig::CNameMap::const_iterator
								index = config.m_nameToCanonicalName.begin();
								index != config.m_nameToCanonicalName.end();
								++index) {
			if (index->first != index->second) {
				aliases.insert(std::make_pair(index->second, index->first));
			}
		}

		// dump it
		CString screen;
		s << "section: aliases" << std::endl;
		for (CMNameMap::const_iterator index = aliases.begin();
								index != aliases.end(); ++index) {
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
	const CConfig::CScreenOptions* options = config.getOptions("");
	if (options != NULL && options->size() > 0) {
		for (CConfig::CScreenOptions::const_iterator
							option  = options->begin();
							option != options->end(); ++option) {
			const char* name = CConfig::getOptionName(option->first);
			CString value    = CConfig::getOptionValue(option->first,
														option->second);
			if (name != NULL && !value.empty()) {
				s << "\t" << name << " = " << value << std::endl;
			}
		}
	}
	if (config.m_synergyAddress.isValid()) {
		s << "\taddress = " <<
			config.m_synergyAddress.getHostname().c_str() << std::endl;
	}
	s << config.m_inputFilter.format("\t");
	s << "end" << std::endl;

	return s;
}


//
// CConfigReadContext
//

CConfigReadContext::CConfigReadContext(std::istream& s, SInt32 firstLine) :
	m_stream(s),
	m_line(firstLine - 1)
{
	// do nothing
}

CConfigReadContext::~CConfigReadContext()
{
	// do nothing
}

bool
CConfigReadContext::readLine(CString& line)
{
	++m_line;
	while (std::getline(m_stream, line)) {
		// strip leading whitespace
		CString::size_type i = line.find_first_not_of(" \t");
		if (i != CString::npos) {
			line.erase(0, i);
		}

		// strip comments and then trailing whitespace
		i = line.find('#');
		if (i != CString::npos) {
			line.erase(i);
		}
		i = line.find_last_not_of(" \r\t");
		if (i != CString::npos) {
			line.erase(i + 1);
		}

		// return non empty line
		if (!line.empty()) {
			// make sure there are no invalid characters
			for (i = 0; i < line.length(); ++i) {
				if (!isgraph(line[i]) && line[i] != ' ' && line[i] != '\t') {
					throw XConfigRead(*this,
								"invalid character %{1}",
								CStringUtil::print("%#2x", line[i]));
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
CConfigReadContext::getLineNumber() const
{
	return m_line;
}

bool
CConfigReadContext::operator!() const
{
	return !m_stream;
}

OptionValue
CConfigReadContext::parseBoolean(const CString& arg) const
{
	if (CStringUtil::CaselessCmp::equal(arg, "true")) {
		return static_cast<OptionValue>(true);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "false")) {
		return static_cast<OptionValue>(false);
	}
	throw XConfigRead(*this, "invalid boolean argument \"%{1}\"", arg);
}

OptionValue
CConfigReadContext::parseInt(const CString& arg) const
{
	const char* s = arg.c_str();
	char* end;
	long tmp      = strtol(s, &end, 10);
	if (*end != '\0') {
		// invalid characters
		throw XConfigRead(*this, "invalid integer argument \"%{1}\"", arg);
	}
	OptionValue value = static_cast<OptionValue>(tmp);
	if (value != tmp) {
		// out of range
		throw XConfigRead(*this, "integer argument \"%{1}\" out of range", arg);
	}
	return value;
}

OptionValue
CConfigReadContext::parseModifierKey(const CString& arg) const
{
	if (CStringUtil::CaselessCmp::equal(arg, "shift")) {
		return static_cast<OptionValue>(kKeyModifierIDShift);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "ctrl")) {
		return static_cast<OptionValue>(kKeyModifierIDControl);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "alt")) {
		return static_cast<OptionValue>(kKeyModifierIDAlt);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "altgr")) {
		return static_cast<OptionValue>(kKeyModifierIDAltGr);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "meta")) {
		return static_cast<OptionValue>(kKeyModifierIDMeta);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "super")) {
		return static_cast<OptionValue>(kKeyModifierIDSuper);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "none")) {
		return static_cast<OptionValue>(kKeyModifierIDNull);
	}
	throw XConfigRead(*this, "invalid argument \"%{1}\"", arg);
}

OptionValue
CConfigReadContext::parseCorner(const CString& arg) const
{
	if (CStringUtil::CaselessCmp::equal(arg, "left")) {
		return kTopLeftMask | kBottomLeftMask;
	}
	else if (CStringUtil::CaselessCmp::equal(arg, "right")) {
		return kTopRightMask | kBottomRightMask;
	}
	else if (CStringUtil::CaselessCmp::equal(arg, "top")) {
		return kTopLeftMask | kTopRightMask;
	}
	else if (CStringUtil::CaselessCmp::equal(arg, "bottom")) {
		return kBottomLeftMask | kBottomRightMask;
	}
	else if (CStringUtil::CaselessCmp::equal(arg, "top-left")) {
		return kTopLeftMask;
	}
	else if (CStringUtil::CaselessCmp::equal(arg, "top-right")) {
		return kTopRightMask;
	}
	else if (CStringUtil::CaselessCmp::equal(arg, "bottom-left")) {
		return kBottomLeftMask;
	}
	else if (CStringUtil::CaselessCmp::equal(arg, "bottom-right")) {
		return kBottomRightMask;
	}
	else if (CStringUtil::CaselessCmp::equal(arg, "none")) {
		return kNoCornerMask;
	}
	else if (CStringUtil::CaselessCmp::equal(arg, "all")) {
		return kAllCornersMask;
	}
	throw XConfigRead(*this, "invalid argument \"%{1}\"", arg);
}

OptionValue
CConfigReadContext::parseCorners(const CString& args) const
{
	// find first token
	CString::size_type i = args.find_first_not_of(" \t", 0);
	if (i == CString::npos) {
		throw XConfigRead(*this, "missing corner argument");
	}
	CString::size_type j = args.find_first_of(" \t", i);

	// parse first corner token
	OptionValue corners = parseCorner(args.substr(i, j - i));

	// get +/-
	i = args.find_first_not_of(" \t", j);
	while (i != CString::npos) {
		// parse +/-
		bool add;
		if (args[i] == '-') {
			add = false;
		}
		else if (args[i] == '+') {
			add = true;
		}
		else {
			throw XConfigRead(*this,
							"invalid corner operator \"%{1}\"",
							CString(args.c_str() + i, 1));
		}

		// get next corner token
		i = args.find_first_not_of(" \t", i + 1);
		j = args.find_first_of(" \t", i);
		if (i == CString::npos) {
			throw XConfigRead(*this, "missing corner argument");
		}

		// parse next corner token
		if (add) {
			corners |= parseCorner(args.substr(i, j - i));
		}
		else {
			corners &= ~parseCorner(args.substr(i, j - i));
		}
		i = args.find_first_not_of(" \t", j);
	}

	return corners;
}

CConfig::CInterval
CConfigReadContext::parseInterval(const ArgList& args) const
{
	if (args.size() == 0) {
		return CConfig::CInterval(0.0f, 1.0f);
	}
	if (args.size() != 2 || args[0].empty() || args[1].empty()) {
		throw XConfigRead(*this, "invalid interval \"%{1}\"", concatArgs(args));
	}

	char* end;
	long startValue = strtol(args[0].c_str(), &end, 10);
	if (end[0] != '\0') {
		throw XConfigRead(*this, "invalid interval \"%{1}\"", concatArgs(args));
	}
	long endValue = strtol(args[1].c_str(), &end, 10);
	if (end[0] != '\0') {
		throw XConfigRead(*this, "invalid interval \"%{1}\"", concatArgs(args));
	}

	if (startValue < 0 || startValue > 100 ||
		endValue   < 0 || endValue   > 100 ||
		startValue >= endValue) {
		throw XConfigRead(*this, "invalid interval \"%{1}\"", concatArgs(args));
	}

	return CConfig::CInterval(startValue / 100.0f, endValue / 100.0f);
}

void
CConfigReadContext::parseNameWithArgs(
				const CString& type, const CString& line,
				const CString& delim, CString::size_type& index,
				CString& name, ArgList& args) const
{
	// skip leading whitespace
	CString::size_type i = line.find_first_not_of(" \t", index);
	if (i == CString::npos) {
		throw XConfigRead(*this, CString("missing ") + type);
	}

	// find end of name
	CString::size_type j = line.find_first_of(" \t(" + delim, i);
	if (j == CString::npos) {
		j = line.length();
	}

	// save name
	name = line.substr(i, j - i);
	args.clear();

	// is it okay to not find a delimiter?
	bool needDelim = (!delim.empty() && delim.find('\n') == CString::npos);

	// skip whitespace
	i = line.find_first_not_of(" \t", j);
	if (i == CString::npos && needDelim) {
		// expected delimiter but didn't find it
		throw XConfigRead(*this, CString("missing ") + delim[0]);
	}
	if (i == CString::npos) {
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
	while (j != CString::npos) {
		// extract arg
		CString arg(line.substr(i, j - i));
		i = j;

		// trim whitespace
		j = arg.find_first_not_of(" \t");
		if (j != CString::npos) {
			arg.erase(0, j);
		}
		j = arg.find_last_not_of(" \t");
		if (j != CString::npos) {
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
	if (j == CString::npos) {
		// expected )
		throw XConfigRead(*this, "missing )");
	}

	// eat ')'
	++i;

	// skip whitespace
	j = line.find_first_not_of(" \t", i);
	if (j == CString::npos && needDelim) {
		// expected delimiter but didn't find it
		throw XConfigRead(*this, CString("missing ") + delim[0]);
	}

	// verify delimiter
	if (needDelim && delim.find(line[j]) == CString::npos) {
		throw XConfigRead(*this, CString("expected ") + delim[0]);
	}

	if (j == CString::npos) {
		j = line.length();
	}

	index = j;
	return;
}

IPlatformScreen::CKeyInfo*
CConfigReadContext::parseKeystroke(const CString& keystroke) const
{
	return parseKeystroke(keystroke, std::set<CString>());
}

IPlatformScreen::CKeyInfo*
CConfigReadContext::parseKeystroke(const CString& keystroke,
				const std::set<CString>& screens) const
{
	CString s = keystroke;

	KeyModifierMask mask;
	if (!CKeyMap::parseModifiers(s, mask)) {
		throw XConfigRead(*this, "unable to parse key modifiers");
	}

	KeyID key;
	if (!CKeyMap::parseKey(s, key)) {
		throw XConfigRead(*this, "unable to parse key");
	}

	if (key == kKeyNone && mask == 0) {
		throw XConfigRead(*this, "missing key and/or modifiers in keystroke");
	}

	return IPlatformScreen::CKeyInfo::alloc(key, mask, 0, 0, screens);
}

IPlatformScreen::CButtonInfo*
CConfigReadContext::parseMouse(const CString& mouse) const
{
	CString s = mouse;

	KeyModifierMask mask;
	if (!CKeyMap::parseModifiers(s, mask)) {
		throw XConfigRead(*this, "unable to parse button modifiers");
	}

	char* end;
	ButtonID button = (ButtonID)strtol(s.c_str(), &end, 10);
	if (*end != '\0') {
		throw XConfigRead(*this, "unable to parse button");
	}
	if (s.empty() || button <= 0) {
		throw XConfigRead(*this, "invalid button");
	}

	return IPlatformScreen::CButtonInfo::alloc(button, mask);
}

KeyModifierMask
CConfigReadContext::parseModifier(const CString& modifiers) const
{
	CString s = modifiers;

	KeyModifierMask mask;
	if (!CKeyMap::parseModifiers(s, mask)) {
		throw XConfigRead(*this, "unable to parse modifiers");
	}

	if (mask == 0) {
		throw XConfigRead(*this, "no modifiers specified");
	}

	return mask;
}

CString
CConfigReadContext::concatArgs(const ArgList& args)
{
	CString s("(");
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
// CConfig I/O exceptions
//

XConfigRead::XConfigRead(const CConfigReadContext& context,
				const CString& error) :
	m_error(CStringUtil::print("line %d: %s",
							context.getLineNumber(), error.c_str()))
{
	// do nothing
}

XConfigRead::XConfigRead(const CConfigReadContext& context,
				const char* errorFmt, const CString& arg) :
	m_error(CStringUtil::print("line %d: ", context.getLineNumber()) +
							CStringUtil::format(errorFmt, arg.c_str()))
{
	// do nothing
}

XConfigRead::~XConfigRead()
{
	// do nothing
}

CString
XConfigRead::getWhat() const throw()
{
	return format("XConfigRead", "read error: %{1}", m_error.c_str());
}
