#ifndef CSERVER_H
#define CSERVER_H

#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CScreenMap.h"
#include "CClipboard.h"
#include "CMutex.h"
#include "CString.h"
#include "XBase.h"
#include <list>
#include <map>

class CThread;
class IServerProtocol;
class ISocketFactory;
class ISecurityFactory;
class IPrimaryScreen;

class CServer {
  public:
	CServer();
	~CServer();

	// manipulators

	// start the server.  does not return until quit() is called.
	void				run();

	// tell server to exit gracefully
	void				quit();

	// update screen map
	void				setScreenMap(const CScreenMap&);

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
	void				grabClipboard();

	// handle messages from clients
	void				setInfo(const CString& clientName,
								SInt32 w, SInt32 h, SInt32 zoneSize);
	void				grabClipboard(const CString& clientName);
	void				setClipboard(UInt32 seqNum, const CString& data);

	// accessors

	bool				isLockedToScreen() const;

	// get the current screen map
	void				getScreenMap(CScreenMap*) const;

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
		CString			m_name;
		IServerProtocol* m_protocol;
		SInt32			m_width, m_height;
		SInt32			m_zoneSize;
		bool			m_gotClipboard;
	};

	// change the active screen
	void				switchScreen(CScreenInfo*, SInt32 x, SInt32 y);

	// lookup neighboring screen
	CScreenInfo*		getNeighbor(CScreenInfo*, CScreenMap::EDirection) const;

	// lookup neighboring screen.  given a position relative to the
	// source screen, find the screen we should move onto and where.
	// if the position is sufficiently far from the source then we
	// cross multiple screens.
	CScreenInfo*		getNeighbor(CScreenInfo*,
								CScreenMap::EDirection,
								SInt32& x, SInt32& y) const;

	// adjust coordinates to account for resolution differences.  the
	// position is converted to a resolution independent form then
	// converted back to screen coordinates on the destination screen.
	void				mapPosition(CScreenInfo* src,
								CScreenMap::EDirection srcSide,
								CScreenInfo* dst,
								SInt32& x, SInt32& y) const;

	// open/close the primary screen
	void				openPrimaryScreen();
	void				closePrimaryScreen();

	// cancel running threads
	void				cleanupThreads();

	// thread method to accept incoming client connections
	void				acceptClients(void*);

	// thread method to do startup handshake with client
	void				handshakeClient(void*);

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

	CMutex				m_mutex;

	double				m_bindTimeout;

	ISocketFactory*		m_socketFactory;
	ISecurityFactory*	m_securityFactory;

	CThreadList			m_cleanupList;

	IPrimaryScreen*		m_primary;
	CScreenList			m_screens;
	CScreenInfo*		m_active;
	CScreenInfo*		m_primaryInfo;

	SInt32				m_x, m_y;

	CScreenMap			m_screenMap;

	CClipboard			m_clipboard;
	CString				m_clipboardData;
	CString				m_clipboardOwner;
	UInt32				m_clipboardSeqNum;
	bool				m_clipboardReady;
};

#endif
