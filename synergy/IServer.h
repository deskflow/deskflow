#ifndef ISERVER_H
#define ISERVER_H

#include "IInterface.h"
#include "ClipboardTypes.h"
#include "CString.h"

class CClientInfo;

//! Server interface
/*!
This interface defines the methods necessary for clients to
communicate with the server.  Note that the methods in this
interface are similar to the methods in IScreenReceiver but
include extra parameters.  This interface is suitable for
server-side client proxies.  Client-side objects should use
the IScreenReceiver interface since the extra parameters are
meaningless on the client-side.
*/
class IServer : public IInterface {
public:
	//! @name manipulators
	//@{

	//! Notify of error
	/*!
	Called when the screen is unexpectedly closing.  This implies that
	the screen is no longer usable and that the program should close
	the screen and probably terminate.
	*/
	virtual void		onError() = 0;

	//! Notify of client screen change
	/*!
	Called when the client's info has changed.
	*/
	virtual void		onInfoChanged(const CString& clientName,
							const CClientInfo&) = 0;

	//! Notify of clipboad grab
	/*!
	Called when the clipboard was grabbed by another program and,
	therefore, we no longer own it.  Returns true if the grab was
	honored, false otherwise.
	*/
	virtual bool		onGrabClipboard(const CString& clientName,
							ClipboardID, UInt32 seqNum) = 0;

	//! Notify of new clipboard data
	/*!
	Called when the data on the clipboard has changed because some
	other program has changed it.
	*/
	virtual void		onClipboardChanged(ClipboardID,
							UInt32 seqNum, const CString& data) = 0;

	//@}
};

#endif
