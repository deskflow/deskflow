#include "CClient.h"
#include "CInputPacketStream.h"
#include "COutputPacketStream.h"
#include "CProtocolUtil.h"
#include "CTimerThread.h"
#include <stdio.h>

//
// CClient
//

CClient::CClient(const CString& clientName) :
								m_name(clientName)
{
}

CClient::~CClient()
{
}

#include "CTCPSocket.h"
void					CClient::run(const CNetworkAddress& serverAddress)
{
	std::auto_ptr<ISocket> socket;
	std::auto_ptr<IInputStream> input;
	std::auto_ptr<IOutputStream> output;
	try {
		// allow connect this much time to succeed
		CTimerThread timer(30.0);		// FIXME -- timeout in member

		// create socket and attempt to connect to server
		socket.reset(new CTCPSocket());	// FIXME -- use factory
		socket->connect(serverAddress);

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
		SInt32 major, minor;
		CProtocolUtil::readf(input.get(), "Synergy%2i%2i", &major, &minor);

		// check versions
		if (major < kMajorVersion ||
			(major == kMajorVersion && minor < kMinorVersion)) {
			throw XIncompatibleClient(major, minor);
		}

		// say hello back
		CProtocolUtil::writef(output.get(), "Synergy%2i%2i%s",
								kMajorVersion, kMinorVersion,
								m_name.size(), m_name.data());

		// record streams in a more useful place
		m_input  = input.get();
		m_output = output.get();
	}
	catch (XIncompatibleClient& e) {
		fprintf(stderr, "incompatible server version (%d.%d)\n",
								e.getMajor(), e.getMinor());
		return;
	}
	catch (XThread&) {
		fprintf(stderr, "connection timed out\n");
		throw;
	}
	catch (XBase& e) {
		fprintf(stderr, "connection failed: %s\n", e.what());
		return;
	}

	// connect to screen
	// FIXME -- make object that closes and destroys screen in
	// it's d'tor.  screen must not be handling event queue by
	// the time the streams are destroyed.

	// handle messages from server
	try {
		for (;;) {
			// wait for reply
			UInt8 code[4];
			UInt32 n = input->read(code, 4);

			// verify we got an entire code
			if (n == 0) {
				// server hungup
				break;
			}
			if (n != 4) {
				// client sent an incomplete message
				fprintf(stderr, "incomplete message from server\n");
				break;
			}

			// parse message
			if (memcmp(code, kMsgDMouseMove, 4) == 0) {
				onMouseMove(input.get());
			}
			else if (memcmp(code, kMsgDMouseWheel, 4) == 0) {
				onMouseWheel(input.get());
			}
			else if (memcmp(code, kMsgDKeyDown, 4) == 0) {
				onKeyDown(input.get());
			}
			else if (memcmp(code, kMsgDKeyUp, 4) == 0) {
				onKeyUp(input.get());
			}
			else if (memcmp(code, kMsgDMouseDown, 4) == 0) {
				onMouseDown(input.get());
			}
			else if (memcmp(code, kMsgDMouseUp, 4) == 0) {
				onMouseUp(input.get());
			}
			else if (memcmp(code, kMsgDKeyRepeat, 4) == 0) {
				onKeyRepeat(input.get());
			}
			else if (memcmp(code, kMsgCEnter, 4) == 0) {
				onEnter(input.get());
			}
			else if (memcmp(code, kMsgCLeave, 4) == 0) {
				onLeave(input.get());
			}
			else if (memcmp(code, kMsgCClipboard, 4) == 0) {
				onGrabClipboard(input.get());
			}
			else if (memcmp(code, kMsgCScreenSaver, 4) == 0) {
				onScreenSaver(input.get());
			}
			else if (memcmp(code, kMsgQInfo, 4) == 0) {
				onQueryInfo(input.get());
			}
			else if (memcmp(code, kMsgQClipboard, 4) == 0) {
				onQueryClipboard(input.get());
			}
			else if (memcmp(code, kMsgDClipboard, 4) == 0) {
				onSetClipboard(input.get());
			}
			else if (memcmp(code, kMsgCClose, 4) == 0) {
				// server wants us to hangup
				break;
			}
			else {
				// unknown message
				fprintf(stderr, "unknown message from server\n");
				break;
			}
		}

		// done with socket
		m_socket->close();
	}
	catch (XBase& e) {
		fprintf(stderr, "error: %s\n", e.what());
		return;
	}
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
	m_screen->getSize(w, h);
	SInt32 zoneSize = m_screen->getJumpZoneSize();
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
	m_screen->onKeyDown(reinterpret_cast<KeyID>(id),
								reinterpret_cast<KeyModifierMask>(mask));
}

void					CClient::onKeyRepeat()
{
	SInt32 id, mask, count;
	CProtocolUtil::readf(m_input, kMsgDKeyRepeat + 4, &id, &mask, &count);
	m_screen->onKeyRepeat(reinterpret_cast<KeyID>(id),
								reinterpret_cast<KeyModifierMask>(mask),
								count);
}

void					CClient::onKeyUp()
{
	SInt32 id, mask;
	CProtocolUtil::readf(m_input, kMsgDKeyUp + 4, &id, &mask);
	m_screen->onKeyUp(reinterpret_cast<KeyID>(id),
								reinterpret_cast<KeyModifierMask>(mask));
}

void					CClient::onMouseDown()
{
	SInt32 id;
	CProtocolUtil::readf(m_input, kMsgDMouseDown + 4, &id);
	m_screen->onMouseDown(reinterpret_cast<ButonID>(id));
}

void					CClient::onMouseUp()
{
	SInt32 id;
	CProtocolUtil::readf(m_input, kMsgDMouseUp + 4, &id);
	m_screen->onMouseUp(reinterpret_cast<ButonID>(id));
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
