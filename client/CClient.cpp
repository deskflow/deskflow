#include "CClient.h"
#include "CClipboard.h"
#include "CInputPacketStream.h"
#include "COutputPacketStream.h"
#include "CProtocolUtil.h"
#include "ISecondaryScreen.h"
#include "ProtocolTypes.h"
#include "XScreen.h"
#include "XSynergy.h"
#include "XSocket.h"
#include "CLock.h"
#include "CThread.h"
#include "CTimerThread.h"
#include "XThread.h"
#include "CLog.h"
#include "TMethodJob.h"
#include <memory>

// hack to work around operator=() bug in STL in g++ prior to v3
#if defined(__GNUC__) && (__GNUC__ < 3)
#define assign(_dst, _src, _type)	_dst.reset(_src)
#else
#define assign(_dst, _src, _type)	_dst = std::auto_ptr<_type >(_src)
#endif


//
// CClient
//

CClient::CClient(const CString& clientName) :
	m_name(clientName),
	m_input(NULL),
	m_output(NULL),
	m_screen(NULL),
	m_camp(false),
	m_active(false),
	m_seqNum(0),
	m_ignoreMove(false)
{
	// do nothing
}

CClient::~CClient()
{
	// do nothing
}

void
CClient::camp(bool on)
{
	m_camp = on;
}

bool
CClient::run(const CNetworkAddress& serverAddress)
{
	CThread* thread = NULL;
	try {
		log((CLOG_NOTE "starting client"));

		// connect to secondary screen
		while (m_screen == NULL) {
			try {
				openSecondaryScreen();
			}
			catch (XScreenOpenFailure&) {
				// can't open screen yet.  wait a few seconds to retry.
				log((CLOG_INFO "failed to open screen.  waiting to retry."));
				CThread::sleep(3.0);
			}
		}

		// start server interactions
		m_serverAddress = &serverAddress;
		thread = new CThread(new TMethodJob<CClient>(this, &CClient::runSession));

		// handle events
		log((CLOG_DEBUG "starting event handling"));
		m_screen->run();

		// clean up
		log((CLOG_NOTE "stopping client"));
		thread->cancel();
		void* result = thread->getResult();
		delete thread;
		closeSecondaryScreen();
		return (result != NULL);
	}
	catch (XBase& e) {
		log((CLOG_ERR "client error: %s", e.what()));

		// clean up
		log((CLOG_NOTE "stopping client"));
		if (thread != NULL) {
			thread->cancel();
			thread->wait();
			delete thread;
		}
		closeSecondaryScreen();
		return true;
	}
	catch (XThread&) {
		// clean up
		log((CLOG_NOTE "stopping client"));
		if (thread != NULL) {
			thread->cancel();
			thread->wait();
			delete thread;
		}
		if (m_screen != NULL) {
			closeSecondaryScreen();
		}
		throw;
	}
	catch (...) {
		log((CLOG_DEBUG "unknown client error"));

		// clean up
		log((CLOG_NOTE "stopping client"));
		if (thread != NULL) {
			thread->cancel();
			thread->wait();
			delete thread;
		}
		if (m_screen != NULL) {
			closeSecondaryScreen();
		}
		throw;
	}
}

void
CClient::quit()
{
	m_screen->stop();
}

void
CClient::onClipboardChanged(ClipboardID id)
{
	log((CLOG_DEBUG "sending clipboard %d changed", id));
	CLock lock(&m_mutex);
	if (m_output != NULL) {
		// m_output can be NULL if the screen calls this method
		// before we've gotten around to connecting to the server.
		CProtocolUtil::writef(m_output, kMsgCClipboard, id, m_seqNum);
	}

	// we now own the clipboard and it has not been sent to the server
	m_ownClipboard[id]  = true;
	m_timeClipboard[id] = 0;

	// if we're not the active screen then send the clipboard now,
	// otherwise we'll wait until we leave.
	if (!m_active) {
		// get clipboard
		CClipboard clipboard;
		m_screen->getClipboard(id, &clipboard);

		// save new time
		m_timeClipboard[id] = clipboard.getTime();

		// marshall the data
		CString data = clipboard.marshall();

		// send data
		log((CLOG_DEBUG "sending clipboard %d seqnum=%d, size=%d", id, m_seqNum, data.size()));
		if (m_output != NULL) {
// FIXME -- will we send the clipboard when we connect?
			CProtocolUtil::writef(m_output, kMsgDClipboard, id, m_seqNum, &data);
		}
	}
}

