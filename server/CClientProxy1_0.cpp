#include "CClientProxy1_0.h"
#include "CServer.h"
#include "CClipboard.h"
#include "CProtocolUtil.h"
#include "XSynergy.h"
#include "IInputStream.h"
#include "IOutputStream.h"
#include "CLock.h"
#include "CThread.h"
#include "CLog.h"
#include "CStopwatch.h"
#include <cstring>

//
// CClientProxy1_0
//

CClientProxy1_0::CClientProxy1_0(IServer* server, const CString& name,
				IInputStream* input, IOutputStream* output) :
	CClientProxy(server, name, input, output)
{
	for (UInt32 i = 0; i < kClipboardEnd; ++i) {
		m_clipboardDirty[i] = true;
	}
}

CClientProxy1_0::~CClientProxy1_0()
{
	// do nothing
}

bool
CClientProxy1_0::open()
{
	// send request
	log((CLOG_DEBUG1 "querying client \"%s\" info", getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgQInfo);
	getOutputStream()->flush();

	// wait for and verify reply
	UInt8 code[4];
	for (;;) {
		UInt32 n = getInputStream()->read(code, 4, -1.0);
		if (n == 4) {
			if (memcmp(code, kMsgCNoop, 4) == 0) {
				// discard heartbeats
				continue;
			}
			if (memcmp(code, kMsgDInfo, 4) == 0) {
				break;
			}
		}
		throw XBadClient();
	}

	// handle reply
	recvInfo(false);

	return true;
}

void
CClientProxy1_0::run()
{
	// handle messages until the client hangs up or stops sending heartbeats
	CStopwatch heartTimer;
	for (;;) {
		CThread::testCancel();

		// wait for a message
		UInt8 code[4];
		UInt32 n = getInputStream()->read(code, 4, kHeartRate);
		CThread::testCancel();

		// check if client hungup
		if (n == 0) {
			log((CLOG_NOTE "client \"%s\" disconnected", getName().c_str()));
			return;
		}

		// check if client has stopped sending heartbeats
		if (n == (UInt32)-1) {
/* FIXME -- disabled to avoid masking bugs
			if (heartTimer.getTime() > kHeartDeath) {
				log((CLOG_NOTE "client \"%s\" is dead", getName().c_str()));
				return;
			}
*/
			continue;
		}

		// got a message so reset heartbeat monitor
		heartTimer.reset();

		// verify we got an entire code
		if (n != 4) {
			log((CLOG_ERR "incomplete message from \"%s\": %d bytes", getName().c_str(), n));

			// client sent an incomplete message
			throw XBadClient();
		}

		// parse message
		log((CLOG_DEBUG2 "msg from \"%s\": %c%c%c%c", getName().c_str(), code[0], code[1], code[2], code[3]));
		if (memcmp(code, kMsgDInfo, 4) == 0) {
			recvInfo(true);
		}
		else if (memcmp(code, kMsgCNoop, 4) == 0) {
			// discard no-ops
			continue;
		}
		else if (memcmp(code, kMsgCClipboard, 4) == 0) {
			recvGrabClipboard();
		}
		else if (memcmp(code, kMsgDClipboard, 4) == 0) {
			recvClipboard();
		}
		// note -- more message handlers go here
		else {
			log((CLOG_ERR "invalid message from client \"%s\"", getName().c_str()));

			// unknown message
			throw XBadClient();
		}
	}
}

void
CClientProxy1_0::close()
{
	log((CLOG_DEBUG1 "send close to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCClose);

	// force the close to be sent before we return
	getOutputStream()->flush();
}

void
CClientProxy1_0::enter(SInt32 xAbs, SInt32 yAbs,
				UInt32 seqNum, KeyModifierMask mask, bool)
{
	log((CLOG_DEBUG1 "send enter to \"%s\", %d,%d %d %04x", getName().c_str(), xAbs, yAbs, seqNum, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgCEnter,
								xAbs, yAbs, seqNum, mask);
}

bool
CClientProxy1_0::leave()
{
	log((CLOG_DEBUG1 "send leave to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCLeave);

	// we can never prevent the user from leaving
	return true;
}

void
CClientProxy1_0::setClipboard(ClipboardID id, const CString& data)
{
	// ignore if this clipboard is already clean
	CLock lock(&m_mutex);
	if (m_clipboardDirty[id]) {
		// this clipboard is now clean
		m_clipboardDirty[id] = false;

		log((CLOG_DEBUG "send clipboard %d to \"%s\" size=%d", id, getName().c_str(), data.size()));
		CProtocolUtil::writef(getOutputStream(), kMsgDClipboard, id, 0, &data);
	}
}

void
CClientProxy1_0::grabClipboard(ClipboardID id)
{
	log((CLOG_DEBUG "send grab clipboard %d to \"%s\"", id, getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCClipboard, id, 0);

	// this clipboard is now dirty
	CLock lock(&m_mutex);
	m_clipboardDirty[id] = true;
}

void
CClientProxy1_0::setClipboardDirty(ClipboardID id, bool dirty)
{
	CLock lock(&m_mutex);
	m_clipboardDirty[id] = dirty;
}

void
CClientProxy1_0::keyDown(KeyID key, KeyModifierMask mask)
{
	log((CLOG_DEBUG1 "send key down to \"%s\" id=%d, mask=0x%04x", getName().c_str(), key, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyDown, key, mask);
}

void
CClientProxy1_0::keyRepeat(KeyID key, KeyModifierMask mask, SInt32 count)
{
	log((CLOG_DEBUG1 "send key repeat to \"%s\" id=%d, mask=0x%04x, count=%d", getName().c_str(), key, mask, count));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyRepeat, key, mask, count);
}

void
CClientProxy1_0::keyUp(KeyID key, KeyModifierMask mask)
{
	log((CLOG_DEBUG1 "send key up to \"%s\" id=%d, mask=0x%04x", getName().c_str(), key, mask));
	CProtocolUtil::writef(getOutputStream(), kMsgDKeyUp, key, mask);
}

void
CClientProxy1_0::mouseDown(ButtonID button)
{
	log((CLOG_DEBUG1 "send mouse down to \"%s\" id=%d", getName().c_str(), button));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseDown, button);
}

void
CClientProxy1_0::mouseUp(ButtonID button)
{
	log((CLOG_DEBUG1 "send mouse up to \"%s\" id=%d", getName().c_str(), button));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseUp, button);
}

void
CClientProxy1_0::mouseMove(SInt32 xAbs, SInt32 yAbs)
{
	log((CLOG_DEBUG2 "send mouse move to \"%s\" %d,%d", getName().c_str(), xAbs, yAbs));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseMove, xAbs, yAbs);
}

void
CClientProxy1_0::mouseWheel(SInt32 delta)
{
	log((CLOG_DEBUG2 "send mouse wheel to \"%s\" %+d", getName().c_str(), delta));
	CProtocolUtil::writef(getOutputStream(), kMsgDMouseWheel, delta);
}

void
CClientProxy1_0::screenSaver(bool on)
{
	log((CLOG_DEBUG1 "send screen saver to \"%s\" on=%d", getName().c_str(), on ? 1 : 0));
	CProtocolUtil::writef(getOutputStream(), kMsgCScreenSaver, on ? 1 : 0);
}

void
CClientProxy1_0::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	CLock lock(&m_mutex);
	x = m_info.m_x;
	y = m_info.m_y;
	w = m_info.m_w;
	h = m_info.m_h;
}

void
CClientProxy1_0::getCenter(SInt32& x, SInt32& y) const
{
	CLock lock(&m_mutex);
	x = m_info.m_mx;
	y = m_info.m_my;
}

void
CClientProxy1_0::getMousePos(SInt32&, SInt32&) const
{
	assert(0 && "shouldn't be called");
}

SInt32
CClientProxy1_0::getJumpZoneSize() const
{
	CLock lock(&m_mutex);
	return m_info.m_zoneSize;
}

void
CClientProxy1_0::recvInfo(bool notify)
{
	{
		CLock lock(&m_mutex);

		// parse the message
		SInt16 x, y, w, h, zoneSize, mx, my;
		CProtocolUtil::readf(getInputStream(), kMsgDInfo + 4,
								&x, &y, &w, &h, &zoneSize, &mx, &my);
		log((CLOG_DEBUG "received client \"%s\" info shape=%d,%d %dx%d, zone=%d, pos=%d,%d", getName().c_str(), x, y, w, h, zoneSize, mx, my));

		// validate
		if (w <= 0 || h <= 0 || zoneSize < 0) {
			throw XBadClient();
		}
		if (mx < x || my < y || mx >= x + w || my >= y + h) {
			throw XBadClient();
		}

		// save
		m_info.m_x        = x;
		m_info.m_y        = y;
		m_info.m_w        = w;
		m_info.m_h        = h;
		m_info.m_zoneSize = zoneSize;
		m_info.m_mx       = mx;
		m_info.m_my       = my;
	}

	// tell server of change
	if (notify) {
		getServer()->onInfoChanged(getName(), m_info);
	}

	// acknowledge receipt
	log((CLOG_DEBUG1 "send info ack to \"%s\"", getName().c_str()));
	CProtocolUtil::writef(getOutputStream(), kMsgCInfoAck);
}

void
CClientProxy1_0::recvClipboard()
{
	// parse message
	ClipboardID id;
	UInt32 seqNum;
	CString data;
	CProtocolUtil::readf(getInputStream(), kMsgDClipboard + 4, &id, &seqNum, &data);
	log((CLOG_DEBUG "received client \"%s\" clipboard %d seqnum=%d, size=%d", getName().c_str(), id, seqNum, data.size()));

	// validate
	if (id >= kClipboardEnd) {
		throw XBadClient();
	}

	// send update.  this calls us back to reset our clipboard dirty flag
	// so don't hold a lock during the call.
	getServer()->onClipboardChanged(id, seqNum, data);
}

void
CClientProxy1_0::recvGrabClipboard()
{
	// parse message
	ClipboardID id;
	UInt32 seqNum;
	CProtocolUtil::readf(getInputStream(), kMsgCClipboard + 4, &id, &seqNum);
	log((CLOG_DEBUG "received client \"%s\" grabbed clipboard %d seqnum=%d", getName().c_str(), id, seqNum));

	// validate
	if (id >= kClipboardEnd) {
		throw XBadClient();
	}

	// send update.  this calls us back to reset our clipboard dirty flag
	// so don't hold a lock during the call.
	getServer()->onGrabClipboard(getName(), id, seqNum);
}
