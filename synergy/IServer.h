#ifndef ISERVER_H
#define ISERVER_H

#include "IInterface.h"
#include "ClipboardTypes.h"
#include "CString.h"

class CClientInfo;

// the server interface.  this provides all the methods necessary for
// clients to communicate with the server.  note that the methods
// in this interface are similar to the methods in IScreenReceiver but
// include extra parameters.  this interface is suitable for server-side
// client proxies.  client-side objects should use the IScreenReceiver
// interface since the extra parameters are meaningless on the client-side.
class IServer : public IInterface {
public:
	// manipulators

	// notify of client info change
	virtual void		onInfoChanged(const CString& clientName,
							const CClientInfo&) = 0;

	// notify of clipboard grab.  returns true if the grab was honored,
	// false otherwise.
	virtual bool		onGrabClipboard(const CString& clientName,
							ClipboardID, UInt32 seqNum) = 0;

	// notify of new clipboard data
	virtual void		onClipboardChanged(ClipboardID,
							UInt32 seqNum, const CString& data) = 0;
};

#endif
