#include "CClient.h"
#include "CInputPacketStream.h"
#include "COutputPacketStream.h"
#include "CProtocolUtil.h"
#include "ISecondaryScreen.h"
#include "ProtocolTypes.h"
#include "CThread.h"
#include "CTimerThread.h"
#include "XSynergy.h"
#include "TMethodJob.h"
#include "CLog.h"
#include <memory>

//
// CClient
//

CClient::CClient(const CString& clientName) :
								m_name(clientName),
								m_input(NULL),
								m_output(NULL),
								m_screen(NULL)
{
	// do nothing
}

CClient::~CClient()
{
	// do nothing
}

void					CClient::run(const CNetworkAddress& serverAddress)
{
	m_serverAddress = &serverAddress;
	CThread thread(new TMethodJob<CClient>(this, &CClient::runSession));
	thread.wait();
}

#include "CTCPSocket.h"
#include "CXWindowsSecondaryScreen.h"
void					CClient::runSession(void*)
{
	log((CLOG_DEBUG "starting client \"%s\"", m_name.c_str()));

	std::auto_ptr<ISocket> socket;
	std::auto_ptr<IInputStream> input;
	std::auto_ptr<IOutputStream> output;
	try {
		// allow connect this much time to succeed
		CTimerThread timer(30.0);		// FIXME -- timeout in member

		// create socket and attempt to connect to server
		log((CLOG_DEBUG "connecting to server"));
		socket.reset(new CTCPSocket());	// FIXME -- use factory
		socket->connect(*m_serverAddress);
		log((CLOG_INFO "connected to server"));

		// get the input and output streams
		IInputStream*  srcInput  = socket->getInputStream();
		IOutputStream* srcOutput = socket->getOutputStream();

		// attach the encryption layer
/* FIXME -- implement ISecurityFactory
		if (m_securityFactory != NULL) {
			input.reset(m_securityFactory->createInputFilter(srcInput, false));
			output.reset(m_securityFactory->createOutputFilter(srcOutput, false));
			srcInput  = input.get();
			srcOutput = output.get();
		}
*/

		// attach the packetizing filters
		input.reset(new CInputPacketStream(srcInput, true));
		output.reset(new COutputPacketStream(srcOutput, true));

		// wait for hello from server
		log((CLOG_DEBUG "wait for hello"));
		SInt32 major, minor;
		CProtocolUtil::readf(input.get(), "Synergy%2i%2i", &major, &minor);

		// check versions
		log((CLOG_DEBUG "got hello version %d.%d", major, minor));
		if (major < kMajorVersion ||
			(major == kMajorVersion && minor < kMinorVersion)) {
			throw XIncompatibleClient(major, minor);
		}

		// say hello back
		log((CLOG_DEBUG "say hello version %d.%d", kMajorVersion, kMinorVersion));
		CProtocolUtil::writef(output.get(), "Synergy%2i%2i%s",
								kMajorVersion, kMinorVersion,
								m_name.size(), m_name.data());

		// record streams in a more useful place
		m_input  = input.get();
		m_output = output.get();
	}
	catch (XIncompatibleClient& e) {
		log((CLOG_ERR "server has incompatible version %d.%d", e.getMajor(), e.getMinor()));
		return;
	}
	catch (XThread&) {
		log((CLOG_ERR "connection timed out"));
		throw;
	}
	catch (XBase& e) {
		log((CLOG_ERR "connection failed: %s", e.what()));
		return;
	}

	// connect to screen
	std::auto_ptr<CScreenCleaner> screenCleaner;
	try {
		log((CLOG_DEBUG "creating secondary screen"));
		m_screen = new CXWindowsSecondaryScreen;
		screenCleaner.reset(new CScreenCleaner(this, m_screen));
	}
	catch (XBase& e) {
		log((CLOG_ERR "cannot open screen: %s", e.what()));
		return;
	}

	try {
		// handle messages from server
		for (;;) {
			// wait for reply
			log((CLOG_DEBUG "waiting for message"));
			UInt8 code[4];
			UInt32 n = input->read(code, 4);

			// verify we got an entire code
			if (n == 0) {
				log((CLOG_NOTE "server disconnected"));
				// server hungup
				break;
			}
			if (n != 4) {
				// client sent an incomplete message
				log((CLOG_ERR "incomplete message from server"));
				break;
			}

			// parse message
			log((CLOG_DEBUG "msg from server: %c%c%c%c", code[0], code[1], code[2], code[3]));
			if (memcmp(code, kMsgDMouseMove, 4) == 0) {
				onMouseMove();
			}
			else if (memcmp(code, kMsgDMouseWheel, 4) == 0) {
				onMouseWheel();
			}
			else if (memcmp(code, kMsgDKeyDown, 4) == 0) {
				onKeyDown();
			}
			else if (memcmp(code, kMsgDKeyUp, 4) == 0) {
				onKeyUp();
			}
			else if (memcmp(code, kMsgDMouseDown, 4) == 0) {
				onMouseDown();
			}
			else if (memcmp(code, kMsgDMouseUp, 4) == 0) {
				onMouseUp();
			}
			else if (memcmp(code, kMsgDKeyRepeat, 4) == 0) {
				onKeyRepeat();
			}
			else if (memcmp(code, kMsgCEnter, 4) == 0) {
				onEnter();
			}
			else if (memcmp(code, kMsgCLeave, 4) == 0) {
				onLeave();
			}
			else if (memcmp(code, kMsgCClipboard, 4) == 0) {
				onGrabClipboard();
			}
			else if (memcmp(code, kMsgCScreenSaver, 4) == 0) {
				onScreenSaver();
			}
			else if (memcmp(code, kMsgQInfo, 4) == 0) {
				onQueryInfo();
			}
			else if (memcmp(code, kMsgQClipboard, 4) == 0) {
				onQueryClipboard();
			}
			else if (memcmp(code, kMsgDClipboard, 4) == 0) {
				onSetClipboard();
			}
			else if (memcmp(code, kMsgCClose, 4) == 0) {
				// server wants us to hangup
				break;
			}
			else {
				// unknown message
				log((CLOG_ERR "unknown message from server"));
				break;
			}
		}
	}
	catch (XBase& e) {
		log((CLOG_ERR "error: %s", e.what()));
		return;
	}

	// done with screen
	log((CLOG_DEBUG "destroying secondary screen"));
	screenCleaner.reset();

	// done with socket
	log((CLOG_DEBUG "disconnecting from server"));
	socket->close();
}

