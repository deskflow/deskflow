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

#ifndef CCONFIG_H
#define CCONFIG_H

#include "OptionTypes.h"
#include "ProtocolTypes.h"
#include "CNetworkAddress.h"
#include "CStringUtil.h"
#include "CInputFilter.h"
#include "XBase.h"
#include "stdmap.h"
#include "stdset.h"
#include "IPlatformScreen.h"
#include <iosfwd>

class CConfig;
class CConfigReadContext;

namespace std {
template <>
struct iterator_traits<CConfig> {
	typedef CString						value_type;
	typedef ptrdiff_t					difference_type;
	typedef bidirectional_iterator_tag	iterator_category;
	typedef CString*					pointer;
	typedef CString&					reference;
};
};

//! Server configuration
/*!
This class holds server configuration information.  That includes
the names of screens and their aliases, the links between them,
and network addresses.

Note that case is preserved in screen names but is ignored when
comparing names.  Screen names and their aliases share a
namespace and must be unique.
*/
class CConfig {
public:
	typedef std::map<OptionID, OptionValue> CScreenOptions;
	typedef std::pair<float, float> CInterval;

	class CCellEdge {
	public:
		CCellEdge(EDirection side, float position);
		CCellEdge(EDirection side, const CInterval&);
		CCellEdge(const CString& name, EDirection side, const CInterval&);
		~CCellEdge();

		CInterval		getInterval() const;
		void			setName(const CString& newName);
		CString			getName() const;
		EDirection		getSide() const;
		bool			overlaps(const CCellEdge&) const;
		bool			isInside(float x) const;

		// transform position to [0,1]
		float			transform(float x) const;

		// transform [0,1] to position
		float			inverseTransform(float x) const;

		// compares side and start of interval
		bool			operator<(const CCellEdge&) const;

		// compares side and interval
		bool			operator==(const CCellEdge&) const;
		bool			operator!=(const CCellEdge&) const;

	private:
		void			init(const CString& name, EDirection side,
							const CInterval&);

	private:
		CString			m_name;
		EDirection		m_side;
		CInterval		m_interval;
	};

private:
	class CName {
	public:
		CName(CConfig*, const CString& name);

		bool			operator==(const CString& name) const;

	private:
		CConfig*		m_config;
		CString			m_name;
	};

	class CCell {
	private:
		typedef std::map<CCellEdge, CCellEdge> CEdgeLinks;

	public:
		typedef CEdgeLinks::const_iterator const_iterator;

		bool			add(const CCellEdge& src, const CCellEdge& dst);
		void			remove(EDirection side);
		void			remove(EDirection side, float position);
		void			remove(const CName& destinationName);
		void			rename(const CName& oldName, const CString& newName);

		bool			hasEdge(const CCellEdge&) const;
		bool			overlaps(const CCellEdge&) const;

		bool			getLink(EDirection side, float position,
							const CCellEdge*& src, const CCellEdge*& dst) const;

		bool			operator==(const CCell&) const;
		bool			operator!=(const CCell&) const;

		const_iterator	begin() const;
		const_iterator	end() const;

	private:
		CEdgeLinks		m_neighbors;

	public:
		CScreenOptions	m_options;
	};
	typedef std::map<CString, CCell, CStringUtil::CaselessCmp> CCellMap;
	typedef std::map<CString, CString, CStringUtil::CaselessCmp> CNameMap;

public:
	typedef CCell::const_iterator link_const_iterator;
	typedef CCellMap::const_iterator internal_const_iterator;
	typedef CNameMap::const_iterator all_const_iterator;
	class const_iterator : std::iterator_traits<CConfig> {
	public:
		explicit const_iterator() : m_i() { }
		explicit const_iterator(const internal_const_iterator& i) : m_i(i) { }

		const_iterator&	operator=(const const_iterator& i) {
			m_i = i.m_i;
			return *this;
		}
		CString			operator*() { return m_i->first; }
		const CString*	operator->() { return &(m_i->first); }
		const_iterator&	operator++() { ++m_i;  return *this; }
		const_iterator	operator++(int) { return const_iterator(m_i++); }
		const_iterator&	operator--() { --m_i;  return *this; }
		const_iterator	operator--(int) { return const_iterator(m_i--); }
		bool			operator==(const const_iterator& i) const {
			return (m_i == i.m_i);
		}
		bool			operator!=(const const_iterator& i) const {
			return (m_i != i.m_i);
		}

	private:
		internal_const_iterator	m_i;
	};

	CConfig();
	virtual ~CConfig();

	//! @name manipulators
	//@{

	//! Add screen
	/*!
	Adds a screen, returning true iff successful.  If a screen or
	alias with the given name exists then it fails.
	*/
	bool				addScreen(const CString& name);

	//! Rename screen
	/*!
	Renames a screen.  All references to the name are updated.
	Returns true iff successful.
	*/
	bool				renameScreen(const CString& oldName,
							const CString& newName);

