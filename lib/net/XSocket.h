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

#ifndef XSOCKET_H
#define XSOCKET_H

#include "XIO.h"
#include "XBase.h"
#include "CString.h"
#include "BasicTypes.h"

//! Generic socket exception
class XSocket : public XBase { };

//! Socket bad address exception
/*!
Thrown when attempting to create an invalid network address.
*/
class XSocketAddress : public XSocket {
public:
	//! Failure codes
	enum EError {
		kUnknown,		//!< Unknown error
		kNotFound,		//!< The hostname is unknown
		kNoAddress,		//!< The hostname is valid but has no IP address
		kBadPort		//!< The port is invalid
	};

	XSocketAddress(EError, const CString& hostname, UInt16 port) throw();

	//! @name accessors
	//@{

	//! Get the error code
	EError				getError() const throw();
	//! Get the hostname
	CString				getHostname() const throw();
	//! Get the port
	UInt16				getPort() const throw();

	//@}

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();

private:
	EError				m_error;
	CString				m_hostname;
	UInt16				m_port;
};

//! Generic socket exception using \c errno
class XSocketErrno : public MXErrno {
public:
	XSocketErrno();
	XSocketErrno(int);

	// MXErrno overrides
	virtual const char*	getErrstr() const;
};

//! I/O closing exception
/*!
Thrown if a stream cannot be closed.
*/
class XSocketIOClose : public XIOClose, public XSocketErrno {
protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

//! Socket cannot bind address exception
/*!
Thrown when a socket cannot be bound to an address.
*/
class XSocketBind : public XSocket, public XSocketErrno {
public:
	XSocketBind() { }
	XSocketBind(int e) : XSocketErrno(e) { }

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

//! Socket address in use exception
/*!
Thrown when a socket cannot be bound to an address because the address
is already in use.
*/
class XSocketAddressInUse : public XSocketBind {
public:
	XSocketAddressInUse() { }
	XSocketAddressInUse(int e) : XSocketBind(e) { }
};

//! Cannot connect socket exception
/*!
Thrown when a socket cannot connect to a remote endpoint.
*/
class XSocketConnect : public XSocket, public XSocketErrno {
public:
	XSocketConnect() { }
	XSocketConnect(int e) : XSocketErrno(e) { }

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

//! Cannot create socket exception
/*!
Thrown when a socket cannot be created (by the operating system).
*/
class XSocketCreate : public XSocket, public XSocketErrno {
public:
	XSocketCreate() { }
	XSocketCreate(int e) : XSocketErrno(e) { }

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

#endif
