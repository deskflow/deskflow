#ifndef ISCREENEVENTHANDLER_H
#define ISCREENEVENTHANDLER_H

#include "IInterface.h"

// the platform screen should define this
class CEvent;

class IScreen;

// the interface through which IScreen sends notification of events.
// each platform will derive two types from IScreenEventHandler, one
// for handling events on the primary screen and one for the
// secondary screen.  the header file with the IScreen subclass for
// each platform should define the CEvent type, which depends on the
// type of native events for that platform.
class IScreenEventHandler : public IInterface {
public:
	// manipulators

	// called if the screen is unexpectedly closing.  this implies that
	// the screen is no longer usable and that the program should
	// close the screen and possibly terminate.
	virtual void		onError() = 0;

	// called when the screensaver is activated or deactivated
	virtual void		onScreensaver(bool activated) = 0;

	// called for each event before event translation and dispatch.  return
	// true to skip translation and dispatch.  subclasses should call the
	// superclass's version first and return true if it returns true.
	virtual bool		onPreDispatch(const CEvent* event) = 0;

	// called by mainLoop().  iff the event was handled return true and
	// store the result, if any, in m_result, which defaults to zero.
	virtual bool		onEvent(CEvent* event) = 0;
};

#endif
