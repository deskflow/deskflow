#ifndef CCLIENT_H
#define CCLIENT_H

#include "IClient.h"

class IScreen;
class ISocket;

class CClient : public IClient {
  public:
	CClient(IScreen* screen);
	virtual ~CClient();

	// IClient overrides
	virtual void		run(const CString& hostname);

  private:
	void				onConnect();
	void				onRead();

	void				sendScreenSize();

  private:
	IScreen*			m_screen;
	ISocket*			m_socket;
};

#endif
