#ifndef IMSWINDOWSSCREENEVENTHANDLER_H
#define IMSWINDOWSSCREENEVENTHANDLER_H

#include "IScreenEventHandler.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class IMSWindowsScreenEventHandler : public IScreenEventHandler {
public:
	// manipulators

	// called after the window is created
	virtual void		postCreateWindow(HWND) = 0;

	// called before the window is destroyed
	virtual void		preDestroyWindow(HWND) = 0;

	// IScreenEventHandler overrides
	virtual void		onError() = 0;
	virtual void		onScreensaver(bool activated) = 0;
	virtual bool		onPreDispatch(const CEvent* event) = 0;
	virtual bool		onEvent(CEvent* event) = 0;
	virtual SInt32		getJumpZoneSize() const = 0;
};

#endif
