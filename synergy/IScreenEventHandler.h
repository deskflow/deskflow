#ifndef ISCREENEVENTHANDLER_H
#define ISCREENEVENTHANDLER_H

#include "IInterface.h"

// the platform screen should define this
class CEvent;

class IScreen;

//! Screen event handler interface
/*!
This is the interface through which IScreen sends notification of events.
Each platform will derive two types from IScreenEventHandler, one
for handling events on the primary screen and one for the
secondary screen.  The header file with the IScreen subclass for
each platform should define the CEvent type, which depends on the
type of native events for that platform.
*/
class IScreenEventHandler : public IInterface {
public:
	//! @name manipulators
	//@{

	//! Notify of screen saver change
	/*!
	Called when the screensaver is activated or deactivated.
	*/
	virtual void		onScreensaver(bool activated) = 0;

	//! Event filtering
	/*!
	Called for each event before event translation and dispatch.  Return
	true to skip translation and dispatch.  Subclasses should call the
	superclass's version first and return true if it returns true.
	*/
	virtual bool		onPreDispatch(const CEvent* event) = 0;

	//! Event handling
	/*!
	Called to handle an event.  Iff the event was handled return true and
	store the result, if any, in event->m_result, which defaults to zero.
	*/
	virtual bool		onEvent(CEvent* event) = 0;

	//@}
	//! @name accessors
	//@{

	//! Get jump zone size
	/*!
	Called to get the jump zone size.
	*/
	virtual SInt32		getJumpZoneSize() const = 0;

	//@}
};

#endif
