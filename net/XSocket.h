#ifndef XSOCKET_H
#define XSOCKET_H

#include "XBase.h"
#include "CString.h"
#include "BasicTypes.h"

class XSocket : public XBase { };

class XSocketAddress : public XSocket {
public:
	enum Error { kUnknown, kNotFound, kNoAddress, kBadPort };

	XSocketAddress(Error, const CString& hostname, UInt16 port) throw();

	// accessors

	virtual Error		getError() const throw();
	virtual CString		getHostname() const throw();
	virtual UInt16		getPort() const throw();

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();

private:
	Error				m_error;
	CString				m_hostname;
	UInt16				m_port;
};

class XSocketErrno : public XSocket, public MXErrno {
public:
	XSocketErrno();
	XSocketErrno(int);
};

class XSocketBind : public XSocketErrno {
public:
	XSocketBind() { }
	XSocketBind(int e) : XSocketErrno(e) { }

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

class XSocketAddressInUse : public XSocketBind {
public:
	XSocketAddressInUse() { }
	XSocketAddressInUse(int e) : XSocketBind(e) { }
};

class XSocketConnect : public XSocketErrno {
public:
	XSocketConnect() { }
	XSocketConnect(int e) : XSocketErrno(e) { }

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

class XSocketCreate : public XSocketErrno {
public:
	XSocketCreate() { }
	XSocketCreate(int e) : XSocketErrno(e) { }

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

#endif
