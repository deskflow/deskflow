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

#ifndef CCLIENT_H
#define CCLIENT_H

#include "IClient.h"
#include "IClipboard.h"
#include "CNetworkAddress.h"
#include "INode.h"
#include "CCryptoOptions.h"
#include "CEventTypes.h"
#include "CDragInformation.h"

class CEventQueueTimer;
class CScreen;
class CServerProxy;
class IDataSocket;
class ISocketFactory;
namespace synergy { class IStream; }
class IStreamFilterFactory;
class IEventQueue;
class CCryptoStream;
class CThread;

//! Synergy client
/*!
This class implements the top-level client algorithms for synergy.
*/
class CClient : public IClient, public INode {
public:
	class CFailInfo {
	public:
		CFailInfo(const char* what) : m_retry(false), m_what(what) { }
		bool			m_retry;
		CString			m_what;
	};

public:
	/*!
	This client will attempt to connect to the server using \p name
	as its name and \p address as the server's address and \p factory
	to create the socket.  \p screen is	the local screen.
	*/
	CClient(IEventQueue* events,
							const CString& name, const CNetworkAddress& address,
							ISocketFactory* socketFactory,
							IStreamFilterFactory* streamFilterFactory,
							CScreen* screen,
							const CCryptoOptions& crypto,
							bool enableDragDrop);
	~CClient();
	
#ifdef TEST_ENV
	CClient() : m_mock(true) { }
#endif

	//! @name manipulators
	//@{

	//! Connect to server
	/*!
	Starts an attempt to connect to the server.  This is ignored if
	the client is trying to connect or is already connected.
	*/
	void				connect();

	//! Disconnect
	/*!
	Disconnects from the server with an optional error message.
	*/
	void				disconnect(const char* msg);

	//! Notify of handshake complete
	/*!
	Notifies the client that the connection handshake has completed.
	*/
	virtual void		handshakeComplete();

	//! Set crypto IV for decryption
	virtual void		setDecryptIv(const UInt8* iv);

	//! Clears the file buffer
	void				clearReceivedFileData();

	//! Set the expected size of receiving file
	void				setExpectedFileSize(CString data);

	//! Received a chunk of file data
	void				fileChunkReceived(CString data);

	//! Received drag information
	void				dragInfoReceived(UInt32 fileNum, CString data);

	//! Create a new thread and use it to send file to Server
	void				sendFileToServer(const char* filename);

	//! Set file transder destination
	void				setFileTransferDes(CString& des) { m_fileTransferDes = des; }
	
	//! Send dragging file information back to server
	void				draggingInfoSending(UInt32 fileCount, CString& fileList, size_t size);
	
	//@}
	//! @name accessors
	//@{

	//! Test if connected
	/*!
	Returns true iff the client is successfully connected to the server.
	*/
	bool				isConnected() const;

	//! Test if connecting
	/*!
	Returns true iff the client is currently attempting to connect to
	the server.
	*/
	bool				isConnecting() const;

	//! Get address of server
	/*!
	Returns the address of the server the client is connected (or wants
	to connect) to.
	*/
	CNetworkAddress		getServerAddress() const;
	
	//! Return true if recieved file size is valid
	bool				isReceivedFileSizeValid();

	//! Return expected file size
	size_t				getExpectedFileSize() { return m_expectedFileSize; }

	//@}

	// IScreen overrides
	virtual void*		getEventTarget() const;
	virtual bool		getClipboard(ClipboardID id, IClipboard*) const;
	virtual void		getShape(SInt32& x, SInt32& y,
							SInt32& width, SInt32& height) const;
	virtual void		getCursorPos(SInt32& x, SInt32& y) const;

	// IClient overrides
	virtual void		enter(SInt32 xAbs, SInt32 yAbs,
							UInt32 seqNum, KeyModifierMask mask,
							bool forScreensaver);
	virtual bool		leave();
	virtual void		setClipboard(ClipboardID, const IClipboard*);
	virtual void		grabClipboard(ClipboardID);
	virtual void		setClipboardDirty(ClipboardID, bool);
	virtual void		keyDown(KeyID, KeyModifierMask, KeyButton);
	virtual void		keyRepeat(KeyID, KeyModifierMask,
							SInt32 count, KeyButton);
	virtual void		keyUp(KeyID, KeyModifierMask, KeyButton);
	virtual void		mouseDown(ButtonID);
	virtual void		mouseUp(ButtonID);
	virtual void		mouseMove(SInt32 xAbs, SInt32 yAbs);
	virtual void		mouseRelativeMove(SInt32 xRel, SInt32 yRel);
	virtual void		mouseWheel(SInt32 xDelta, SInt32 yDelta);
	virtual void		screensaver(bool activate);
	virtual void		resetOptions();
	virtual void		setOptions(const COptionsList& options);
	virtual CString		getName() const;

private:
	void				sendClipboard(ClipboardID);
	void				sendEvent(CEvent::Type, void*);
	void				sendConnectionFailedEvent(const char* msg);
	void				sendFileChunk(const void* data);
	void				sendFileThread(void*);
	void				writeToDropDirThread(void*);
	void				setupConnecting();
	void				setupConnection();
	void				setupScreen();
	void				setupTimer();
	void				cleanupConnecting();
	void				cleanupConnection();
	void				cleanupScreen();
	void				cleanupTimer();
	void				handleConnected(const CEvent&, void*);
	void				handleConnectionFailed(const CEvent&, void*);
	void				handleConnectTimeout(const CEvent&, void*);
	void				handleOutputError(const CEvent&, void*);
	void				handleDisconnected(const CEvent&, void*);
	void				handleShapeChanged(const CEvent&, void*);
	void				handleClipboardGrabbed(const CEvent&, void*);
	void				handleHello(const CEvent&, void*);
	void				handleSuspend(const CEvent& event, void*);
	void				handleResume(const CEvent& event, void*);
	void				handleFileChunkSending(const CEvent&, void*);
	void				handleFileRecieveCompleted(const CEvent&, void*);
	void				onFileRecieveCompleted();

public:
	bool					m_mock;

private:
	CString					m_name;
	CNetworkAddress			m_serverAddress;
	ISocketFactory*			m_socketFactory;
	IStreamFilterFactory*	m_streamFilterFactory;
	CScreen*				m_screen;
	synergy::IStream*		m_stream;
	CEventQueueTimer*		m_timer;
	CServerProxy*			m_server;
	bool					m_ready;
	bool					m_active;
	bool					m_suspended;
	bool					m_connectOnResume;
	bool					m_ownClipboard[kClipboardEnd];
	bool					m_sentClipboard[kClipboardEnd];
	IClipboard::Time		m_timeClipboard[kClipboardEnd];
	CString					m_dataClipboard[kClipboardEnd];
	IEventQueue*			m_events;
	CCryptoStream*			m_cryptoStream;
	CCryptoOptions			m_crypto;
	std::size_t				m_expectedFileSize;
	CString					m_receivedFileData;
	CString					m_fileTransferSrc;
	CString					m_fileTransferDes;
	CDragFileList			m_dragFileList;
	CString					m_dragFileExt;
	CThread*				m_sendFileThread;
	CThread*				m_writeToDropDirThread;
	bool					m_enableDragDrop;
};

#endif
