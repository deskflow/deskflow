#ifndef XSOCKET_H
#define XSOCKET_H

#include "CString.h"
#include "XBase.h"
#include "BasicTypes.h"

class XSocket : public XBase { };

class XSocketAddress : public XSocket {
public:
	enum Error { kUnknown, kNotFound, kNoAddress, kBadPort };

	XSocketAddress(Error, const CString& hostname, SInt16 port) throw();

	// accessors

	virtual Error		getError() const throw();
	virtual CString		getHostname() const throw();
	virtual SInt16		getPort() const throw();

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();

private:
	Error				m_error;
	CString				m_hostname;
	SInt16				m_port;
};

class XSocketErrno : public XSocket, public MXErrno {
public:
	XSocketErrno();
	XSocketErrno(int);
};

class XSocketBind : public XSocketErrno {
protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

class XSocketAddressInUse : public XSocketBind { };

class XSocketConnect : public XSocketErrno {
protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

class XSocketCreate : public XSocketErrno {
protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

#endif
