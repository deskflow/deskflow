/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CClient.h"
#include "CServerProxy.h"
#include "CScreen.h"
#include "CClipboard.h"
#include "CPacketStreamFilter.h"
#include "CProtocolUtil.h"
#include "ProtocolTypes.h"
#include "XSynergy.h"
#include "IDataSocket.h"
#include "ISocketFactory.h"
#include "IStreamFilterFactory.h"
#include "CLog.h"
#include "IEventQueue.h"
#include "TMethodEventJob.h"
#include "CArch.h"
#include "IPlatformScreen.h"
#include "CCryptoStream.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CFileChunker.h"
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <stdexcept>

//
// CClient
//

CClient::CClient(IEventQueue* events,
				const CString& name, const CNetworkAddress& address,
				ISocketFactory* socketFactory,
				IStreamFilterFactory* streamFilterFactory,
				CScreen* screen,
				const CCryptoOptions& crypto,
				bool enableDragDrop) :
	m_mock(false),
	m_name(name),
	m_serverAddress(address),
	m_socketFactory(socketFactory),
	m_streamFilterFactory(streamFilterFactory),
	m_screen(screen),
	m_stream(NULL),
	m_timer(NULL),
	m_server(NULL),
	m_ready(false),
	m_active(false),
	m_suspended(false),
	m_connectOnResume(false),
	m_events(events),
	m_cryptoStream(NULL),
	m_crypto(crypto),
	m_sendFileThread(NULL),
	m_writeToDropDirThread(NULL),
	m_enableDragDrop(enableDragDrop)
{
	assert(m_socketFactory != NULL);
	assert(m_screen        != NULL);

	// register suspend/resume event handlers
	m_events->adoptHandler(m_events->forIScreen().suspend(),
							getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleSuspend));
	m_events->adoptHandler(m_events->forIScreen().resume(),
							getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleResume));

	if (m_enableDragDrop) {
		m_events->adoptHandler(m_events->forIScreen().fileChunkSending(),
								this,
								new TMethodEventJob<CClient>(this,
									&CClient::handleFileChunkSending));
		m_events->adoptHandler(m_events->forIScreen().fileRecieveCompleted(),
								this,
								new TMethodEventJob<CClient>(this,
									&CClient::handleFileRecieveCompleted));
	}
}

CClient::~CClient()
{
	if (m_mock) {
		return;
	}

	m_events->removeHandler(m_events->forIScreen().suspend(),
							  getEventTarget());
	m_events->removeHandler(m_events->forIScreen().resume(),
							  getEventTarget());

	cleanupTimer();
	cleanupScreen();
	cleanupConnecting();
	cleanupConnection();
	delete m_socketFactory;
	delete m_streamFilterFactory;
}

void
CClient::connect()
{
	if (m_stream != NULL) {
		return;
	}
	if (m_suspended) {
		m_connectOnResume = true;
		return;
	}

	try {
		// resolve the server hostname.  do this every time we connect
		// in case we couldn't resolve the address earlier or the address
		// has changed (which can happen frequently if this is a laptop
		// being shuttled between various networks).  patch by Brent
		// Priddy.
		m_serverAddress.resolve();
		
		// m_serverAddress will be null if the hostname address is not reolved
		if (m_serverAddress.getAddress() != NULL) {
		  // to help users troubleshoot, show server host name (issue: 60)
		  LOG((CLOG_NOTE "connecting to '%s': %s:%i", 
		  m_serverAddress.getHostname().c_str(),
		  ARCH->addrToString(m_serverAddress.getAddress()).c_str(),
		  m_serverAddress.getPort()));
		}

		// create the socket
		IDataSocket* socket = m_socketFactory->create();

		// filter socket messages, including a packetizing filter
		m_stream = socket;
		if (m_streamFilterFactory != NULL) {
			m_stream = m_streamFilterFactory->create(m_stream, true);
		}
		m_stream = new CPacketStreamFilter(m_events, m_stream, true);

		if (m_crypto.m_mode != kDisabled) {
			m_cryptoStream = new CCryptoStream(
				m_events, m_stream, m_crypto, true);
			m_stream = m_cryptoStream;
		}

		// connect
		LOG((CLOG_DEBUG1 "connecting to server"));
		setupConnecting();
		setupTimer();
		socket->connect(m_serverAddress);
	}
	catch (XBase& e) {
		cleanupTimer();
		cleanupConnecting();
		delete m_stream;
		m_stream = NULL;
		LOG((CLOG_DEBUG1 "connection failed"));
		sendConnectionFailedEvent(e.what());
		return;
	}
}

