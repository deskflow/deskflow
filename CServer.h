#ifndef CSERVER_H
#define CSERVER_H

#include "IServer.h"
#include "BasicTypes.h"
#include "CString.h"
#include <map>
#include <set>

class CEvent;
class CEventKey;
class IScreen;
class ISocket;

class CServer : public IServer {
  public:
	enum EDirection { kLeft, kRight, kTop, kBottom,
						kFirstDirection = kLeft, kLastDirection = kBottom };

	CServer();
	virtual ~CServer();

	// manipulators

	// set the server's interface and port to listen for remote screens
	void				setListenPort(const CString& hostname, UInt16 port);

	// add local screen
	void				addLocalScreen(IScreen*);

	// add a remote screen
	void				addRemoteScreen(const CString& name);

	// remove a local or remote screen.  neighbors on opposite sides
	// of this screen are made neighbors of each other.
	void				removeScreen(const CString& name);

	// connect/disconnect screen edges
	void				connectEdge(const CString& src, EDirection srcSide,
								const CString& dst);
	void				disconnectEdge(const CString& src, EDirection srcSide);

	// accessors


	// IServer overrides
	virtual void		run();
	virtual void		onClipboardChanged(IScreen*);
	virtual void		setActiveScreen(IScreen*);
	virtual IScreen*	getActiveScreen() const;

  protected:
	virtual void		relayEvent(const CEvent* event);
	virtual bool		onCommandKey(const CEventKey* keyEvent);
	virtual void		onLocalMouseMove(SInt32 x, SInt32 y);
	virtual void		onRemoteMouseMove(SInt32 dx, SInt32 dy);
	virtual bool		isLockedToScreen() const;
	virtual void		mapPosition(const IScreen* src, EDirection srcSide,
								const IScreen* dst, SInt32& x, SInt32& y) const;
	IScreen*			getNeighbor(const IScreen* src, EDirection) const;
	IScreen*			getNeighbor(const IScreen* src, EDirection srcSide,
								SInt32& x, SInt32& y) const;
	void				switchScreen(IScreen* screen, SInt32 x, SInt32 y);

  private:
	void				addScreen(const CString&, IScreen*);
	void				newConnectionCB();
	void				loginCB(ISocket*);

	struct ScreenCell {
	  public:
		ScreenCell() : m_screen(NULL) { }
	  public:
		IScreen*		m_screen;
		CString			m_neighbor[kLastDirection - kFirstDirection + 1];
	};

  private:
	typedef std::map<CString, ScreenCell> ScreenMap;
	typedef std::set<ISocket*> SocketSet;

	// main loop stuff
	bool				m_running;
	bool				m_done;

	// screen tracking
	IScreen*			m_localScreen;
	IScreen*			m_activeScreen;
	SInt32				m_x, m_y;
	ScreenMap			m_map;

	// listen socket stuff
	CString				m_listenHost;
	UInt16				m_listenPort;
	ISocket*			m_listenSocket;

	// login sockets
	SocketSet			m_logins;

	static const SInt32	s_zoneSize;
};

#endif
