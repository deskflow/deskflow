#include "CServerProtocol1_0.h"
#include "CServer.h"
#include "CProtocolUtil.h"
#include "ProtocolTypes.h"
#include "IInputStream.h"
#include "CLog.h"
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

void					CServerProtocol1_0::run()
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
		log((CLOG_DEBUG "msg from \"%s\": %c%c%c%c", getClient().c_str(), code[0], code[1], code[2], code[3]));
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

void					CServerProtocol1_0::queryInfo()
{
	log((CLOG_INFO "querying client \"%s\" info", getClient().c_str()));

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

void					CServerProtocol1_0::sendClose()
{
	log((CLOG_INFO "send close to \"%s\"", getClient().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCClose);
}

void					CServerProtocol1_0::sendEnter(
								SInt32 xAbs, SInt32 yAbs)
{
	log((CLOG_INFO "send enter to \"%s\", %d,%d", getClient().c_str(), xAbs, yAbs));
	CProtocolUtil::writef(getOutputStream(), kMsgCEnter, xAbs, yAbs);
}

void					CServerProtocol1_0::sendLeave()
{
	log((CLOG_INFO "send leave to \"%s\"", getClient().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCLeave);
}

void					CServerProtocol1_0::sendGrabClipboard()
{
	log((CLOG_INFO "send grab clipboard to \"%s\"", getClient().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCClipboard);
}

void					CServerProtocol1_0::sendQueryClipboard()
{
	log((CLOG_INFO "query clipboard to \"%s\"", getClient().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgQClipboard);
}

void					CServerProtocol1_0::sendScreenSaver(bool on)
{
	log((CLOG_INFO "send screen saver to \"%s\"", getClient().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCScreenSaver, on ? 1 : 0);
}

void					CServerProtocol1_0::sendKeyDown(
								KeyID key, KeyModifierMask mask)
{
	log((CLOG_INFO "send key down to \"%s\" id=%d, mask=0x%04x", getClient().c_str(), key, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyDown, key, mask);
}

void					CServerProtocol1_0::sendKeyRepeat(
								KeyID key, KeyModifierMask mask)
{
	log((CLOG_INFO "send key repeat to \"%s\" id=%d, mask=0x%04x", getClient().c_str(), key, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyRepeat, key, mask);
}

void					CServerProtocol1_0::sendKeyUp(
								KeyID key, KeyModifierMask mask)
{
	log((CLOG_INFO "send key up to \"%s\" id=%d, mask=0x%04x", getClient().c_str(), key, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyUp, key, mask);
}

void					CServerProtocol1_0::sendMouseDown(
								ButtonID button)
{
	log((CLOG_INFO "send mouse down to \"%s\" id=%d", getClient().c_str(), button));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseDown, button);
}

void					CServerProtocol1_0::sendMouseUp(
								ButtonID button)
{
	log((CLOG_INFO "send mouse up to \"%s\" id=%d", getClient().c_str(), button));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseUp, button);
}

void					CServerProtocol1_0::sendMouseMove(
								SInt32 xAbs, SInt32 yAbs)
{
	log((CLOG_INFO "send mouse move to \"%s\" %d,%d", getClient().c_str(), xAbs, yAbs));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseMove, xAbs, yAbs);
}

void					CServerProtocol1_0::sendMouseWheel(
								SInt32 delta)
{
	log((CLOG_INFO "send mouse wheel to \"%s\" %+d", getClient().c_str(), delta));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseWheel, delta);
}

void					CServerProtocol1_0::recvInfo()
{
	// parse the message
	SInt32 w, h, zoneInfo;
	CProtocolUtil::readf(getInputStream(), kMsgDInfo + 4, &w, &h, &zoneInfo);
	log((CLOG_INFO "received client \"%s\" info size=%dx%d, zone=%d", getClient().c_str(), w, h, zoneInfo));

	// validate
	if (w == 0 || h == 0) {
		throw XBadClient();
	}

	// tell server of change
	getServer()->setInfo(getClient(), w, h, zoneInfo);
}