void
CClient::disconnect(const char* msg)
{
	m_connectOnResume = false;
	cleanupTimer();
	cleanupScreen();
	cleanupConnecting();
	cleanupConnection();
	if (msg != NULL) {
		sendConnectionFailedEvent(msg);
	}
	else {
		sendEvent(m_events->forCClient().disconnected(), NULL);
	}
}

void
CClient::handshakeComplete()
{
	m_ready = true;
	m_screen->enable();
	sendEvent(m_events->forCClient().connected(), NULL);
}

void
CClient::setDecryptIv(const UInt8* iv)
{
	if (m_cryptoStream != NULL) {
		m_cryptoStream->setDecryptIv(iv);
	}
}

bool
CClient::isConnected() const
{
	return (m_server != NULL);
}

bool
CClient::isConnecting() const
{
	return (m_timer != NULL);
}

CNetworkAddress
CClient::getServerAddress() const
{
	return m_serverAddress;
}

void*
CClient::getEventTarget() const
{
	return m_screen->getEventTarget();
}

bool
CClient::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
	return m_screen->getClipboard(id, clipboard);
}

void
CClient::getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h) const
{
	m_screen->getShape(x, y, w, h);
}

void
CClient::getCursorPos(SInt32& x, SInt32& y) const
{
	m_screen->getCursorPos(x, y);
}

void
CClient::enter(SInt32 xAbs, SInt32 yAbs, UInt32, KeyModifierMask mask, bool)
{
	m_active = true;
	m_screen->mouseMove(xAbs, yAbs);
	m_screen->enter(mask);
}

bool
CClient::leave()
{
	m_screen->leave();

	m_active = false;

	// send clipboards that we own and that have changed
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		if (m_ownClipboard[id]) {
			sendClipboard(id);
		}
	}

	return true;
}

void
CClient::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
 	m_screen->setClipboard(id, clipboard);
	m_ownClipboard[id]  = false;
	m_sentClipboard[id] = false;
}

void
CClient::grabClipboard(ClipboardID id)
{
	m_screen->grabClipboard(id);
	m_ownClipboard[id]  = false;
	m_sentClipboard[id] = false;
}

void
CClient::setClipboardDirty(ClipboardID, bool)
{
	assert(0 && "shouldn't be called");
}

void
CClient::keyDown(KeyID id, KeyModifierMask mask, KeyButton button)
{
 	m_screen->keyDown(id, mask, button);
}

void
CClient::keyRepeat(KeyID id, KeyModifierMask mask,
				SInt32 count, KeyButton button)
{
 	m_screen->keyRepeat(id, mask, count, button);
}

void
CClient::keyUp(KeyID id, KeyModifierMask mask, KeyButton button)
{
 	m_screen->keyUp(id, mask, button);
}

void
CClient::mouseDown(ButtonID id)
{
 	m_screen->mouseDown(id);
}

void
CClient::mouseUp(ButtonID id)
{
 	m_screen->mouseUp(id);
}

void
CClient::mouseMove(SInt32 x, SInt32 y)
{
	m_screen->mouseMove(x, y);
}

void
CClient::mouseRelativeMove(SInt32 dx, SInt32 dy)
{
	m_screen->mouseRelativeMove(dx, dy);
}

void
CClient::mouseWheel(SInt32 xDelta, SInt32 yDelta)
{
	m_screen->mouseWheel(xDelta, yDelta);
}

void
CClient::screensaver(bool activate)
{
 	m_screen->screensaver(activate);
}

void
CClient::resetOptions()
{
	m_screen->resetOptions();
}

void
CClient::setOptions(const COptionsList& options)
{
	m_screen->setOptions(options);
}

CString
CClient::getName() const
{
	return m_name;
}

