#include "CClient.h"
#include "CString.h"
#include "TMethodJob.h"
#include "IScreen.h"
#include "ISocket.h"
#include "CMessageSocket.h"
#include "CSocketFactory.h"
#include "IEventQueue.h"
#include "CEvent.h"
#include "CTrace.h"
#include <assert.h>

//
// CClient
//

CClient::CClient(IScreen* screen) : m_screen(screen),
								m_socket(NULL)
{
	assert(m_screen != NULL);
	assert(!m_screen->getName().empty());
}

CClient::~CClient()
{
	assert(m_socket == NULL);
}

void					CClient::run(const CString& hostname)
{
	assert(m_socket == NULL);

	try {
		// create socket and
		m_socket = CSOCKETFACTORY->create();
		m_socket->setWriteJob(new TMethodJob<CClient>(this,
												&CClient::onConnect));
		TRACE(("connecting to %s...", hostname.c_str()));
		m_socket->connect(hostname, 40001); // CProtocol::kDefaultPort

		bool m_done = false;	// FIXME

		IEventQueue* queue = CEQ;
		while (!m_done) {
			// wait for connection, network messages, and events
			queue->wait(-1.0);

			// handle events
			while (!queue->isEmpty()) {
				// get the next event
				CEvent event;
				queue->pop(&event);

				// handle it
				switch (event.m_any.m_type) {
				  case CEventBase::kScreenSize: {
					sendScreenSize();
					break;
				  }

				  case CEventBase::kNull:
				  case CEventBase::kKeyDown:
				  case CEventBase::kKeyRepeat:
				  case CEventBase::kKeyUp:
				  case CEventBase::kMouseDown:
				  case CEventBase::kMouseUp:
				  case CEventBase::kMouseMove:
				  case CEventBase::kMouseWheel:
					// FIXME -- other cases
					break;
				}
			}
		}

		delete m_socket;
		m_socket = NULL;
	}

	catch (...) {
		delete m_socket;
		m_socket = NULL;
		throw;
	}
}

void					CClient::onConnect()
{
	TRACE(("connected"));

	// say hello
	const CString name(m_screen->getName());
	char buf[512];
	memcpy(buf, "SYNERGY\000\001", 9);
	buf[9] = static_cast<char>(name.length());
	memcpy(buf + 10, name.c_str(), name.length());
	m_socket->write(buf, 10 + name.length());

	// handle messages
	m_socket->setWriteJob(NULL);
	m_socket = new CMessageSocket(m_socket);
	m_socket->setReadJob(new TMethodJob<CClient>(this, &CClient::onRead));
}

