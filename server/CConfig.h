#ifndef CCONFIG_H
#define CCONFIG_H

#include "BasicTypes.h"
#include "CString.h"
#include "XBase.h"
#include <iosfwd>
#include "stdmap.h"

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
public:
	enum EDirection { kLeft, kRight, kTop, kBottom,
						kFirstDirection = kLeft, kLastDirection = kBottom };
	enum EDirectionMask { kLeftMask = 1, kRightMask = 2,
							kTopMask = 4, kBottomMask = 8 };
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
	// comparing names.

	// add/remove screens
	void				addScreen(const CString& name);
	void				removeScreen(const CString& name);
	void				removeAllScreens();

	// connect edges
	void				connect(const CString& srcName,
								EDirection srcSide,
								const CString& dstName);
	void				disconnect(const CString& srcName,
								EDirection srcSide);

	// accessors

	// returns true iff the given name is a valid screen name.
	bool				isValidScreenName(const CString&) const;

	// iterators over screen names
	const_iterator		begin() const;
	const_iterator		end() const;

	// returns true iff name names a screen
	bool				isScreen(const CString& name) const;

	// get the neighbor in the given direction.  returns the empty string
	// if there is no neighbor in that direction.
	CString				getNeighbor(const CString&, EDirection) const;

	// read/write a configuration.  operator>> will throw XConfigRead
	// on error.
	friend std::istream&	operator>>(std::istream&, CConfig&);
	friend std::ostream&	operator<<(std::ostream&, const CConfig&);

	// get the name of a direction (for debugging)
	static const char*	dirName(EDirection);

private:
	static bool			readLine(std::istream&, CString&);
	void				readSection(std::istream&);
	void				readSectionScreens(std::istream&);
	void				readSectionLinks(std::istream&);

private:
	CCellMap			m_map;
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
