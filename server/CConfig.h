#ifndef CCONFIG_H
#define CCONFIG_H

#include "BasicTypes.h"
#include "CString.h"
#include <map>

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
	typedef std::map<CString, CCell> CCellMap;

public:
	typedef CCellMap::const_iterator internal_const_iterator;
	class const_iterator : public std::iterator<
								std::bidirectional_iterator_tag,
								CString, ptrdiff_t, CString*, CString&> {
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
		CConfig::internal_const_iterator	m_i;
	};

	CConfig();
	virtual ~CConfig();

	// manipulators

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

	// iterators over screen names
	const_iterator		begin() const;
	const_iterator		end() const;

	// get the neighbor in the given direction.  returns the empty string
	// if there is no neighbor in that direction.
	CString				getNeighbor(const CString&, EDirection) const;

	// get the name of a direction (for debugging)
	static const char*	dirName(EDirection);

private:
	CCellMap			m_map;
};

#endif
