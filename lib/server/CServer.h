/*
 * synergy -- mouse and keyboard sharing utility
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
 */

#ifndef CSERVER_H
#define CSERVER_H

#include "IServer.h"
#include "IPrimaryScreenReceiver.h"
#include "CConfig.h"
#include "CClipboard.h"
#include "CCondVar.h"
#include "CMutex.h"
#include "CThread.h"
#include "CJobList.h"
#include "CStopwatch.h"
#include "stdlist.h"
#include "stdmap.h"
#include "stdvector.h"

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
	enum EStatus {
		kNotRunning,
		kRunning,
		kServerNameUnknown,
		kError,
		kMaxStatus
	};

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
	Open the server.  Throws XScreenUnavailable if the server's
	screen cannot be opened but might be available after some time.
	Otherwise throws some other exception if the server's screen or
	the server cannot be opened and retrying won't help.
	*/
	void				open();

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

	//! Add a job to notify of status changes
	/*!
	The added job is run whenever the server's status changes in
	certain externally visible ways.  The client keeps ownership
	of the job.
	*/
	void				addStatusJob(IJob*);

	//! Remove a job to notify of status changes
	/*!
	Removes a previously added status notification job.  A job can
	remove itself when called but must not remove any other jobs.
	The client keeps ownership of the job.
	*/
	void				removeStatusJob(IJob*);

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

	//! Get number of connected clients
	/*!
	Returns the number of connected clients, including the server itself.
	*/
	UInt32				getNumClients() const;

	//! Get the list of connected clients
	/*!
	Set the \c list to the names of the currently connected clients.
	*/
	void				getClients(std::vector<CString>& list) const;

	//! Get the status
	/*!
	Returns the current status and status message.
	*/
	EStatus				getStatus(CString* = NULL) const;

	//@}

	// IServer overrides
	virtual void		onError();
	virtual void		onInfoChanged(const CString&, const CClientInfo&);
	virtual bool		onGrabClipboard(const CString&, ClipboardID, UInt32);
	virtual void		onClipboardChanged(ClipboardID, UInt32, const CString&);

	// IPrimaryScreenReceiver overrides
	virtual void		onScreensaver(bool activated);
	virtual void		onOneShotTimerExpired(UInt32 id);
	virtual void		onKeyDown(KeyID, KeyModifierMask, KeyButton);
	virtual void		onKeyUp(KeyID, KeyModifierMask, KeyButton);
	virtual void		onKeyRepeat(KeyID, KeyModifierMask,
							SInt32 count, KeyButton);
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

	//! Exit event loop and note an error condition
	/*!
	Force mainLoop() to return by throwing an exception.  This call
	can return before mainLoop() does (i.e. asynchronously).  This
	may only be called between a successful open() and close().
	*/
	void				exitMainLoopWithError();

private:
	typedef std::list<CThread> CThreadList;

	// notify status jobs of a change
	void				runStatusJobs() const;

	// set new status
	void				setStatus(EStatus, const char* msg = NULL);

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

	// test if a switch is permitted.  this includes testing user
	// options like switch delay and tracking any state required to
	// implement them.  returns true iff a switch is permitted.
	bool				isSwitchOkay(IClient* dst, EDirection,
							SInt32 x, SInt32 y);

	// update switch state due to a mouse move that doesn't try to
	// switch screens.
	void				onNoSwitch(bool inTapZone);

	// reset switch wait state
	void				clearSwitchState();

	// send screen options to \c client
	void				sendOptions(IClient* client) const;

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
	class XServerRethrow : public XBase {
	protected:
		// XBase overrides
		virtual CString	getWhat() const throw();
	};

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

	// true if we should exit the main loop by throwing an exception.
	// this is used to propagate an exception from one of our threads
	// to the mainLoop() thread.  but, since we can't make a copy of
	// the original exception, we return an arbitrary, unique
	// exception type.  the caller of mainLoop() cannot catch this
	// exception except through XBase or ....
	bool				m_error;

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

	// common state for screen switch tests.  all tests are always
	// trying to reach the same screen in the same direction.
	EDirection			m_switchDir;
	IClient*			m_switchScreen;

	// state for delayed screen switching
	double				m_switchWaitDelay;
	UInt32				m_switchWaitTimer;
	bool				m_switchWaitEngaged;
	SInt32				m_switchWaitX, m_switchWaitY;

	// state for double-tap screen switching
	double				m_switchTwoTapDelay;
	CStopwatch			m_switchTwoTapTimer;
	bool				m_switchTwoTapEngaged;
	bool				m_switchTwoTapArmed;
	SInt32				m_switchTwoTapZone;

	// the status change jobs and status
	CJobList			m_statusJobs;
	EStatus				m_status;
	CString				m_statusMessage;
};

#endif
