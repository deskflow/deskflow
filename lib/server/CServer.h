#ifndef CSERVER_H
#define CSERVER_H

#include "IServer.h"
#include "IPrimaryScreenReceiver.h"
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
class IClient;
class IDataSocket;
class IPrimaryScreenFactory;
class IServerProtocol;
class ISocketFactory;
class IStreamFilterFactory;

//! Synergy server
/*!
This class implements the top-level server algorithms for synergy.
*/
class CServer : public IServer, public IPrimaryScreenReceiver {
public:
	/*!
	The server will look itself up in the configuration using \c serverName
	as its name.
	*/
	CServer(const CString& serverName);
	~CServer();

	//! @name manipulators
	//@{

	//! Open server
	/*!
	Open the server and return true iff successful.
	*/
	bool				open();

	//! Server main loop
	/*!
	Run server's event loop and return when exitMainLoop() is called.
	This must be called between a successful open() and close().

	(cancellation point)
	*/
	void				mainLoop();

	//! Exit event loop
	/*!
	Force mainLoop() to return.  This call can return before
	mainLoop() does (i.e. asynchronously).  This may only be
	called between a successful open() and close().
	*/
	void				exitMainLoop();

	//! Close server
	/*!
	Close the server.
	*/
	void				close();

	//! Set configuration
	/*!
	Change the server's configuration.  Returns true iff the new
	configuration was accepted (it must include the server's name).
	This will disconnect any clients no longer in the configuration.
	*/
	bool				setConfig(const CConfig&);

	//! Set primary screen factory
	/*!
	Sets the factory for creating primary screens.  This must be
	set before calling open().  This object takes ownership of the
	factory.
	*/
	void				setScreenFactory(IPrimaryScreenFactory*);

	//! Set socket factory
	/*!
	Sets the factory used to create a socket to connect to the server.
	This must be set before calling mainLoop().  This object takes
	ownership of the factory.
	*/
	void				setSocketFactory(ISocketFactory*);

	//! Set stream filter factory
	/*!
	Sets the factory used to filter the socket streams used to
	communicate with the server.  This object takes ownership
	of the factory.
	*/
	void				setStreamFilterFactory(IStreamFilterFactory*);

	//@}
	//! @name accessors
	//@{

	//! Get configuration
	/*!
	Returns the current configuration.
	*/
	void				getConfig(CConfig*) const;

	//! Get name
	/*!
	Returns the server's name passed to the c'tor
	*/
	CString				getPrimaryScreenName() const;

	//@}

	// IServer overrides
	virtual void		onError();
	virtual void		onInfoChanged(const CString&, const CClientInfo&);
	virtual bool		onGrabClipboard(const CString&, ClipboardID, UInt32);
	virtual void		onClipboardChanged(ClipboardID, UInt32, const CString&);

	// IPrimaryScreenReceiver overrides
	virtual void		onScreensaver(bool activated);
	virtual void		onKeyDown(KeyID, KeyModifierMask);
	virtual void		onKeyUp(KeyID, KeyModifierMask);
	virtual void		onKeyRepeat(KeyID, KeyModifierMask, SInt32 count);
	virtual void		onMouseDown(ButtonID);
	virtual void		onMouseUp(ButtonID);
	virtual bool		onMouseMovePrimary(SInt32 x, SInt32 y);
	virtual void		onMouseMoveSecondary(SInt32 dx, SInt32 dy);
	virtual void		onMouseWheel(SInt32 delta);

protected:
	//! Handle special keys
	/*!
	Handles keys with special meaning.
	*/
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
	IClient*			getNeighbor(IClient*, EDirection) const;

	// lookup neighboring screen.  given a position relative to the
	// source screen, find the screen we should move onto and where.
	// if the position is sufficiently far from the source then we
	// cross multiple screens.  if there is no suitable screen then
	// return NULL and x,y are not modified.
	IClient*			getNeighbor(IClient*, EDirection,
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
	CThread				startThread(IJob* adopted);

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

	// factories
	IPrimaryScreenFactory*	m_screenFactory;
	ISocketFactory*			m_socketFactory;
	IStreamFilterFactory*	m_streamFilterFactory;

	// running threads
	CThreadList			m_threads;
	CThread*			m_acceptClientThread;

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
