#ifndef CSERVER_H
#define CSERVER_H

#include "KeyTypes.h"
#include "MouseTypes.h"
#include "CScreenMap.h"
#include "CCondVar.h"
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

	void				run();

	// update screen map
	void				setScreenMap(const CScreenMap&);

	// handle events on server's screen
	void				onKeyDown(KeyID, KeyModifierMask);
	void				onKeyUp(KeyID, KeyModifierMask);
	void				onKeyRepeat(KeyID, KeyModifierMask);
	void				onMouseDown(ButtonID);
	void				onMouseUp(ButtonID);
	void				onMouseMovePrimary(SInt32 x, SInt32 y);
	void				onMouseMoveSecondary(SInt32 dx, SInt32 dy);
	void				onMouseWheel(SInt32 delta);

	// handle messages from clients
	void				setInfo(const CString& clientName,
								SInt32 w, SInt32 h, SInt32 zoneSize);

	// accessors

	bool				isLockedToScreen() const;

	// get the current screen map
	void				getScreenMap(CScreenMap*) const;

  protected:
	bool				onCommandKey(KeyID, KeyModifierMask, bool down);
	void				quit();

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
	CCondVar<bool>		m_done;

	double				m_bindTimeout;

	ISocketFactory*		m_socketFactory;
	ISecurityFactory*	m_securityFactory;

	CThreadList			m_cleanupList;

	IPrimaryScreen*		m_primary;
	CScreenList			m_screens;
	CScreenInfo*		m_active;

	SInt32				m_x, m_y;

	CScreenMap			m_screenMap;
};

#endif
