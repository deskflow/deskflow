#include "CConfig.h"
#include "stdistream.h"
#include "stdostream.h"
#include <assert.h>

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

void					CConfig::addScreen(const CString& name)
{
	if (m_map.count(name) != 0) {
		assert(0 && "name already in map");	// FIXME -- throw instead
	}
	m_map.insert(std::make_pair(name, CCell()));
}

void					CConfig::removeScreen(const CString& name)
{
	CCellMap::iterator index = m_map.find(name);
	if (index == m_map.end()) {
		assert(0 && "name not in map");	// FIXME -- throw instead
	}

	// remove from map
	m_map.erase(index);

	// disconnect
	for (index = m_map.begin(); index != m_map.end(); ++index) {
		CCell& cell = index->second;
		for (SInt32 i = 0; i <= kLastDirection - kFirstDirection; ++i)
			if (cell.m_neighbor[i] == name) {
				cell.m_neighbor[i].erase();
			}
	}
}

void					CConfig::removeAllScreens()
{
	m_map.clear();
}

void					CConfig::connect(const CString& srcName,
								EDirection srcSide,
								const CString& dstName)
{
	// find source cell
	CCellMap::iterator index = m_map.find(srcName);
	if (index == m_map.end()) {
		assert(0 && "name not in map");	// FIXME -- throw instead
	}

	// connect side (overriding any previous connection)
	index->second.m_neighbor[srcSide - kFirstDirection] = dstName;
}

void					CConfig::disconnect(const CString& srcName,
								EDirection srcSide)
{
	// find source cell
	CCellMap::iterator index = m_map.find(srcName);
	if (index == m_map.end()) {
		assert(0 && "name not in map");	// FIXME -- throw instead
	}

	// disconnect side
	index->second.m_neighbor[srcSide - kFirstDirection].erase();
}

bool					CConfig::isValidScreenName(const CString& name) const
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

CConfig::const_iterator	CConfig::begin() const
{
	return const_iterator(m_map.begin());
}

CConfig::const_iterator	CConfig::end() const
{
	return const_iterator(m_map.end());
}

bool					CConfig::isScreen(const CString& name) const
{
	return (m_map.count(name) > 0);
}

CString					CConfig::getNeighbor(const CString& srcName,
								EDirection srcSide) const
{
	// find source cell
	CCellMap::const_iterator index = m_map.find(srcName);
	if (index == m_map.end()) {
		assert(0 && "name not in map");	// FIXME -- throw instead
	}

	// return connection
	return index->second.m_neighbor[srcSide - kFirstDirection];
}

const char*				CConfig::dirName(EDirection dir)
{
	static const char* s_name[] = { "left", "right", "top", "bottom" };
	return s_name[dir - kFirstDirection];
}

bool					CConfig::readLine(std::istream& s, CString& line)
{
	s >> std::ws;
	while (std::getline(s, line)) {
		// strip comments and then trailing whitespace
		CString::size_type i = line.rfind('#');
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

void					CConfig::readSection(std::istream& s)
{
	static const char s_section[] = "section:";
	static const char s_screens[] = "screens";
	static const char s_links[]   = "links";

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
	if (name == s_screens) {
		readSectionScreens(s);
	}
	else if (name == s_links) {
		readSectionLinks(s);
	}
	else {
		throw XConfigRead("unknown section name");
	}
}

void					CConfig::readSectionScreens(std::istream& s)
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
			addScreen(name);
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

void					CConfig::readSectionLinks(std::istream& s)
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

			// verify we known about the screen
			if (!isScreen(screen)) {
				throw XConfigRead("unknown screen name");
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


//
// CConfig I/O
//

std::istream&			operator>>(std::istream& s, CConfig& config)
{
	// FIXME -- should track line and column to improve error reporting

	CConfig tmp;
	while (s) {
		tmp.readSection(s);
	}
	config = tmp;
	return s;
}

std::ostream&			operator<<(std::ostream& s, const CConfig& config)
{
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

		neighbor = config.getNeighbor(*screen, CConfig::kLeft);
		if (!neighbor.empty()) {
			s << "\t\tleft=" << neighbor.c_str() << std::endl;
		}

		neighbor = config.getNeighbor(*screen, CConfig::kRight);
		if (!neighbor.empty()) {
			s << "\t\tright=" << neighbor.c_str() << std::endl;
		}

		neighbor = config.getNeighbor(*screen, CConfig::kTop);
		if (!neighbor.empty()) {
			s << "\t\tup=" << neighbor.c_str() << std::endl;
		}

		neighbor = config.getNeighbor(*screen, CConfig::kBottom);
		if (!neighbor.empty()) {
			s << "\t\tdown=" << neighbor.c_str() << std::endl;
		}
	}
	s << "end" << std::endl;

	return s;
}


//
// CConfig I/O exceptions
//

XConfigRead::XConfigRead(const CString& error) : m_error(error)
{
	// do nothing
}

XConfigRead::~XConfigRead()
{
	// do nothing
}

CString					XConfigRead::getWhat() const throw()
{
	return m_error;
}