void
CClient::onResolutionChanged()
{
	log((CLOG_DEBUG "resolution changed"));

	CLock lock(&m_mutex);

	// start ignoring mouse movement until we get an acknowledgment
	m_ignoreMove = true;

	// send notification of resolution change
	onQueryInfoNoLock();
}

#include "CTCPSocket.h" // FIXME
void
CClient::runSession(void*)
{
	log((CLOG_DEBUG "starting client \"%s\"", m_name.c_str()));

	std::auto_ptr<IDataSocket> socket;
	std::auto_ptr<IInputStream> input;
	std::auto_ptr<IOutputStream> output;
	try {
		for (;;) {
			try {
				// allow connect this much time to succeed
				// FIXME -- timeout in member
				CTimerThread timer(m_camp ? -1.0 : 30.0);

				// create socket and attempt to connect to server
				log((CLOG_DEBUG1 "connecting to server"));
				assign(socket, new CTCPSocket(), IDataSocket);	// FIXME -- use factory
				socket->connect(*m_serverAddress);
				log((CLOG_INFO "connected to server"));
				break;
			}
			catch (XSocketConnect&) {
				// failed to connect.  if not camping then rethrow.
				if (!m_camp) {
					throw;
				}

				// we're camping.  wait a bit before retrying
				CThread::sleep(5.0);
			}
		}

		// get the input and output streams
		IInputStream*  srcInput  = socket->getInputStream();
		IOutputStream* srcOutput = socket->getOutputStream();

		// attach the encryption layer
		bool own = false;
/* FIXME -- implement ISecurityFactory
		if (m_securityFactory != NULL) {
			input.reset(m_securityFactory->createInputFilter(srcInput, own));
			output.reset(m_securityFactory->createOutputFilter(srcOutput, own));
			srcInput  = input.get();
			srcOutput = output.get();
			own       = true;
		}
*/

		// give handshake some time
		CTimerThread timer(30.0);

		// attach the packetizing filters
		assign(input, new CInputPacketStream(srcInput, own), IInputStream);
		assign(output, new COutputPacketStream(srcOutput, own), IOutputStream);

		// wait for hello from server
		log((CLOG_DEBUG1 "wait for hello"));
		SInt16 major, minor;
		CProtocolUtil::readf(input.get(), "Synergy%2i%2i", &major, &minor);

		// check versions
		log((CLOG_DEBUG1 "got hello version %d.%d", major, minor));
		if (major < kProtocolMajorVersion ||
			(major == kProtocolMajorVersion && minor < kProtocolMinorVersion)) {
			throw XIncompatibleClient(major, minor);
		}

		// say hello back
		log((CLOG_DEBUG1 "say hello version %d.%d", kProtocolMajorVersion, kProtocolMinorVersion));
		CProtocolUtil::writef(output.get(), "Synergy%2i%2i%s",
								kProtocolMajorVersion,
								kProtocolMinorVersion, &m_name);

		// record streams in a more useful place
		CLock lock(&m_mutex);
		m_input  = input.get();
		m_output = output.get();
	}
	catch (XIncompatibleClient& e) {
		log((CLOG_ERR "server has incompatible version %d.%d", e.getMajor(), e.getMinor()));
		m_screen->stop();
		CThread::exit(NULL);
	}
	catch (XThread&) {
		log((CLOG_ERR "connection timed out"));
		m_screen->stop();
		throw;
	}
	catch (XBase& e) {
		log((CLOG_ERR "connection failed: %s", e.what()));
		m_screen->stop();
		CThread::exit(NULL);
	}
	catch (...) {
		log((CLOG_ERR "connection failed: <unknown error>"));
		m_screen->stop();
		CThread::exit(NULL);
	}

	bool fail = false;
	try {
		// handle messages from server
		for (;;) {
			// wait for reply
			log((CLOG_DEBUG2 "waiting for message"));
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
			log((CLOG_DEBUG2 "msg from server: %c%c%c%c", code[0], code[1], code[2], code[3]));
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
			else if (memcmp(code, kMsgCInfoAck, 4) == 0) {
				onInfoAcknowledgment();
			}
			else if (memcmp(code, kMsgDClipboard, 4) == 0) {
				onSetClipboard();
			}
			else if (memcmp(code, kMsgCClose, 4) == 0) {
				// server wants us to hangup
				log((CLOG_DEBUG1 "recv close"));
				break;
			}
			else if (memcmp(code, kMsgEIncompatible, 4) == 0) {
				onErrorIncompatible();
				fail = true;
				break;
			}
			else if (memcmp(code, kMsgEBusy, 4) == 0) {
				onErrorBusy();
				fail = true;
				break;
			}
			else if (memcmp(code, kMsgEUnknown, 4) == 0) {
				onErrorUnknown();
				fail = true;
				break;
			}
			else if (memcmp(code, kMsgEBad, 4) == 0) {
				onErrorBad();
				fail = true;
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
		m_screen->stop();
		CThread::exit(reinterpret_cast<void*>(1));
	}

	// done with socket
	log((CLOG_DEBUG "disconnecting from server"));
	socket->close();

	// exit event loop
	m_screen->stop();

	CThread::exit(fail ? NULL : reinterpret_cast<void*>(1));
}

// FIXME -- use factory to create screen
#if defined(CONFIG_PLATFORM_WIN32)
#include "CMSWindowsSecondaryScreen.h"
#elif defined(CONFIG_PLATFORM_UNIX)
#include "CXWindowsSecondaryScreen.h"
#endif
void
CClient::openSecondaryScreen()
{
	assert(m_screen == NULL);

	// not active
	m_active = false;

	// reset last sequence number
	m_seqNum = 0;

	// reset clipboard state
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		m_ownClipboard[id]  = false;
		m_timeClipboard[id] = 0;
	}

	// open screen
	log((CLOG_DEBUG1 "creating secondary screen"));
#if defined(CONFIG_PLATFORM_WIN32)
	m_screen = new CMSWindowsSecondaryScreen;
#elif defined(CONFIG_PLATFORM_UNIX)
	m_screen = new CXWindowsSecondaryScreen;
#endif
	log((CLOG_DEBUG1 "opening secondary screen"));
	m_screen->open(this);
}

void
CClient::closeSecondaryScreen()
{
	assert(m_screen != NULL);

	// close the secondary screen
	try {
		log((CLOG_DEBUG1 "closing secondary screen"));
		m_screen->close();
	}
	catch (...) {
		// ignore
	}

	// clean up
	log((CLOG_DEBUG1 "destroying secondary screen"));
	delete m_screen;
	m_screen = NULL;
}

void
CClient::onEnter()
{
	SInt16 x, y;
	UInt16 mask;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgCEnter + 4, &x, &y, &m_seqNum, &mask);
		m_active = true;
	}
	log((CLOG_DEBUG1 "recv enter, %d,%d %d %04x", x, y, m_seqNum, mask));
	m_screen->enter(x, y, static_cast<KeyModifierMask>(mask));
}

