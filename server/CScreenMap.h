#ifndef CSCREENMAP_H
#define CSCREENMAP_H

#include "BasicTypes.h"
#include "CString.h"
#include <map>

class CScreenMap {
  public:
	enum EDirection { kLeft, kRight, kTop, kBottom,
						kFirstDirection = kLeft, kLastDirection = kBottom };
	enum EDirectionMask { kLeftMask = 1, kRightMask = 2,
							kTopMask = 4, kBottomMask = 8 };

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
	CString				getNeighbor(const CString&, EDirection) const;

	// get the name of a direction (for debugging)
	static const char*	dirName(EDirection);

  private:
	class CCell {
	  public:
		CString			m_neighbor[kLastDirection - kFirstDirection + 1];
	};
	typedef std::map<CString, CCell> CCellMap;

	CCellMap			m_map;
};

#endif
