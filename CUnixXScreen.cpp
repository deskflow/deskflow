#include "CUnixXScreen.h"
#include "CUnixEventQueue.h"
#include "TMethodJob.h"
#include <X11/X.h>

//
// CUnixXScreen
//

CUnixXScreen::CUnixXScreen(const CString& name) :
								CXScreen(name)
{
	// do nothing
}

CUnixXScreen::~CUnixXScreen()
{
	// do nothing
}

void					CUnixXScreen::onOpen(bool)
{
	// register our X event	handler
	CEQ->addFileDesc(ConnectionNumber(getDisplay()),
								new TMethodJob<CUnixXScreen>(this,
									&CUnixXScreen::onEvents), NULL);

}

void					CUnixXScreen::onClose()
{
	// unregister the X event handler
	CEQ->removeFileDesc(ConnectionNumber(getDisplay()));
}