void
CClient::onLeave()
{
	log((CLOG_DEBUG1 "recv leave"));

	// tell screen we're leaving
	m_screen->leave();

	// no longer the active screen
	CLock lock(&m_mutex);
	m_active = false;

	// send clipboards that we own and that have changed
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		if (m_ownClipboard[id]) {
			// get clipboard data.  set the clipboard time to the last
			// clipboard time before getting the data from the screen
			// as the screen may detect an unchanged clipboard and
			// avoid copying the data.
			CClipboard clipboard;
			if (clipboard.open(m_timeClipboard[id]))
				clipboard.close();
			m_screen->getClipboard(id, &clipboard);

			// check time
			if (m_timeClipboard[id] == 0 ||
				clipboard.getTime() != m_timeClipboard[id]) {
				// save new time
				m_timeClipboard[id] = clipboard.getTime();

				// marshall the data
				CString data = clipboard.marshall();

				// save and send data if different
				if (data != m_dataClipboard[id]) {
					log((CLOG_DEBUG "sending clipboard %d seqnum=%d, size=%d", id, m_seqNum, data.size()));
					m_dataClipboard[id] = data;
					CProtocolUtil::writef(m_output,
								kMsgDClipboard, id, m_seqNum, &data);
				}
			}
		}
	}
}

void
CClient::onGrabClipboard()
{
	ClipboardID id;
	UInt32 seqNum;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgCClipboard + 4, &id, &seqNum);
		log((CLOG_DEBUG "recv grab clipboard %d", id));

		// validate
		if (id >= kClipboardEnd) {
			return;
		}

		// we no longer own the clipboard
		m_ownClipboard[id] = false;
	}
	m_screen->grabClipboard(id);
}

void
CClient::onScreenSaver()
{
	SInt8 on;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgCScreenSaver + 4, &on);
	}
	log((CLOG_DEBUG1 "recv screen saver on=%d", on));
	// FIXME
}

