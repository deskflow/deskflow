#ifndef XSOCKET_H
#define XSOCKET_H

#include "XBase.h"

class XSocket : public XBase {
  public:
	// accessors

	const char*			getMessage() const { return m_msg; }

  protected:
	XSocket(const char* msg) : m_msg(msg) { }

  private:
	const char*			m_msg;
};

#define XSOCKETDEF(_n)								\
class _n : public XSocket {							\
  public:											\
	_n(const char* msg) : XSocket(msg) { }			\
	XNAME(_n)										\
};

XSOCKETDEF(XSocketCreate)
XSOCKETDEF(XSocketName)
XSOCKETDEF(XSocketConnect)
XSOCKETDEF(XSocketListen)
XSOCKETDEF(XSocketAccept)
XSOCKETDEF(XSocketWrite)

#endif