void					CClient::onEnter()
{
	SInt32 x, y;
	CProtocolUtil::readf(m_input, kMsgCEnter + 4, &x, &y);
	m_screen->enter(x, y);
}

void					CClient::onLeave()
{
	m_screen->leave();
}

void					CClient::onGrabClipboard()
{
	// FIXME
}

void					CClient::onScreenSaver()
{
	SInt32 on;
	CProtocolUtil::readf(m_input, kMsgCScreenSaver + 4, &on);
	// FIXME
}

void					CClient::onQueryInfo()
{
	SInt32 w, h;
	m_screen->getSize(&w, &h);
	SInt32 zoneSize = m_screen->getJumpZoneSize();
	log((CLOG_DEBUG "sending info size=%d,%d zone=%d", w, h, zoneSize));
	CProtocolUtil::writef(m_output, kMsgDInfo, w, h, zoneSize);
}

void					CClient::onQueryClipboard()
{
	// FIXME
}

void					CClient::onSetClipboard()
{
	// FIXME
}

void					CClient::onKeyDown()
{
	SInt32 id, mask;
	CProtocolUtil::readf(m_input, kMsgDKeyDown + 4, &id, &mask);
	m_screen->onKeyDown(static_cast<KeyID>(id),
								static_cast<KeyModifierMask>(mask));
}

void					CClient::onKeyRepeat()
{
	SInt32 id, mask, count;
	CProtocolUtil::readf(m_input, kMsgDKeyRepeat + 4, &id, &mask, &count);
	m_screen->onKeyRepeat(static_cast<KeyID>(id),
								static_cast<KeyModifierMask>(mask),
								count);
}

void					CClient::onKeyUp()
{
	SInt32 id, mask;
	CProtocolUtil::readf(m_input, kMsgDKeyUp + 4, &id, &mask);
	m_screen->onKeyUp(static_cast<KeyID>(id),
								static_cast<KeyModifierMask>(mask));
}

void					CClient::onMouseDown()
{
	SInt32 id;
	CProtocolUtil::readf(m_input, kMsgDMouseDown + 4, &id);
	m_screen->onMouseDown(static_cast<ButtonID>(id));
}

void					CClient::onMouseUp()
{
	SInt32 id;
	CProtocolUtil::readf(m_input, kMsgDMouseUp + 4, &id);
	m_screen->onMouseUp(static_cast<ButtonID>(id));
}

void					CClient::onMouseMove()
{
	SInt32 x, y;
	CProtocolUtil::readf(m_input, kMsgDMouseMove + 4, &x, &y);
	m_screen->onMouseMove(x, y);
}

void					CClient::onMouseWheel()
{
	SInt32 delta;
	CProtocolUtil::readf(m_input, kMsgDMouseWheel + 4, &delta);
	m_screen->onMouseWheel(delta);
}


//
// CClient::CScreenCleaner
//

CClient::CScreenCleaner::CScreenCleaner(CClient* client,
								ISecondaryScreen* screen) :
								m_screen(screen)
{
	assert(m_screen != NULL);
	try {
		m_screen->open(client);
	}
	catch (...) {
		delete m_screen;
		throw;
	}
}

CClient::CScreenCleaner::~CScreenCleaner()
{
	m_screen->close();
	delete m_screen;
}
