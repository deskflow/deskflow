#include "CServerProtocol1_0.h"
#include "CServer.h"
#include "CClipboard.h"
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
			log((CLOG_NOTE "client \"%s\" disconnected", getClient().c_str()));

			// client hungup
			return;
		}
		if (n != 4) {
			log((CLOG_ERR "incomplete message from \"%s\": %d bytes", getClient().c_str(), n));

			// client sent an incomplete message
			throw XBadClient();
		}

		// parse message
		log((CLOG_DEBUG2 "msg from \"%s\": %c%c%c%c", getClient().c_str(), code[0], code[1], code[2], code[3]));
		if (memcmp(code, kMsgDInfo, 4) == 0) {
			recvInfo();
		}
		else if (memcmp(code, kMsgCClipboard, 4) == 0) {
			recvGrabClipboard();
		}
		else if (memcmp(code, kMsgDClipboard, 4) == 0) {
			recvClipboard();
		}
		// FIXME -- more message here
		else {
			log((CLOG_ERR "invalid message from client \"%s\"", getClient().c_str()));

			// unknown message
			throw XBadClient();
		}
	}
}

void					CServerProtocol1_0::queryInfo()
{
	log((CLOG_DEBUG1 "querying client \"%s\" info", getClient().c_str()));

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
	log((CLOG_DEBUG1 "send close to \"%s\"", getClient().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCClose);
}

void					CServerProtocol1_0::sendEnter(
								SInt32 xAbs, SInt32 yAbs,
								UInt32 seqNum, KeyModifierMask mask)
{
	log((CLOG_DEBUG1 "send enter to \"%s\", %d,%d %d %04x", getClient().c_str(), xAbs, yAbs, seqNum, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgCEnter,
								xAbs, yAbs, seqNum, mask);
}

void					CServerProtocol1_0::sendLeave()
{
	log((CLOG_DEBUG1 "send leave to \"%s\"", getClient().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCLeave);
}

void					CServerProtocol1_0::sendClipboard(
								ClipboardID id, const CString& data)
{
	log((CLOG_DEBUG "send clipboard %d to \"%s\" size=%d", id, getClient().c_str(), data.size()));
	CProtocolUtil::writef(getOutputStream(), kMsgDClipboard, id, 0, &data);
}

void					CServerProtocol1_0::sendGrabClipboard(ClipboardID id)
{
	log((CLOG_DEBUG "send grab clipboard %d to \"%s\"", id, getClient().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCClipboard, id, 0);
}

void					CServerProtocol1_0::sendScreenSaver(bool on)
{
	log((CLOG_DEBUG1 "send screen saver to \"%s\" on=%d", getClient().c_str(), on ? 1 : 0));
	CProtocolUtil::writef(getOutputStream(), kMsgCScreenSaver, on ? 1 : 0);
}

void					CServerProtocol1_0::sendInfoAcknowledgment()
{
	log((CLOG_DEBUG1 "send info ack to \"%s\"", getClient().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCInfoAck);
}

void					CServerProtocol1_0::sendKeyDown(
								KeyID key, KeyModifierMask mask)
{
	log((CLOG_DEBUG1 "send key down to \"%s\" id=%d, mask=0x%04x", getClient().c_str(), key, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyDown, key, mask);
}

void					CServerProtocol1_0::sendKeyRepeat(
								KeyID key, KeyModifierMask mask, SInt32 count)
{
	log((CLOG_DEBUG1 "send key repeat to \"%s\" id=%d, mask=0x%04x, count=%d", getClient().c_str(), key, mask, count));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyRepeat, key, mask, count);
}

void					CServerProtocol1_0::sendKeyUp(
								KeyID key, KeyModifierMask mask)
{
	log((CLOG_DEBUG1 "send key up to \"%s\" id=%d, mask=0x%04x", getClient().c_str(), key, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyUp, key, mask);
}

void					CServerProtocol1_0::sendMouseDown(
								ButtonID button)
{
	log((CLOG_DEBUG1 "send mouse down to \"%s\" id=%d", getClient().c_str(), button));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseDown, button);
}

void					CServerProtocol1_0::sendMouseUp(
								ButtonID button)
{
	log((CLOG_DEBUG1 "send mouse up to \"%s\" id=%d", getClient().c_str(), button));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseUp, button);
}

void					CServerProtocol1_0::sendMouseMove(
								SInt32 xAbs, SInt32 yAbs)
{
	log((CLOG_DEBUG2 "send mouse move to \"%s\" %d,%d", getClient().c_str(), xAbs, yAbs));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseMove, xAbs, yAbs);
}

void					CServerProtocol1_0::sendMouseWheel(
								SInt32 delta)
{
	log((CLOG_DEBUG2 "send mouse wheel to \"%s\" %+d", getClient().c_str(), delta));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseWheel, delta);
}

void					CServerProtocol1_0::recvInfo()
{
	// parse the message
	SInt16 x, y, w, h, zoneInfo;
	CProtocolUtil::readf(getInputStream(), kMsgDInfo + 4,
								&w, &h, &zoneInfo, &x, &y);
	log((CLOG_DEBUG "received client \"%s\" info size=%dx%d, zone=%d, pos=%d,%d", getClient().c_str(), w, h, zoneInfo, x, y));

	// validate
	if (w <= 0 || h <= 0 || zoneInfo < 0) {
		throw XBadClient();
	}
	if (x < 0 || y < 0 || x >= w || y >= h) {
		throw XBadClient();
	}

	// tell server of change
	getServer()->setInfo(getClient(), w, h, zoneInfo, x, y);
}

void					CServerProtocol1_0::recvClipboard()
{
	// parse message
	ClipboardID id;
	UInt32 seqNum;
	CString data;
	CProtocolUtil::readf(getInputStream(), kMsgDClipboard + 4, &id, &seqNum, &data);
	log((CLOG_DEBUG "received client \"%s\" clipboard %d seqnum=%d, size=%d", getClient().c_str(), id, seqNum, data.size()));

	// validate
	if (id >= kClipboardEnd) {
		throw XBadClient();
	}

	// send update
	getServer()->setClipboard(id, seqNum, data);
}

void					CServerProtocol1_0::recvGrabClipboard()
{
	// parse message
	ClipboardID id;
	UInt32 seqNum;
	CProtocolUtil::readf(getInputStream(), kMsgCClipboard + 4, &id, &seqNum);
	log((CLOG_DEBUG "received client \"%s\" grabbed clipboard %d seqnum=%d", getClient().c_str(), id, seqNum));

	// validate
	if (id >= kClipboardEnd) {
		throw XBadClient();
	}

	// send update
	getServer()->grabClipboard(id, seqNum, getClient());
}