void
CClient::sendClipboard(ClipboardID id)
{
	// note -- m_mutex must be locked on entry
	assert(m_screen != NULL);
	assert(m_server != NULL);

	// get clipboard data.  set the clipboard time to the last
	// clipboard time before getting the data from the screen
	// as the screen may detect an unchanged clipboard and
	// avoid copying the data.
	CClipboard clipboard;
	if (clipboard.open(m_timeClipboard[id])) {
		clipboard.close();
	}
	m_screen->getClipboard(id, &clipboard);

	// check time
	if (m_timeClipboard[id] == 0 ||
		clipboard.getTime() != m_timeClipboard[id]) {
		// save new time
		m_timeClipboard[id] = clipboard.getTime();

		// marshall the data
		CString data = clipboard.marshall();

		// save and send data if different or not yet sent
		if (!m_sentClipboard[id] || data != m_dataClipboard[id]) {
			m_sentClipboard[id] = true;
			m_dataClipboard[id] = data;
			m_server->onClipboardChanged(id, &clipboard);
		}
	}
}

void
CClient::sendEvent(CEvent::Type type, void* data)
{
	m_events->addEvent(CEvent(type, getEventTarget(), data));
}

void
CClient::sendConnectionFailedEvent(const char* msg)
{
	CFailInfo* info = new CFailInfo(msg);
	info->m_retry = true;
	CEvent event(m_events->forCClient().connectionFailed(), getEventTarget(), info, CEvent::kDontFreeData);
	m_events->addEvent(event);
}

void
CClient::sendFileChunk(const void* data)
{
	CFileChunker::CFileChunk* fileChunk = reinterpret_cast<CFileChunker::CFileChunk*>(const_cast<void*>(data));
	LOG((CLOG_DEBUG1 "sendFileChunk"));
	assert(m_server != NULL);

	// relay
	m_server->fileChunkSending(fileChunk->m_chunk[0], &(fileChunk->m_chunk[1]), fileChunk->m_dataSize);
}

void
CClient::setupConnecting()
{
	assert(m_stream != NULL);

	m_events->adoptHandler(m_events->forIDataSocket().connected(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleConnected));
	m_events->adoptHandler(m_events->forIDataSocket().connectionFailed(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleConnectionFailed));
}

void
CClient::setupConnection()
{
	assert(m_stream != NULL);

	m_events->adoptHandler(m_events->forISocket().disconnected(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleDisconnected));
	m_events->adoptHandler(m_events->forIStream().inputReady(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleHello));
	m_events->adoptHandler(m_events->forIStream().outputError(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleOutputError));
	m_events->adoptHandler(m_events->forIStream().inputShutdown(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleDisconnected));
	m_events->adoptHandler(m_events->forIStream().outputShutdown(),
							m_stream->getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleDisconnected));
}

void
CClient::setupScreen()
{
	assert(m_server == NULL);

	m_ready  = false;
	m_server = new CServerProxy(this, m_stream, m_events);
	m_events->adoptHandler(m_events->forIScreen().shapeChanged(),
							getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleShapeChanged));
	m_events->adoptHandler(m_events->forIScreen().clipboardGrabbed(),
							getEventTarget(),
							new TMethodEventJob<CClient>(this,
								&CClient::handleClipboardGrabbed));
}

void
CClient::setupTimer()
{
	assert(m_timer == NULL);

	m_timer = m_events->newOneShotTimer(15.0, NULL);
	m_events->adoptHandler(CEvent::kTimer, m_timer,
							new TMethodEventJob<CClient>(this,
								&CClient::handleConnectTimeout));
}

void
CClient::cleanupConnecting()
{
	if (m_stream != NULL) {
		m_events->removeHandler(m_events->forIDataSocket().connected(),
							m_stream->getEventTarget());
		m_events->removeHandler(m_events->forIDataSocket().connectionFailed(),
							m_stream->getEventTarget());
	}
}

void
CClient::cleanupConnection()
{
	if (m_stream != NULL) {
		m_events->removeHandler(m_events->forIStream().inputReady(),
							m_stream->getEventTarget());
		m_events->removeHandler(m_events->forIStream().outputError(),
							m_stream->getEventTarget());
		m_events->removeHandler(m_events->forIStream().inputShutdown(),
							m_stream->getEventTarget());
		m_events->removeHandler(m_events->forIStream().outputShutdown(),
							m_stream->getEventTarget());
		m_events->removeHandler(m_events->forISocket().disconnected(),
							m_stream->getEventTarget());
		delete m_stream;
		m_stream = NULL;
	}
}

