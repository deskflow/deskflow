/*
 * synergy -- mouse and keyboard sharing utility
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
 */

#include "CConfig.h"
#include "KeyTypes.h"
#include "XSocket.h"
#include "stdistream.h"
#include "stdostream.h"
#include <stdlib.h>

//
// CConfig
//

CConfig::CConfig()
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
	for (index = m_map.begin(); index != m_map.end(); ++index) {
		for (UInt32 i = 0; i < kNumDirections; ++i) {
			if (CStringUtil::CaselessCmp::equal(getCanonicalName(
						index->second.m_neighbor[i]), oldCanonical)) {
				index->second.m_neighbor[i] = newName;
			}
		}
	}

	// update alias targets
	if (CStringUtil::CaselessCmp::equal(oldName, oldCanonical)) {
		for (CNameMap::iterator index = m_nameToCanonicalName.begin();
							index != m_nameToCanonicalName.end(); ++index) {
			if (CStringUtil::CaselessCmp::equal(
							index->second, oldCanonical)) {
				index->second = newName;
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
	for (index = m_map.begin(); index != m_map.end(); ++index) {
		CCell& cell = index->second;
		for (UInt32 i = 0; i < kNumDirections; ++i) {
			if (getCanonicalName(cell.m_neighbor[i]) == canonical) {
				cell.m_neighbor[i].erase();
			}
		}
	}

	// remove aliases (and canonical name)
	for (CNameMap::iterator index = m_nameToCanonicalName.begin();
								index != m_nameToCanonicalName.end(); ) {
		if (index->second == canonical) {
			m_nameToCanonicalName.erase(index++);
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
				EDirection srcSide, const CString& dstName)
{
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// find source cell
	CCellMap::iterator index = m_map.find(getCanonicalName(srcName));
	if (index == m_map.end()) {
		return false;
	}

	// connect side (overriding any previous connection).  we
	// canonicalize in getNeighbor() instead of here because the
	// destination name doesn't have to exist yet.
	index->second.m_neighbor[srcSide - kFirstDirection] = dstName;

	return true;
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
	index->second.m_neighbor[srcSide - kFirstDirection].erase();

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
	return CStringUtil::CaselessCmp::equal(getCanonicalName(name), name);
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
CConfig::getNeighbor(const CString& srcName, EDirection srcSide) const
{
	assert(srcSide >= kFirstDirection && srcSide <= kLastDirection);

	// find source cell
	CCellMap::const_iterator index = m_map.find(getCanonicalName(srcName));
	if (index == m_map.end()) {
		return CString();
	}

	// return connection
	return getCanonicalName(index->second.m_neighbor[
								srcSide - kFirstDirection]);
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

		// compare options
		if (index1->second.m_options != index2->second.m_options) {
			return false;
		}

		// compare neighbors
		for (UInt32 i = 0; i < kNumDirections; ++i) {
			if (!CStringUtil::CaselessCmp::equal(index1->second.m_neighbor[i],
								index2->second.m_neighbor[i])) {
				return false;
			}
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

	return true;
}

bool
CConfig::operator!=(const CConfig& x) const
{
	return !operator==(x);
}

const char*
CConfig::dirName(EDirection dir)
{
	static const char* s_name[] = { "left", "right", "top", "bottom" };

	assert(dir >= kFirstDirection && dir <= kLastDirection);

	return s_name[dir - kFirstDirection];
}

bool
CConfig::readLine(std::istream& s, CString& line)
{
	s >> std::ws;
	while (std::getline(s, line)) {
		// strip comments and then trailing whitespace
		CString::size_type i = line.find('#');
		if (i != CString::npos) {
			line.erase(i);
		}
		i = line.find_last_not_of(" \r\t");
		if (i != CString::npos) {
			line.erase(i + 1);
		}

		// return non empty line
		if (!line.empty()) {
			return true;
		}
		s >> std::ws;
	}
	return false;
}

OptionValue
CConfig::parseBoolean(const CString& arg)
{
	if (CStringUtil::CaselessCmp::equal(arg, "true")) {
		return static_cast<OptionValue>(true);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "false")) {
		return static_cast<OptionValue>(false);
	}
	throw XConfigRead("invalid argument");
}

OptionValue
CConfig::parseInt(const CString& arg)
{
	const char* s = arg.c_str();
	char* end;
	long tmp      = strtol(s, &end, 10);
	if (*end != '\0') {
		// invalid characters
		throw XConfigRead("invalid argument");
	}
	OptionValue value = static_cast<OptionValue>(tmp);
	if (value != tmp) {
		// out of range
		throw XConfigRead("argument out of range");
	}
	return value;
}

OptionValue
CConfig::parseModifierKey(const CString& arg)
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
	if (CStringUtil::CaselessCmp::equal(arg, "meta")) {
		return static_cast<OptionValue>(kKeyModifierIDMeta);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "super")) {
		return static_cast<OptionValue>(kKeyModifierIDSuper);
	}
	if (CStringUtil::CaselessCmp::equal(arg, "none")) {
		return static_cast<OptionValue>(kKeyModifierIDNull);
	}
	throw XConfigRead("invalid argument");
}

OptionValue
CConfig::parseCorner(const CString& arg)
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
	throw XConfigRead("invalid argument");
}

OptionValue
CConfig::parseCorners(const CString& args)
{
	// find first token
	std::string::size_type i = args.find_first_not_of(" \t", 0);
	if (i == std::string::npos) {
		throw XConfigRead("missing corner argument");
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
		}
		else if (args[i] == '+') {
			add = true;
		}
		else {
			throw XConfigRead("invalid operator");
		}

		// get next corner token
		i = args.find_first_not_of(" \t", i + 1);
		j = args.find_first_of(" \t", i);
		if (i == std::string::npos) {
			throw XConfigRead("missing corner argument");
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
	return NULL;
}

CString
CConfig::getOptionValue(OptionID id, OptionValue value)
{
	if (id == kOptionHalfDuplexCapsLock ||
		id == kOptionHalfDuplexNumLock ||
		id == kOptionHalfDuplexScrollLock ||
		id == kOptionScreenSaverSync ||
		id == kOptionXTestXineramaUnaware ||
		id == kOptionRelativeMouseMoves ||
		id == kOptionWin32KeepForeground) {
		return (value != 0) ? "true" : "false";
	}
	if (id == kOptionModifierMapForShift ||
		id == kOptionModifierMapForControl ||
		id == kOptionModifierMapForAlt ||
		id == kOptionModifierMapForMeta ||
		id == kOptionModifierMapForSuper) {
		switch (value) {
		case kKeyModifierIDShift:
			return "shift";

		case kKeyModifierIDControl:
			return "ctrl";

		case kKeyModifierIDAlt:
			return "alt";

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
			result += " +bottom-left";
		}
		return result;
	}

	return "";
}

void
CConfig::readSection(std::istream& s)
{
	static const char s_section[] = "section:";
	static const char s_options[] = "options";
	static const char s_screens[] = "screens";
	static const char s_links[]   = "links";
	static const char s_aliases[] = "aliases";

	CString line;
	if (!readLine(s, line)) {
		// no more sections
		return;
	}

	// should be a section header
	if (line.find(s_section) != 0) {
		throw XConfigRead("found data outside section");
	}

	// get section name
	CString::size_type i = line.find_first_not_of(" \t", sizeof(s_section) - 1);
	if (i == CString::npos) {
		throw XConfigRead("section name is missing");
	}
	CString name = line.substr(i);
	i = name.find_first_of(" \t");
	if (i != CString::npos) {
		throw XConfigRead("unexpected data after section name");
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
		throw XConfigRead("unknown section name");
	}
}

void
CConfig::readSectionOptions(std::istream& s)
{
	CString line;
	CString name;
	while (readLine(s, line)) {
		// check for end of section
		if (line == "end") {
			return;
		}

		// parse argument:  `<name>=<value>'
		CString::size_type i = line.find_first_of(" \t=");
		if (i == 0) {
			throw XConfigRead("missing argument name");
		}
		if (i == CString::npos) {
			throw XConfigRead("missing = in argument");
		}
		CString name = line.substr(0, i);
		i = line.find_first_not_of(" \t", i);
		if (i == CString::npos || line[i] != '=') {
			throw XConfigRead("missing = in argument");
		}
		i = line.find_first_not_of(" \t", i + 1);
		CString value;
		if (i != CString::npos) {
			value = line.substr(i);
		}
		if (value.empty()) {
			throw XConfigRead("missing value after =");
		}

		if (name == "address") {
			try {
				m_synergyAddress = CNetworkAddress(value, kDefaultPort);
				m_synergyAddress.resolve();
			}
			catch (XSocketAddress& e) {
				throw XConfigRead(CString("invalid address argument:  ") +
							e.what());
			}
		}
		else if (name == "heartbeat") {
			addOption("", kOptionHeartbeat, parseInt(value));
		}
		else if (name == "switchCorners") {
			addOption("", kOptionScreenSwitchCorners, parseCorners(value));
		}
		else if (name == "switchCornerSize") {
			addOption("", kOptionScreenSwitchCornerSize, parseInt(value));
		}
		else if (name == "switchDelay") {
			addOption("", kOptionScreenSwitchDelay, parseInt(value));
		}
		else if (name == "switchDoubleTap") {
			addOption("", kOptionScreenSwitchTwoTap, parseInt(value));
		}
		else if (name == "screenSaverSync") {
			addOption("", kOptionScreenSaverSync, parseBoolean(value));
		}
		else if (name == "relativeMouseMoves") {
			addOption("", kOptionRelativeMouseMoves, parseBoolean(value));
		}
		else if (name == "win32KeepForeground") {
			addOption("", kOptionWin32KeepForeground, parseBoolean(value));
		}
		else {
			throw XConfigRead("unknown argument");
		}
	}
	throw XConfigRead("unexpected end of screens section");
}

void
CConfig::readSectionScreens(std::istream& s)
{
	CString line;
	CString screen;
	while (readLine(s, line)) {
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
				throw XConfigRead("invalid screen name");
			}

			// add the screen to the configuration
			if (!addScreen(screen)) {
				throw XConfigRead("duplicate screen name");
			}
		}
		else if (screen.empty()) {
			throw XConfigRead("argument before first screen");
		}
		else {
			// parse argument:  `<name>=<value>'
			CString::size_type i = line.find_first_of(" \t=");
			if (i == 0) {
				throw XConfigRead("missing argument name");
			}
			if (i == CString::npos) {
				throw XConfigRead("missing = in argument");
			}
			CString name = line.substr(0, i);
			i = line.find_first_not_of(" \t", i);
			if (i == CString::npos || line[i] != '=') {
				throw XConfigRead("missing = in argument");
			}
			i = line.find_first_not_of(" \t", i + 1);
			CString value;
			if (i != CString::npos) {
				value = line.substr(i);
			}

			// handle argument
			if (name == "halfDuplexCapsLock") {
				addOption(screen, kOptionHalfDuplexCapsLock,
					parseBoolean(value));
			}
			else if (name == "halfDuplexNumLock") {
				addOption(screen, kOptionHalfDuplexNumLock,
					parseBoolean(value));
			}
			else if (name == "halfDuplexScrollLock") {
				addOption(screen, kOptionHalfDuplexScrollLock,
					parseBoolean(value));
			}
			else if (name == "shift") {
				addOption(screen, kOptionModifierMapForShift,
					parseModifierKey(value));
			}
			else if (name == "ctrl") {
				addOption(screen, kOptionModifierMapForControl,
					parseModifierKey(value));
			}
			else if (name == "alt") {
				addOption(screen, kOptionModifierMapForAlt,
					parseModifierKey(value));
			}
			else if (name == "meta") {
				addOption(screen, kOptionModifierMapForMeta,
					parseModifierKey(value));
			}
			else if (name == "super") {
				addOption(screen, kOptionModifierMapForSuper,
					parseModifierKey(value));
			}
			else if (name == "xtestIsXineramaUnaware") {
				addOption(screen, kOptionXTestXineramaUnaware,
					parseBoolean(value));
			}
			else if (name == "switchCorners") {
				addOption(screen, kOptionScreenSwitchCorners,
					parseCorners(value));
			}
			else if (name == "switchCornerSize") {
				addOption(screen, kOptionScreenSwitchCornerSize,
					parseInt(value));
			}
			else {
				// unknown argument
				throw XConfigRead("unknown argument");
			}
		}
	}
	throw XConfigRead("unexpected end of screens section");
}

void
CConfig::readSectionLinks(std::istream& s)
{
	CString line;
	CString screen;
	while (readLine(s, line)) {
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
				throw XConfigRead("unknown screen name");
			}
			if (!isCanonicalName(screen)) {
				throw XConfigRead("cannot use screen name alias here");
			}
		}
		else if (screen.empty()) {
			throw XConfigRead("argument before first screen");
		}
		else {
			// parse argument:  `<name>=<value>'
			CString::size_type i = line.find_first_of(" \t=");
			if (i == 0) {
				throw XConfigRead("missing argument name");
			}
			if (i == CString::npos) {
				throw XConfigRead("missing = in argument");
			}
			CString name = line.substr(0, i);
			i = line.find_first_not_of(" \t", i);
			if (i == CString::npos || line[i] != '=') {
				throw XConfigRead("missing = in argument");
			}
			i = line.find_first_not_of(" \t", i + 1);
			CString value;
			if (i != CString::npos) {
				value = line.substr(i);
			}

			// handle argument
			if (name == "left") {
				if (!isScreen(value)) {
					throw XConfigRead("unknown screen");
				}
				connect(screen, kLeft, value);
			}
			else if (name == "right") {
				if (!isScreen(value)) {
					throw XConfigRead("unknown screen");
				}
				connect(screen, kRight, value);
			}
			else if (name == "up") {
				if (!isScreen(value)) {
					throw XConfigRead("unknown screen");
				}
				connect(screen, kTop, value);
			}
			else if (name == "down") {
				if (!isScreen(value)) {
					throw XConfigRead("unknown screen");
				}
				connect(screen, kBottom, value);
			}
			else {
				// unknown argument
				throw XConfigRead("unknown argument");
			}
		}
	}
	throw XConfigRead("unexpected end of links section");
}

