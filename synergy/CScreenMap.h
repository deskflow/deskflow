#ifndef CSCREENMAP_H
#define CSCREENMAP_H

#include "BasicTypes.h"
#include "CString.h"
#include <map>

class CScreenMap {
  public:
	enum EDirection { kLeft, kRight, kTop, kBottom,
						kFirstDirection = kLeft, kLastDirection = kBottom };

	CScreenMap();
	virtual ~CScreenMap();

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

	// get the neighbor in the given direction.  returns the empty string
	// if there is no neighbor in that direction.
	CString				getNeighbor(const CString&, EDirection) const throw();

  private:
	class CCell {
	  public:
		CString			m_neighbor[kLastDirection - kFirstDirection + 1];
	};
	typedef std::map<CString, CCell> CCellMap;

	CCellMap			m_map;
};

#endif