void					CClient::onRead()
{
	char buf[512];
	SInt32 n = m_socket->read(buf, sizeof(buf));
	if (n == -1) {
		// disconnect
		TRACE(("hangup"));
	}
	else if (n > 0) {
		TRACE(("msg: 0x%02x length %d", buf[0], n));
		switch (buf[0]) {
		  case '\002':
			TRACE(("  open"));

			// open the screen
			m_screen->open(buf[1] != 0);

			// send initial size
			sendScreenSize();
			break;

		  case '\003':
			TRACE(("  close"));
			m_screen->close();
			break;

		  case '\004': {
			const SInt32 x = static_cast<SInt32>(
								(static_cast<UInt32>(buf[1]) << 24) +
								(static_cast<UInt32>(buf[2]) << 16) +
								(static_cast<UInt32>(buf[3]) << 8) +
								(static_cast<UInt32>(buf[4])     ));
			const SInt32 y = static_cast<SInt32>(
								(static_cast<UInt32>(buf[5]) << 24) +
								(static_cast<UInt32>(buf[6]) << 16) +
								(static_cast<UInt32>(buf[7]) << 8) +
								(static_cast<UInt32>(buf[8])     ));
			TRACE(("  enter: %d,%d", x, y));
			m_screen->enterScreen(x, y);
			break;
		  }

		  case '\005':
			TRACE(("  leave"));
			m_screen->leaveScreen();
			break;

		  case '\007': {
			const KeyID k = static_cast<KeyID>(
								(static_cast<UInt32>(buf[1]) << 24) +
								(static_cast<UInt32>(buf[2]) << 16) +
								(static_cast<UInt32>(buf[3]) << 8) +
								(static_cast<UInt32>(buf[4])     ));
			TRACE(("  key down: %d", k));
			m_screen->onKeyDown(k);
			break;
		  }

		  case '\010': {
			const KeyID k = static_cast<KeyID>(
								(static_cast<UInt32>(buf[1]) << 24) +
								(static_cast<UInt32>(buf[2]) << 16) +
								(static_cast<UInt32>(buf[3]) << 8) +
								(static_cast<UInt32>(buf[4])     ));
			const SInt32 n = static_cast<SInt32>(
								(static_cast<UInt32>(buf[5]) << 24) +
								(static_cast<UInt32>(buf[6]) << 16) +
								(static_cast<UInt32>(buf[7]) << 8) +
								(static_cast<UInt32>(buf[8])     ));
			TRACE(("  key repeat: %d x%d", k, n));
			m_screen->onKeyRepeat(k, n);
			break;
		  }

		  case '\011': {
			const KeyID k = static_cast<KeyID>(
								(static_cast<UInt32>(buf[1]) << 24) +
								(static_cast<UInt32>(buf[2]) << 16) +
								(static_cast<UInt32>(buf[3]) << 8) +
								(static_cast<UInt32>(buf[4])     ));
			TRACE(("  key up: %d", k));
			m_screen->onKeyUp(k);
			break;
		  }

		  case '\012': {
			const KeyToggleMask m = static_cast<KeyToggleMask>(
								(static_cast<UInt32>(buf[1]) << 8) +
								(static_cast<UInt32>(buf[2])     ));
			TRACE(("  key toggle: 0x%04x", m));
			m_screen->onKeyToggle(m);
			break;
		  }

		  case '\013': {
			const ButtonID b = static_cast<ButtonID>(
								static_cast<UInt32>(buf[1]));
			TRACE(("  mouse down: %d", b));
			m_screen->onMouseDown(b);
			break;
		  }

		  case '\014': {
			const ButtonID b = static_cast<ButtonID>(
								static_cast<UInt32>(buf[1]));
			TRACE(("  mouse up: %d", b));
			m_screen->onMouseUp(b);
			break;
		  }

		  case '\015': {
			const SInt32 x = static_cast<SInt32>(
								(static_cast<UInt32>(buf[1]) << 24) +
								(static_cast<UInt32>(buf[2]) << 16) +
								(static_cast<UInt32>(buf[3]) << 8) +
								(static_cast<UInt32>(buf[4])     ));
			const SInt32 y = static_cast<SInt32>(
								(static_cast<UInt32>(buf[5]) << 24) +
								(static_cast<UInt32>(buf[6]) << 16) +
								(static_cast<UInt32>(buf[7]) << 8) +
								(static_cast<UInt32>(buf[8])     ));
			TRACE(("  mouse move: %d,%d", x, y));
			m_screen->onMouseMove(x, y);
			break;
		  }

		  case '\016': {
			const SInt32 n = static_cast<SInt32>(
								(static_cast<UInt32>(buf[1]) << 24) +
								(static_cast<UInt32>(buf[2]) << 16) +
								(static_cast<UInt32>(buf[3]) << 8) +
								(static_cast<UInt32>(buf[4])     ));
			TRACE(("  mouse wheel: %d", n));
			m_screen->onMouseWheel(n);
			break;
		  }

		  case '\017': {
			TRACE(("  screen saver: %s", buf[1] ? "on" : "off"));
			m_screen->onScreenSaver(buf[1] != 0);
			break;
		  }

		  case '\020': {
			const SInt32 x = static_cast<SInt32>(
								(static_cast<UInt32>(buf[1]) << 24) +
								(static_cast<UInt32>(buf[2]) << 16) +
								(static_cast<UInt32>(buf[3]) << 8) +
								(static_cast<UInt32>(buf[4])     ));
			const SInt32 y = static_cast<SInt32>(
								(static_cast<UInt32>(buf[5]) << 24) +
								(static_cast<UInt32>(buf[6]) << 16) +
								(static_cast<UInt32>(buf[7]) << 8) +
								(static_cast<UInt32>(buf[8])     ));
			TRACE(("  warp: %d,%d", x, y));
			m_screen->warpCursor(x, y);
			break;
		  }

		  default:
			TRACE(("  unknown message"));
		}
	}
}

void					CClient::sendScreenSize()
{
	// get the size
	SInt32 w, h;
	m_screen->getSize(&w, &h);

	// send it
	char buf[9];
	memcpy(buf, "\201", 1);
	buf[1] = static_cast<char>((w >> 24) & 0xff);
	buf[2] = static_cast<char>((w >> 16) & 0xff);
	buf[3] = static_cast<char>((w >> 8) & 0xff);
	buf[4] = static_cast<char>(w & 0xff);
	buf[5] = static_cast<char>((h >> 24) & 0xff);
	buf[6] = static_cast<char>((h >> 16) & 0xff);
	buf[7] = static_cast<char>((h >> 8) & 0xff);
	buf[8] = static_cast<char>(h & 0xff);
	m_socket->write(buf, sizeof(buf));
}
