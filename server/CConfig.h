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

	// manipulators

	// note that case is preserved in screen names but is ignored when
	// comparing names.  screen names and their aliases share a
	// namespace and must be unique.

	// add/remove screens.  addScreen() returns false if the name
	// already exists.  the remove methods automatically remove
	// aliases for the named screen and disconnect any connections
	// to the removed screen(s).
	bool				addScreen(const CString& name);
	void				removeScreen(const CString& name);
	void				removeAllScreens();

	// add/remove alias for a screen name.  an alias can be used
	// any place the canonical screen name can (except addScreen).
	// addAlias() returns false if the alias name already exists
	// or the canonical name is unknown.  removeAlias() fails if
	// the alias is unknown or a canonical name.
	bool				addAlias(const CString& canonical,
							const CString& alias);
	bool				removeAlias(const CString& alias);
	void				removeAllAliases();

	// connect/disconnect edges.  both return false if srcName is
	// unknown.
	bool				connect(const CString& srcName,
							EDirection srcSide,
							const CString& dstName);
	bool				disconnect(const CString& srcName,
							EDirection srcSide);

	// set the synergy and http listen addresses.  there are no
	// default addresses.
	void				setSynergyAddress(const CNetworkAddress&);
	void				setHTTPAddress(const CNetworkAddress&);

	// accessors

	// returns true iff the given name is a valid screen name.
	bool				isValidScreenName(const CString&) const;

	// iterators over (canonical) screen names
	const_iterator		begin() const;
	const_iterator		end() const;

	// returns true iff name names a screen
	bool				isScreen(const CString& name) const;

	// returns true iff name is the canonical name of a screen
	bool				isCanonicalName(const CString& name) const;

	// returns the canonical name of a screen or the empty string if
	// the name is unknown.  returns the canonical name if one is given.
	CString				getCanonicalName(const CString& name) const;

	// get the neighbor in the given direction.  returns the empty string
	// if there is no neighbor in that direction.  returns the canonical
	// screen name.
	CString				getNeighbor(const CString&, EDirection) const;

	// get the listen addresses
	const CNetworkAddress&	getSynergyAddress() const;
	const CNetworkAddress&	getHTTPAddress() const;

	// read/write a configuration.  operator>> will throw XConfigRead
	// on error.
	friend std::istream&	operator>>(std::istream&, CConfig&);
	friend std::ostream&	operator<<(std::ostream&, const CConfig&);

	// get the name of a direction (for debugging)
	static const char*	dirName(EDirection);

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