	//! Remove screen
	/*!
	Removes a screen.  This also removes aliases for the screen and
	disconnects any connections to the screen.  \c name may be an
	alias.
	*/
	void				removeScreen(const CString& name);

	//! Remove all screens
	/*!
	Removes all screens, aliases, and connections.
	*/
	void				removeAllScreens();

	//! Add alias
	/*!
	Adds an alias for a screen name.  An alias can be used
	any place the canonical screen name can (except addScreen()).
	Returns false if the alias name already exists or the canonical
	name is unknown, otherwise returns true.
	*/
	bool				addAlias(const CString& canonical,
							const CString& alias);

	//! Remove alias
	/*!
	Removes an alias for a screen name.  It returns false if the
	alias is unknown or a canonical name, otherwise returns true.
	*/
	bool				removeAlias(const CString& alias);

	//! Remove aliases
	/*!
	Removes all aliases for a canonical screen name.  It returns false
	if the canonical name is unknown, otherwise returns true.
	*/
	bool				removeAliases(const CString& canonical);

	//! Remove all aliases
	/*!
	This removes all aliases but not the screens.
	*/
	void				removeAllAliases();

	//! Connect screens
	/*!
	Establishes a one-way connection between portions of opposite edges
	of two screens.  Each portion is described by an interval defined
	by two numbers, the start and end of the interval half-open on the
	end.  The numbers range from 0 to 1, inclusive, for the left/top
	to the right/bottom.  The user will be able to jump from the
	\c srcStart to \c srcSend interval of \c srcSide of screen
	\c srcName to the opposite side of screen \c dstName in the interval
	\c dstStart and \c dstEnd when both screens are connected to the
	server and the user isn't locked to a screen.  Returns false if
	\c srcName is unknown.  \c srcStart must be less than or equal to
	\c srcEnd and \c dstStart must be less then or equal to \c dstEnd
	and all of \c srcStart, \c srcEnd, \c dstStart, or \c dstEnd must
	be inside the range [0,1].
	*/
	bool				connect(const CString& srcName,
							EDirection srcSide,
							float srcStart, float srcEnd,
							const CString& dstName,
							float dstStart, float dstEnd);

	//! Disconnect screens
	/*!
	Removes all connections created by connect() on side \c srcSide.
	Returns false if \c srcName is unknown.
	*/
	bool				disconnect(const CString& srcName,
							EDirection srcSide);

	//! Disconnect screens
	/*!
	Removes the connections created by connect() on side \c srcSide
	covering position \c position.  Returns false if \c srcName is
	unknown.
	*/
	bool				disconnect(const CString& srcName,
							EDirection srcSide, float position);

	//! Set server address
	/*!
	Set the synergy listen addresses.  There is no default address so
	this must be called to run a server using this configuration.
	*/
	void				setSynergyAddress(const CNetworkAddress&);

	//! Add a screen option
	/*!
	Adds an option and its value to the named screen.  Replaces the
	existing option's value if there is one.  Returns true iff \c name
	is a known screen.
	*/
	bool				addOption(const CString& name,
							OptionID option, OptionValue value);

	//! Remove a screen option
	/*!
	Removes an option and its value from the named screen.  Does
	nothing if the option doesn't exist on the screen.  Returns true
	iff \c name is a known screen.
	*/
	bool				removeOption(const CString& name, OptionID option);

	//! Remove a screen options
	/*!
	Removes all options and values from the named screen.  Returns true
	iff \c name is a known screen.
	*/
	bool				removeOptions(const CString& name);

	//! Get the hot key input filter
	/*!
	Returns the hot key input filter.  Clients can modify hotkeys using
	that object.
	*/
	CInputFilter*		getInputFilter();

	//@}
	//! @name accessors
	//@{

	//! Test screen name validity
	/*!
	Returns true iff \c name is a valid screen name.
	*/
	bool				isValidScreenName(const CString& name) const;

	//! Get beginning (canonical) screen name iterator
	const_iterator		begin() const;
	//! Get ending (canonical) screen name iterator
	const_iterator		end() const;

	//! Get beginning screen name iterator
	all_const_iterator	beginAll() const;
	//! Get ending screen name iterator
	all_const_iterator	endAll() const;

	//! Test for screen name
	/*!
	Returns true iff \c name names a screen.
	*/
	bool				isScreen(const CString& name) const;

	//! Test for canonical screen name
	/*!
	Returns true iff \c name is the canonical name of a screen.
	*/
	bool				isCanonicalName(const CString& name) const;

	//! Get canonical name
	/*!
	Returns the canonical name of a screen or the empty string if
	the name is unknown.  Returns the canonical name if one is given.
	*/
	CString				getCanonicalName(const CString& name) const;

	//! Get neighbor
	/*!
	Returns the canonical screen name of the neighbor in the given
	direction (set through connect()) at position \c position.  Returns
	the empty string if there is no neighbor in that direction, otherwise
	saves the position on the neighbor in \c positionOut if it's not
	\c NULL.
	*/
	CString				getNeighbor(const CString&, EDirection,
							float position, float* positionOut) const;