void
CClient::cleanupScreen()
{
	if (m_server != NULL) {
		if (m_ready) {
			m_screen->disable();
			m_ready = false;
		}
		m_events->removeHandler(m_events->forIScreen().shapeChanged(),
							getEventTarget());
		m_events->removeHandler(m_events->forIScreen().clipboardGrabbed(),
							getEventTarget());
		delete m_server;
		m_server = NULL;
	}
}

void
CClient::cleanupTimer()
{
	if (m_timer != NULL) {
		m_events->removeHandler(CEvent::kTimer, m_timer);
		m_events->deleteTimer(m_timer);
		m_timer = NULL;
	}
}

void
CClient::handleConnected(const CEvent&, void*)
{
	LOG((CLOG_DEBUG1 "connected;  wait for hello"));
	cleanupConnecting();
	setupConnection();

	// reset clipboard state
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		m_ownClipboard[id]  = false;
		m_sentClipboard[id] = false;
		m_timeClipboard[id] = 0;
	}
}

void
CClient::handleConnectionFailed(const CEvent& event, void*)
{
	IDataSocket::CConnectionFailedInfo* info =
		reinterpret_cast<IDataSocket::CConnectionFailedInfo*>(event.getData());

	cleanupTimer();
	cleanupConnecting();
	delete m_stream;
	m_stream = NULL;
	LOG((CLOG_DEBUG1 "connection failed"));
	sendConnectionFailedEvent(info->m_what.c_str());
	delete info;
}

void
CClient::handleConnectTimeout(const CEvent&, void*)
{
	cleanupTimer();
	cleanupConnecting();
	cleanupConnection();
	delete m_stream;
	m_stream = NULL;
	LOG((CLOG_DEBUG1 "connection timed out"));
	sendConnectionFailedEvent("Timed out");
}

void
CClient::handleOutputError(const CEvent&, void*)
{
	cleanupTimer();
	cleanupScreen();
	cleanupConnection();
	LOG((CLOG_WARN "error sending to server"));
	sendEvent(m_events->forCClient().disconnected(), NULL);
}

void
CClient::handleDisconnected(const CEvent&, void*)
{
	cleanupTimer();
	cleanupScreen();
	cleanupConnection();
	LOG((CLOG_DEBUG1 "disconnected"));
	sendEvent(m_events->forCClient().disconnected(), NULL);
}

void
CClient::handleShapeChanged(const CEvent&, void*)
{
	LOG((CLOG_DEBUG "resolution changed"));
	m_server->onInfoChanged();
}

void
CClient::handleClipboardGrabbed(const CEvent& event, void*)
{
	const IScreen::CClipboardInfo* info =
		reinterpret_cast<const IScreen::CClipboardInfo*>(event.getData());

	// grab ownership
	m_server->onGrabClipboard(info->m_id);

	// we now own the clipboard and it has not been sent to the server
	m_ownClipboard[info->m_id]  = true;
	m_sentClipboard[info->m_id] = false;
	m_timeClipboard[info->m_id] = 0;

	// if we're not the active screen then send the clipboard now,
	// otherwise we'll wait until we leave.
	if (!m_active) {
		sendClipboard(info->m_id);
	}
}

void
CClient::handleHello(const CEvent&, void*)
{
	SInt16 major, minor;
	if (!CProtocolUtil::readf(m_stream, kMsgHello, &major, &minor)) {
		sendConnectionFailedEvent("Protocol error from server");
		cleanupTimer();
		cleanupConnection();
		return;
	}

	// check versions
	LOG((CLOG_DEBUG1 "got hello version %d.%d", major, minor));
	if (major < kProtocolMajorVersion ||
		(major == kProtocolMajorVersion && minor < kProtocolMinorVersion)) {
		sendConnectionFailedEvent(XIncompatibleClient(major, minor).what());
		cleanupTimer();
		cleanupConnection();
		return;
	}

	// say hello back
	LOG((CLOG_DEBUG1 "say hello version %d.%d", kProtocolMajorVersion, kProtocolMinorVersion));
	CProtocolUtil::writef(m_stream, kMsgHelloBack,
							kProtocolMajorVersion,
							kProtocolMinorVersion, &m_name);

	// now connected but waiting to complete handshake
	setupScreen();
	cleanupTimer();

	// make sure we process any remaining messages later.  we won't
	// receive another event for already pending messages so we fake
	// one.
	if (m_stream->isReady()) {
		m_events->addEvent(CEvent(m_events->forIStream().inputReady(),
							m_stream->getEventTarget()));
	}
}

