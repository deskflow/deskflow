#ifndef CSERVER_H
#define CSERVER_H

#include "ClipboardTypes.h"
#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CConfig.h"
#include "CClipboard.h"
#include "CNetworkAddress.h"
#include "CCondVar.h"
#include "CMutex.h"
#include "CString.h"
#include "CThread.h"
#include "XBase.h"
#include "stdlist.h"
#include "stdmap.h"

class CThread;
class IServerProtocol;
class ISocketFactory;
class ISecurityFactory;
class IPrimaryScreen;
class CHTTPServer;

class CServer {
public:
	CServer();
	~CServer();

	// manipulators

	// start the server.  does not return until quit() is called.
	void				run();

	// tell server to exit gracefully
	void				quit();

	// tell the server to shutdown.  this is called in an emergency
	// when we need to tell the server that we cannot continue.  the
	// server will attempt to clean up.
	void				shutdown();

	// update screen map.  returns true iff the new configuration was
	// accepted.
	bool				setConfig(const CConfig&);

	// handle events on server's screen.  onMouseMovePrimary() returns
	// true iff the mouse enters a jump zone and jumps.
	void				onKeyDown(KeyID, KeyModifierMask);
	void				onKeyUp(KeyID, KeyModifierMask);
	void				onKeyRepeat(KeyID, KeyModifierMask, SInt32 count);
	void				onMouseDown(ButtonID);
	void				onMouseUp(ButtonID);
	bool				onMouseMovePrimary(SInt32 x, SInt32 y);
	void				onMouseMoveSecondary(SInt32 dx, SInt32 dy);
	void				onMouseWheel(SInt32 delta);
	void				grabClipboard(ClipboardID);

	// handle updates from primary
	void				setInfo(SInt32 wScreen, SInt32 hScreen,
								SInt32 zoneSize,
								SInt32 xMouse, SInt32 yMouse);

	// handle messages from clients
	void				setInfo(const CString& clientName,
								SInt32 wScreen, SInt32 hScreen,
								SInt32 zoneSize,
								SInt32 xMouse, SInt32 yMouse);
	void				grabClipboard(ClipboardID,
								UInt32 seqNum, const CString& clientName);
	void				setClipboard(ClipboardID,
								UInt32 seqNum, const CString& data);

	// accessors

	// returns true if the mouse should be locked to the current screen
	bool				isLockedToScreen() const;

	// get the current screen map
	void				getConfig(CConfig*) const;

	// get the primary screen's name
	CString				getPrimaryScreenName() const;

	// get the sides of the primary screen that have neighbors
	UInt32				getActivePrimarySides() const;

protected:
	bool				onCommandKey(KeyID, KeyModifierMask, bool down);

private:
	class CCleanupNote {
	public:
		CCleanupNote(CServer*);
		~CCleanupNote();

	private:
		CServer*		m_server;
	};

	class CConnectionNote {
	public:
		CConnectionNote(CServer*, const CString&, IServerProtocol*);
		~CConnectionNote();

	private:
		bool			m_pending;
		CServer*		m_server;
		CString			m_name;
	};

	class CScreenInfo {
	public:
		CScreenInfo(const CString& name, IServerProtocol*);
		~CScreenInfo();

	public:
		// the thread handling this screen's connection.  used when
		// forcing a screen to disconnect.
		CThread			m_thread;
		CString			m_name;
		IServerProtocol* m_protocol;
		bool			m_ready;
		SInt32			m_width, m_height;
		SInt32			m_zoneSize;
		bool			m_gotClipboard[kClipboardEnd];
	};

	// handle mouse motion
	bool				onMouseMovePrimaryNoLock(SInt32 x, SInt32 y);
	void				onMouseMoveSecondaryNoLock(SInt32 dx, SInt32 dy);

	// update screen info
	void				setInfoNoLock(const CString& screenName,
								SInt32 wScreen, SInt32 hScreen,
								SInt32 zoneSize,
								SInt32 xMouse, SInt32 yMouse);

	// grab the clipboard
	void				grabClipboardNoLock(ClipboardID,
								UInt32 seqNum, const CString& clientName);

	// returns true iff mouse should be locked to the current screen
	bool				isLockedToScreenNoLock() const;

	// change the active screen
	void				switchScreen(CScreenInfo*, SInt32 x, SInt32 y);

	// lookup neighboring screen
	CScreenInfo*		getNeighbor(CScreenInfo*, CConfig::EDirection) const;

	// lookup neighboring screen.  given a position relative to the
	// source screen, find the screen we should move onto and where.
	// if the position is sufficiently far from the source then we
	// cross multiple screens.
	CScreenInfo*		getNeighbor(CScreenInfo*,
								CConfig::EDirection,
								SInt32& x, SInt32& y) const;

	// adjust coordinates to account for resolution differences.  the
	// position is converted to a resolution independent form then
	// converted back to screen coordinates on the destination screen.
	void				mapPosition(CScreenInfo* src,
								CConfig::EDirection srcSide,
								CScreenInfo* dst,
								SInt32& x, SInt32& y) const;

	// open/close the primary screen
	void				openPrimaryScreen();
	void				closePrimaryScreen();

	// clear gotClipboard flags in all screens
	void				clearGotClipboard(ClipboardID);

	// send clipboard to the active screen if it doesn't already have it
	void				sendClipboard(ClipboardID);

	// update the clipboard if owned by the primary screen
	void				updatePrimaryClipboard(ClipboardID);

	// cancel running threads
	void				cleanupThreads(double timeout = -1.0);

	// thread method to accept incoming client connections
	void				acceptClients(void*);

	// thread method to do startup handshake with client
	void				handshakeClient(void*);

	// thread method to accept incoming HTTP connections
	void				acceptHTTPClients(void*);

	// thread method to process HTTP requests
	void				processHTTPRequest(void*);

	// thread cleanup list maintenance
	friend class CCleanupNote;
	void				addCleanupThread(const CThread& thread);
	void				removeCleanupThread(const CThread& thread);

	// connection list maintenance
	friend class CConnectionNote;
	CScreenInfo*		addConnection(const CString& name, IServerProtocol*);
	void				removeConnection(const CString& name);

private:
	typedef std::list<CThread*> CThreadList;
	typedef std::map<CString, CScreenInfo*> CScreenList;
	class CClipboardInfo {
	public:
		CClipboardInfo();

	public:
		CClipboard		m_clipboard;
		CString			m_clipboardData;
		CString			m_clipboardOwner;
		UInt32			m_clipboardSeqNum;
		bool			m_clipboardReady;
	};

	CMutex				m_mutex;

	double				m_bindTimeout;

	ISocketFactory*		m_socketFactory;
	ISecurityFactory*	m_securityFactory;

	CThreadList			m_cleanupList;
	CCondVar<SInt32>	m_cleanupSize;

	IPrimaryScreen*		m_primary;
	CScreenList			m_screens;
	CScreenInfo*		m_active;
	CScreenInfo*		m_primaryInfo;

	// the sequence number of enter messages
	UInt32				m_seqNum;

	// current mouse position (in absolute secondary screen coordinates)
	SInt32				m_x, m_y;

	CConfig				m_config;

	CClipboardInfo		m_clipboards[kClipboardEnd];

	// HTTP request processing stuff
	CHTTPServer*		m_httpServer;
	CCondVar<SInt32>	m_httpAvailable;
	static const SInt32	s_httpMaxSimultaneousRequests;
};

#endif
