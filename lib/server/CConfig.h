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

#include "ProtocolTypes.h"
#include "CNetworkAddress.h"
#include "XBase.h"
#include "stdmap.h"
#include "stdset.h"
#include <iosfwd>

class CConfig;

namespace std {
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
private:
	class CCell {
	public:
		CString			m_neighbor[kLastDirection - kFirstDirection + 1];
	};
	typedef std::map<CString, CCell, CStringUtil::CaselessCmp> CCellMap;

public:
	typedef CCellMap::const_iterator internal_const_iterator;
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

	//! Remove all aliases
	/*!
	This removes all aliases but not the screens.
	*/
	void				removeAllAliases();

	//! Connect screens
	/*!
	Establishes a one-way connection between opposite edges of two
	screens.  The user will be able to jump from the \c srcSide of
	screen \c srcName to the opposite side of screen \c dstName
	when both screens are connected to the server and the user
	isn't locked to a screen.  Returns false if \c srcName is
	unknown.
	*/
	bool				connect(const CString& srcName,
							EDirection srcSide,
							const CString& dstName);

	//! Disconnect screens
	/*!
	Removes a connection created by connect().  Returns false if
	\c srcName is unknown.
	*/
	bool				disconnect(const CString& srcName,
							EDirection srcSide);

	//! Set server address
	/*!
	Set the synergy listen addresses.  There is no default address so
	this must be called to run a server using this configuration.
	*/
	void				setSynergyAddress(const CNetworkAddress&);

	//! Set HTTP server address
	/*!
	Set the HTTP listen addresses.  There is no default address so
	this must be called to run an HTTP server using this configuration.
	*/
	void				setHTTPAddress(const CNetworkAddress&);

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
	direction (set through connect()).  Returns the empty string
	if there is no neighbor in that direction.
	*/
	CString				getNeighbor(const CString&, EDirection) const;

	//! Get the server address
	const CNetworkAddress&	getSynergyAddress() const;
	//! Get the HTTP server address
	const CNetworkAddress&	getHTTPAddress() const;

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

	//@}

private:
	static bool			readLine(std::istream&, CString&);
	void				readSection(std::istream&);
	void				readSectionNetwork(std::istream&);
	void				readSectionScreens(std::istream&);
	void				readSectionLinks(std::istream&);
	void				readSectionAliases(std::istream&);

private:
	typedef std::map<CString, CString, CStringUtil::CaselessCmp> CNameMap;

	CCellMap			m_map;
	CNameMap			m_nameToCanonicalName;
	CNetworkAddress		m_synergyAddress;
	CNetworkAddress		m_httpAddress;
};

//! Configuration stream read exception
/*!
Thrown when a configuration stream cannot be parsed.
*/
class XConfigRead : public XBase {
public:
	XConfigRead(const CString&);
	~XConfigRead();

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();

private:
	CString				m_error;
};

#endif
