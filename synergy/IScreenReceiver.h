#ifndef ISCREENRECEIVER_H
#define ISCREENRECEIVER_H

#include "IInterface.h"
#include "ClipboardTypes.h"
#include "ProtocolTypes.h"
#include "CString.h"

//! Screen event receiver interface
/*!
This interface defines the methods common to most types that receive
events for changes to a screen.  Note that the methods in this
interface are similar to the methods in IServer but have different
parameters.  This interface is suitable for client-side types.
*/
class IScreenReceiver : public IInterface {
public:
	//! Notify of error
	/*!
	Called when the screen is unexpectedly closing.  This implies that
	the screen is no longer usable and that the program should close
	the screen and probably terminate.
	*/
	virtual void		onError() = 0;

	//! Notify of client screen change
	/*!
	Called when the client's info has changed.  For example, when the
	screen resolution has changed.
	*/
	virtual void		onInfoChanged(const CClientInfo&) = 0;

	//! Notify of clipboad grab
	/*!
	Called when the clipboard was grabbed by another program and,
	therefore, we no longer own it.  Returns true if the grab was
	honored, false otherwise.
	*/
	virtual bool		onGrabClipboard(ClipboardID) = 0;

	//! Notify of new clipboard data
	/*!
	Called when the data on the clipboard has changed because some
	other program has changed it.
	*/
	virtual void		onClipboardChanged(ClipboardID,
							const CString& data) = 0;
};

#endif
