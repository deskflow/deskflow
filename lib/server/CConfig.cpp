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
#include "ProtocolTypes.h"
#include "XSocket.h"
#include "stdistream.h"
#include "stdostream.h"

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
		for (SInt32 i = 0; i <= kLastDirection - kFirstDirection; ++i)
			if (getCanonicalName(cell.m_neighbor[i]) == canonical) {
				cell.m_neighbor[i].erase();
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
	if (m_nameToCanonicalName.find(canonical) == m_nameToCanonicalName.end()) {
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

void
CConfig::setHTTPAddress(const CNetworkAddress& addr)
{
	m_httpAddress = addr;
}

bool
CConfig::isValidScreenName(const CString& name) const
{
	// name is valid if matches validname
	//  name      ::= [A-Za-z0-9] | [A-Za-z0-9][-A-Za-z0-9]*[A-Za-z0-9]
	//  domain    ::= . name
	//  validname ::= name domain*

	// check each dot separated part
	CString::size_type b = 0;
	for (;;) {
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
		if (!isalnum(name[b]) || !isalnum(name[e - 1])) {
			return false;
		}

		// check interior characters
		for (CString::size_type i = b; i < e; ++i) {
			if (!isalnum(name[i]) && name[i] != '-') {
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

const CNetworkAddress&
CConfig::getHTTPAddress() const
{
	return m_httpAddress;
}

const char*
CConfig::dirName(EDirection dir)
{
	static const char* s_name[] = { "left", "right", "top", "bottom" };
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
		i = line.find_last_not_of(" \t");
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

void
CConfig::readSection(std::istream& s)
{
	static const char s_section[] = "section:";
	static const char s_network[] = "network";
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
	if (name == s_network) {
		readSectionNetwork(s);
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
CConfig::readSectionNetwork(std::istream& s)
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
			}
			catch (XSocketAddress&) {
				throw XConfigRead("invalid address argument");
			}
		}
		else if (name == "http") {
			try {
				m_httpAddress = CNetworkAddress(value, kDefaultPort + 1);
			}
			catch (XSocketAddress&) {
				throw XConfigRead("invalid http argument");
			}
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
	CString name;
	while (readLine(s, line)) {
		// check for end of section
		if (line == "end") {
			return;
		}

		// see if it's the next screen
		if (line[line.size() - 1] == ':') {
			// strip :
			name = line.substr(0, line.size() - 1);

			// verify validity of screen name
			if (!isValidScreenName(name)) {
				throw XConfigRead("invalid screen name");
			}

			// add the screen to the configuration
			if (!addScreen(name)) {
				throw XConfigRead("duplicate screen name");
			}
		}
		else if (name.empty()) {
			throw XConfigRead("argument before first screen");
		}
		else {
			throw XConfigRead("unknown argument");
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
	// network section
	s << "section: network" << std::endl;
	if (config.m_synergyAddress.isValid()) {
		s << "\taddress=" << config.m_synergyAddress.getHostname().c_str() <<
								std::endl;
	}
	if (config.m_httpAddress.isValid()) {
		s << "\thttp=" << config.m_httpAddress.getHostname().c_str() <<
								std::endl;
	}
	s << "end" << std::endl;

	// screens section
	s << "section: screens" << std::endl;
	for (CConfig::const_iterator screen = config.begin();
								screen != config.end(); ++screen) {
		s << "\t" << screen->c_str() << ":" << std::endl;
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
		CConfig::CNameMap aliases;
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
		for (CConfig::CNameMap::const_iterator index = aliases.begin();
								index != aliases.end(); ++index) {
			if (index->first != screen) {
				screen = index->first;
				s << "\t" << screen.c_str() << std::endl;
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
	return format("XConfigRead", "read error: %s", m_error.c_str());
}
