#ifndef ISCREENRECEIVER_H
#define ISCREENRECEIVER_H

#include "IInterface.h"
#include "ClipboardTypes.h"
#include "ProtocolTypes.h"
#include "CString.h"

// the interface for types that receive screen resize and clipboard
// notifications (indirectly) from the system.
class IScreenReceiver : public IInterface {
public:
	// notify of client info change
	virtual void		onInfoChanged(const CClientInfo&) = 0;

	// notify of clipboard grab.  returns true if the grab was honored,
	// false otherwise.
	virtual bool		onGrabClipboard(ClipboardID) = 0;

	// notify of new clipboard data
	virtual void		onClipboardChanged(ClipboardID,
							const CString& data) = 0;
};

#endif