void
CConfig::readSectionAliases(std::istream& s)
{
	CString line;
	CString screen;
	while (readLine(s, line)) {
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
				throw XConfigRead("unknown screen name");
			}
			if (!isCanonicalName(screen)) {
				throw XConfigRead("cannot use screen name alias here");
			}
		}
		else if (screen.empty()) {
			throw XConfigRead("argument before first screen");
		}
		else {
			// verify validity of screen name
			if (!isValidScreenName(line)) {
				throw XConfigRead("invalid screen alias");
			}

			// add alias
			if (!addAlias(screen, line)) {
				throw XConfigRead("alias is duplicate screen name");
			}
		}
	}
	throw XConfigRead("unexpected end of aliases section");
}


//
// CConfig I/O
//

std::istream&
operator>>(std::istream& s, CConfig& config)
{
	// FIXME -- should track line and column to improve error reporting

	CConfig tmp;
	while (s) {
		tmp.readSection(s);
	}
	config = tmp;
	return s;
}

std::ostream&
operator<<(std::ostream& s, const CConfig& config)
{
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
	s << "end" << std::endl;

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

		neighbor = config.getNeighbor(*screen, kLeft);
		if (!neighbor.empty()) {
			s << "\t\tleft=" << neighbor.c_str() << std::endl;
		}

		neighbor = config.getNeighbor(*screen, kRight);
		if (!neighbor.empty()) {
			s << "\t\tright=" << neighbor.c_str() << std::endl;
		}

		neighbor = config.getNeighbor(*screen, kTop);
		if (!neighbor.empty()) {
			s << "\t\tup=" << neighbor.c_str() << std::endl;
		}

		neighbor = config.getNeighbor(*screen, kBottom);
		if (!neighbor.empty()) {
			s << "\t\tdown=" << neighbor.c_str() << std::endl;
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

	return s;
}


//
// CConfig I/O exceptions
//

XConfigRead::XConfigRead(const CString& error) :
	m_error(error)
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
