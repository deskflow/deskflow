#ifndef CSERVER_H
#define CSERVER_H

#include "IServer.h"
#include "CConfig.h"
#include "CClipboard.h"
#include "CCondVar.h"
#include "CMutex.h"
#include "CThread.h"
#include "stdlist.h"
#include "stdmap.h"

class CClientProxy;
class CHTTPServer;
class CPrimaryClient;
class CThread;
class IClient;
class IDataSocket;
class IServerProtocol;
class ISocketFactory;
class ISecurityFactory;

class CServer : public IServer {
public:
	CServer(const CString& serverName);
	~CServer();

	// manipulators

	// open the server's screen
	bool				open();

	// start the server.  does not return until quit() is called.
	// this must be preceeded by a successful call to open().
	void				run();

	// tell server to exit gracefully.  this may only be called
	// after a successful open().
	void				quit();

	// update screen map.  returns true iff the new configuration was
	// accepted.
	bool				setConfig(const CConfig&);

	// accessors

	// get the current screen map
	void				getConfig(CConfig*) const;

	// get the primary screen's name
	CString				getPrimaryScreenName() const;

	// IPrimaryScreenReceiver overrides
	virtual void		onError();
	virtual void		onKeyDown(KeyID, KeyModifierMask);
	virtual void		onKeyUp(KeyID, KeyModifierMask);
	virtual void		onKeyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		onMouseDown(ButtonID);
	virtual void		onMouseUp(ButtonID);
	virtual bool		onMouseMovePrimary(SInt32 x, SInt32 y);
	virtual void		onMouseMoveSecondary(SInt32 dx, SInt32 dy);
	virtual void		onMouseWheel(SInt32 delta);
	virtual void		onScreenSaver(bool activated);

	// IServer overrides
	virtual void		onInfoChanged(const CString&, const CClientInfo&);
	virtual bool		onGrabClipboard(const CString&, ClipboardID, UInt32);
	virtual void		onClipboardChanged(ClipboardID, UInt32, const CString&);

protected:
	bool				onCommandKey(KeyID, KeyModifierMask, bool down);

private:
	typedef std::list<CThread> CThreadList;

	// get the sides of the primary screen that have neighbors
	UInt32				getActivePrimarySides() const;

	// handle mouse motion
	bool				onMouseMovePrimaryNoLock(SInt32 x, SInt32 y);
	void				onMouseMoveSecondaryNoLock(SInt32 dx, SInt32 dy);

	// set the clipboard
	void				onClipboardChangedNoLock(ClipboardID,
							UInt32 seqNum, const CString& data);

	// returns true iff mouse should be locked to the current screen
	bool				isLockedToScreenNoLock() const;

	// change the active screen
	void				switchScreen(IClient*,
							SInt32 x, SInt32 y, bool forScreenSaver);

	// lookup neighboring screen
	IClient*			getNeighbor(IClient*, CConfig::EDirection) const;

	// lookup neighboring screen.  given a position relative to the
	// source screen, find the screen we should move onto and where.
	// if the position is sufficiently far from the source then we
	// cross multiple screens.  if there is no suitable screen then
	// return NULL and x,y are not modified.
	IClient*			getNeighbor(IClient*,
							CConfig::EDirection,
							SInt32& x, SInt32& y) const;

	// open/close the primary screen
	void				openPrimaryScreen();
	void				closePrimaryScreen();

	// update the clipboard if owned by the primary screen
	void				updatePrimaryClipboard(ClipboardID);

	// close all clients that are *not* in config, not including the
	// primary client.
	void				closeClients(const CConfig& config);

	// start a thread, adding it to the list of threads
	void				startThread(IJob* adopted);

	// cancel running threads, waiting at most timeout seconds for
	// them to finish.
	void				stopThreads(double timeout = -1.0);

	// reap threads, clearing finished threads from the thread list.
	// doReapThreads does the work on the given thread list.
	void				reapThreads();
	void				doReapThreads(CThreadList&);

	// thread method to accept incoming client connections
	void				acceptClients(void*);

	// thread method to do client interaction
	void				runClient(void*);
	CClientProxy*		handshakeClient(IDataSocket*);

	// thread method to accept incoming HTTP connections
	void				acceptHTTPClients(void*);

	// thread method to process HTTP requests
	void				processHTTPRequest(void*);

	// connection list maintenance
	void				addConnection(IClient*);
	void				removeConnection(const CString& name);

private:
	class CClipboardInfo {
	public:
		CClipboardInfo();

	public:
		CClipboard		m_clipboard;
		CString			m_clipboardData;
		CString			m_clipboardOwner;
		UInt32			m_clipboardSeqNum;
	};

	CMutex				m_mutex;

	// the name of the primary screen
	CString				m_name;

	// how long to wait to bind our socket until we give up
	double				m_bindTimeout;

	ISocketFactory*		m_socketFactory;
	ISecurityFactory*	m_securityFactory;

	// running threads
	CThreadList			m_threads;

	// the screens
	typedef std::map<CString, IClient*> CClientList;
	typedef std::map<CString, CThread> CClientThreadList;

	// all clients indexed by name
	CClientList			m_clients;

	// run thread of all secondary screen clients.  does not include the
	// primary screen's run thread.
	CClientThreadList	m_clientThreads;

	// the primary screen client
	CPrimaryClient*		m_primaryClient;

	// the client with focus
	IClient*			m_active;

	// the sequence number of enter messages
	UInt32				m_seqNum;

	// current mouse position (in absolute secondary screen coordinates)
	SInt32				m_x, m_y;

	// current configuration
	CConfig				m_config;

	// clipboard cache
	CClipboardInfo		m_clipboards[kClipboardEnd];

	// state saved when screen saver activates
	IClient*			m_activeSaver;
	SInt32				m_xSaver, m_ySaver;

	// HTTP request processing stuff
	CHTTPServer*		m_httpServer;
	CCondVar<SInt32>	m_httpAvailable;
	static const SInt32	s_httpMaxSimultaneousRequests;
};

#endif