void
CClient::onQueryInfo()
{
	CLock lock(&m_mutex);
	onQueryInfoNoLock();
}

void
CClient::onQueryInfoNoLock()
{
	SInt32 x, y, w, h;
	m_screen->getMousePos(&x, &y);
	m_screen->getSize(&w, &h);
	SInt32 zoneSize = m_screen->getJumpZoneSize();

	log((CLOG_DEBUG1 "sending info size=%d,%d zone=%d pos=%d,%d", w, h, zoneSize, x, y));
	CProtocolUtil::writef(m_output, kMsgDInfo, w, h, zoneSize, x, y);
}

void
CClient::onInfoAcknowledgment()
{
	log((CLOG_DEBUG1 "recv info acknowledgment"));
	CLock lock(&m_mutex);
	m_ignoreMove = false;
}

void
CClient::onSetClipboard()
{
	ClipboardID id;
	CString data;
	{
		// parse message
		UInt32 seqNum;
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgDClipboard + 4, &id, &seqNum, &data);
	}
	log((CLOG_DEBUG "recv clipboard %d size=%d", id, data.size()));

	// validate
	if (id >= kClipboardEnd) {
		return;
	}

	// unmarshall
	CClipboard clipboard;
	clipboard.unmarshall(data, 0);

	// set screen's clipboard
	m_screen->setClipboard(id, &clipboard);
}

void
CClient::onKeyDown()
{
	UInt16 id, mask;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgDKeyDown + 4, &id, &mask);
	}
	log((CLOG_DEBUG1 "recv key down id=%d, mask=0x%04x", id, mask));
	m_screen->keyDown(static_cast<KeyID>(id),
								static_cast<KeyModifierMask>(mask));
}

void
CClient::onKeyRepeat()
{
	UInt16 id, mask, count;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgDKeyRepeat + 4, &id, &mask, &count);
	}
	log((CLOG_DEBUG1 "recv key repeat id=%d, mask=0x%04x, count=%d", id, mask, count));
	m_screen->keyRepeat(static_cast<KeyID>(id),
								static_cast<KeyModifierMask>(mask),
								count);
}

void
CClient::onKeyUp()
{
	UInt16 id, mask;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgDKeyUp + 4, &id, &mask);
	}
	log((CLOG_DEBUG1 "recv key up id=%d, mask=0x%04x", id, mask));
	m_screen->keyUp(static_cast<KeyID>(id),
								static_cast<KeyModifierMask>(mask));
}

void
CClient::onMouseDown()
{
	SInt8 id;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgDMouseDown + 4, &id);
	}
	log((CLOG_DEBUG1 "recv mouse down id=%d", id));
	m_screen->mouseDown(static_cast<ButtonID>(id));
}

void
CClient::onMouseUp()
{
	SInt8 id;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgDMouseUp + 4, &id);
	}
	log((CLOG_DEBUG1 "recv mouse up id=%d", id));
	m_screen->mouseUp(static_cast<ButtonID>(id));
}

void
CClient::onMouseMove()
{
	bool ignore;
	SInt16 x, y;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgDMouseMove + 4, &x, &y);
		ignore = m_ignoreMove;
	}
	log((CLOG_DEBUG2 "recv mouse move %d,%d", x, y));
	if (!ignore) {
		m_screen->mouseMove(x, y);
	}
}

void
CClient::onMouseWheel()
{
	SInt16 delta;
	{
		CLock lock(&m_mutex);
		CProtocolUtil::readf(m_input, kMsgDMouseWheel + 4, &delta);
	}
	log((CLOG_DEBUG2 "recv mouse wheel %+d", delta));
	m_screen->mouseWheel(delta);
}

void
CClient::onErrorIncompatible()
{
	SInt32 major, minor;
	CLock lock(&m_mutex);
	CProtocolUtil::readf(m_input, kMsgEIncompatible + 4, &major, &minor);
	log((CLOG_ERR "server has incompatible version %d.%d", major, minor));
}

void
CClient::onErrorBusy()
{
	log((CLOG_ERR "server already has a connected client with name \"%s\"", m_name.c_str()));
}

void
CClient::onErrorUnknown()
{
	log((CLOG_ERR "server refused client with name \"%s\"", m_name.c_str()));
}

void
CClient::onErrorBad()
{
	log((CLOG_ERR "server disconnected due to a protocol error"));
}