void
CClient::handleSuspend(const CEvent&, void*)
{
	LOG((CLOG_INFO "suspend"));
	m_suspended       = true;
	bool wasConnected = isConnected();
	disconnect(NULL);
	m_connectOnResume = wasConnected;
}

void
CClient::handleResume(const CEvent&, void*)
{
	LOG((CLOG_INFO "resume"));
	m_suspended = false;
	if (m_connectOnResume) {
		m_connectOnResume = false;
		connect();
	}
}

void
CClient::handleFileChunkSending(const CEvent& event, void*)
{
	sendFileChunk(event.getData());
}

void
CClient::handleFileRecieveCompleted(const CEvent& event, void*)
{
	onFileRecieveCompleted();
}

void
CClient::onFileRecieveCompleted()
{
	if (isReceivedFileSizeValid()) {
		m_writeToDropDirThread = new CThread(
									   new TMethodJob<CClient>(
															   this, &CClient::writeToDropDirThread));
	}
}


void
CClient::writeToDropDirThread(void*)
{
	LOG((CLOG_DEBUG "starting write to drop dir thread"));

	while (m_screen->getFakeDraggingStarted()) {
		ARCH->sleep(.1f);
	}
	
	m_fileTransferDes = m_screen->getDropTarget();
	LOG((CLOG_DEBUG "dropping file, files=%i target=%s", m_dragFileList.size(), m_fileTransferDes.c_str()));

	if (!m_fileTransferDes.empty() && m_dragFileList.size() > 0) {
		std::fstream file;
		CString dropTarget = m_fileTransferDes;
#ifdef SYSAPI_WIN32
		dropTarget.append("\\");
#else
		dropTarget.append("/");
#endif
		dropTarget.append(m_dragFileList.at(0));
		file.open(dropTarget.c_str(), std::ios::out | std::ios::binary);
		if (!file.is_open()) {
			// TODO: file open failed
		}
		
		file.write(m_receivedFileData.c_str(), m_receivedFileData.size());
		file.close();
	}
	else {
		LOG((CLOG_ERR "drop file failed: drop target is empty"));
	}
}

void
CClient::clearReceivedFileData()
{
	m_receivedFileData.clear();
}

void
CClient::setExpectedFileSize(CString data)
{
	std::istringstream iss(data);
	iss >> m_expectedFileSize;
}

void
CClient::fileChunkReceived(CString data)
{
	m_receivedFileData += data;
}

void
CClient::dragInfoReceived(UInt32 fileNum, CString data)
{
	// TODO: fix duplicate function from CServer

	if (!m_enableDragDrop) {
		LOG((CLOG_DEBUG "drag drop not enabled, ignoring drag info."));
		return;
	}

	CDragInformation::parseDragInfo(m_dragFileList, fileNum, data);
	LOG((CLOG_DEBUG "drag info received, total drag file number: %i", m_dragFileList.size()));

	for (int i = 0; i < m_dragFileList.size(); ++i) {
		LOG((CLOG_DEBUG2 "dragging file %i name: %s", i + 1, m_dragFileList.at(i).c_str()));
	}
	
	if (m_dragFileList.size() == 1) {
		m_dragFileExt = CDragInformation::getDragFileExtension(m_dragFileList.at(0));
	}
	else if (m_dragFileList.size() > 1) {
		m_dragFileExt.clear();
	}
	else {
		return;
	}
	
	m_screen->startDraggingFiles(m_dragFileExt);
}

bool
CClient::isReceivedFileSizeValid()
{
	return m_expectedFileSize == m_receivedFileData.size();
}

void
CClient::sendFileToServer(const char* filename)
{
	m_sendFileThread = new CThread(
		new TMethodJob<CClient>(
			this, &CClient::sendFileThread,
			reinterpret_cast<void*>(const_cast<char*>(filename))));
}

void
CClient::sendFileThread(void* filename)
{
	try {
		char* name  = reinterpret_cast<char*>(filename);
		CFileChunker::sendFileChunks(name, m_events, this);
	}
	catch (std::runtime_error error) {
		LOG((CLOG_ERR "failed sending file chunks: %s", error.what()));
	}

	m_sendFileThread = NULL;
}

void
CClient::draggingInfoSending(UInt32 fileCount, CString& fileList, size_t size)
{
	m_server->draggingInfoSending(fileCount, fileList.c_str(), size);
}