	//! Check for neighbor
	/*!
	Returns \c true if the screen has a neighbor anywhere along the edge
	given by the direction.
	*/
	bool				hasNeighbor(const CString&, EDirection) const;

	//! Check for neighbor
	/*!
	Returns \c true if the screen has a neighbor in the given range along
	the edge given by the direction.
	*/
	bool				hasNeighbor(const CString&, EDirection,
							float start, float end) const;

	//! Get beginning neighbor iterator
	link_const_iterator	beginNeighbor(const CString&) const;
	//! Get ending neighbor iterator
	link_const_iterator	endNeighbor(const CString&) const;

	//! Get the server address
	const CNetworkAddress&	getSynergyAddress() const;

	//! Get the screen options
	/*!
	Returns all the added options for the named screen.  Returns NULL
	if the screen is unknown and an empty collection if there are no
	options.
	*/
	const CScreenOptions* getOptions(const CString& name) const;

	//! Check for lock to screen action
	/*!
	Returns \c true if this configuration has a lock to screen action.
	This is for backwards compatible support of ScrollLock locking.
	*/
	bool					hasLockToScreenAction() const;

	//! Compare configurations
	bool				operator==(const CConfig&) const;
	//! Compare configurations
	bool				operator!=(const CConfig&) const;

	//! Read configuration
	/*!
	Reads a configuration from a context.  Throws XConfigRead on error
	and context is unchanged.
	*/
	void					read(CConfigReadContext& context);

	//! Read configuration
	/*!
	Reads a configuration from a stream.  Throws XConfigRead on error.
	*/
	friend std::istream&	operator>>(std::istream&, CConfig&);

	//! Write configuration
	/*!
	Writes a configuration to a stream.
	*/
	friend std::ostream&	operator<<(std::ostream&, const CConfig&);

	//! Get direction name
	/*!
	Returns the name of a direction (for debugging).
	*/
	static const char*	dirName(EDirection);

	//! Get interval as string
	/*!
	Returns an interval as a parseable string.
	*/
	static CString		formatInterval(const CInterval&);

	//@}

private:
	void				readSection(CConfigReadContext&);
	void				readSectionOptions(CConfigReadContext&);
	void				readSectionScreens(CConfigReadContext&);
	void				readSectionLinks(CConfigReadContext&);
	void				readSectionAliases(CConfigReadContext&);

	CInputFilter::CCondition*
						parseCondition(CConfigReadContext&,
							const CString& condition,
							const std::vector<CString>& args);
	void				parseAction(CConfigReadContext&,
							const CString& action,
							const std::vector<CString>& args,
							CInputFilter::CRule&, bool activate);

	void				parseScreens(CConfigReadContext&, const CString&,
							std::set<CString>& screens) const;
	static const char*	getOptionName(OptionID);
	static CString		getOptionValue(OptionID, OptionValue);

private:
	CCellMap			m_map;
	CNameMap			m_nameToCanonicalName;
	CNetworkAddress		m_synergyAddress;
	CScreenOptions		m_globalOptions;
	CInputFilter		m_inputFilter;
	bool				m_hasLockToScreenAction;
};

//! Configuration read context
/*!
Maintains a context when reading a configuration from a stream.
*/
class CConfigReadContext {
public:
	typedef std::vector<CString> ArgList;

	CConfigReadContext(std::istream&, SInt32 firstLine = 1);
	~CConfigReadContext();

	bool			readLine(CString&);
	UInt32			getLineNumber() const;

	operator void*() const;
	bool			operator!() const;

	OptionValue		parseBoolean(const CString&) const;
	OptionValue		parseInt(const CString&) const;
	OptionValue		parseModifierKey(const CString&) const;
	OptionValue		parseCorner(const CString&) const;
	OptionValue		parseCorners(const CString&) const;
	CConfig::CInterval
					parseInterval(const ArgList& args) const;
	void			parseNameWithArgs(
						const CString& type, const CString& line,
						const CString& delim, CString::size_type& index,
						CString& name, ArgList& args) const;
	IPlatformScreen::CKeyInfo*
					parseKeystroke(const CString& keystroke) const;
	IPlatformScreen::CKeyInfo*
					parseKeystroke(const CString& keystroke,
						const std::set<CString>& screens) const;
	IPlatformScreen::CButtonInfo*
					parseMouse(const CString& mouse) const;
	KeyModifierMask	parseModifier(const CString& modifiers) const;

private:
	// not implemented
	CConfigReadContext&	operator=(const CConfigReadContext&);

	static CString	concatArgs(const ArgList& args);

private:
	std::istream&	m_stream;
	SInt32			m_line;
};

//! Configuration stream read exception
/*!
Thrown when a configuration stream cannot be parsed.
*/
class XConfigRead : public XBase {
public:
	XConfigRead(const CConfigReadContext& context, const CString&);
	XConfigRead(const CConfigReadContext& context,
							const char* errorFmt, const CString& arg);
	~XConfigRead();

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();

private:
	CString				m_error;
};

#endif
