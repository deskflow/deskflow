#include "CServerProtocol1_0.h"
#include "CServer.h"
#include "CProtocolUtil.h"
#include "ProtocolTypes.h"
#include "IInputStream.h"
#include <string.h>

//
// CServerProtocol1_0
//

CServerProtocol1_0::CServerProtocol1_0(CServer* server, const CString& client,
								IInputStream* input, IOutputStream* output) :
								CServerProtocol(server, client, input, output)
{
	// do nothing
}

CServerProtocol1_0::~CServerProtocol1_0()
{
	// do nothing
}

void					CServerProtocol1_0::run() throw(XIO,XBadClient)
{
	// handle messages until the client hangs up
	for (;;) {
		// wait for a message
		UInt8 code[4];
		UInt32 n = getInputStream()->read(code, 4);

		// verify we got an entire code
		if (n == 0) {
			// client hungup
			return;
		}
		if (n != 4) {
			// client sent an incomplete message
			throw XBadClient();
		}

		// parse message
		if (memcmp(code, kMsgDInfo, 4) == 0) {
			recvInfo();
		}
		// FIXME -- more message here
		else {
			// unknown message
			throw XBadClient();
		}
	}
}

void					CServerProtocol1_0::queryInfo() throw(XIO,XBadClient)
{
	// send request
	CProtocolUtil::writef(getOutputStream(), kMsgQInfo);

	// wait for and verify reply
	UInt8 code[4];
	UInt32 n = getInputStream()->read(code, 4);
	if (n != 4 && memcmp(code, kMsgDInfo, 4) != 0) {
		throw XBadClient();
	}

	// handle reply
	recvInfo();
}

void					CServerProtocol1_0::sendClose() throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgCClose);
}

void					CServerProtocol1_0::sendEnter(
								SInt32 xAbs, SInt32 yAbs) throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgCEnter, xAbs, yAbs);
}

void					CServerProtocol1_0::sendLeave() throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgCLeave);
}

void					CServerProtocol1_0::sendGrabClipboard() throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgCClipboard);
}

void					CServerProtocol1_0::sendQueryClipboard() throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgQClipboard);
}

void					CServerProtocol1_0::sendScreenSaver(bool on) throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgCScreenSaver, on ? 1 : 0);
}

void					CServerProtocol1_0::sendKeyDown(
								KeyID key, KeyModifierMask mask) throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyDown, key, mask);
}

void					CServerProtocol1_0::sendKeyRepeat(
								KeyID key, KeyModifierMask mask) throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyRepeat, key, mask);
}

void					CServerProtocol1_0::sendKeyUp(
								KeyID key, KeyModifierMask mask) throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyUp, key, mask);
}

void					CServerProtocol1_0::sendMouseDown(
								ButtonID button) throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseDown, button);
}

void					CServerProtocol1_0::sendMouseUp(
								ButtonID button) throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseUp, button);
}

void					CServerProtocol1_0::sendMouseMove(
								SInt32 xAbs, SInt32 yAbs) throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseMove, xAbs, yAbs);
}

void					CServerProtocol1_0::sendMouseWheel(
								SInt32 delta) throw(XIO)
{
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseWheel, delta);
}

void					CServerProtocol1_0::recvInfo() throw(XIO,XBadClient)
{
	// parse the message
	SInt32 w, h, zoneInfo;
	CProtocolUtil::readf(getInputStream(), kMsgDInfo + 4, &w, &h, &zoneInfo);

	// validate
	if (w == 0 || h == 0) {
		throw XBadClient();
	}

	// tell server of change
	getServer()->setInfo(getClient(), w, h, zoneInfo);
}

