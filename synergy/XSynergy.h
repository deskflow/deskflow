#ifndef XSYNERGY_H
#define XSYNERGY_H

#include "XBase.h"

//! Generic synergy exception
class XSynergy : public XBase { };

//! Client error exception
/*!
Thrown when the client fails to follow the protocol.
*/
class XBadClient : public XSynergy {
protected:
	virtual CString		getWhat() const throw();
};

//! Incompatible client exception
/*!
Thrown when a client attempting to connect has an incompatible version.
*/
class XIncompatibleClient : public XSynergy {
public:
	XIncompatibleClient(int major, int minor);

	//! @name accessors
	//@{

	//! Get client's major version number
	int					getMajor() const throw();
	//! Get client's minor version number
	int					getMinor() const throw();

	//@}

protected:
	virtual CString		getWhat() const throw();

private:
	int					m_major;
	int					m_minor;
};

//! Client already connected exception
/*!
Thrown when a client attempting to connect is using the same name as
a client that is already connected.
*/
class XDuplicateClient : public XSynergy {
public:
	XDuplicateClient(const CString& name);

	//! @name accessors
	//@{

	//! Get client's name
	virtual const CString&
						getName() const throw();

	//@}

protected:
	virtual CString		getWhat() const throw();

private:
	CString				m_name;
};

//! Client not in map
/*!
Thrown when a client attempting to connect is using a name that is
unknown to the server.
*/
class XUnknownClient : public XSynergy {
public:
	XUnknownClient(const CString& name);

	//! @name accessors
	//@{

	//! Get the client's name
	virtual const CString&
						getName() const throw();

	//@}

protected:
	virtual CString		getWhat() const throw();

private:
	CString				m_name;
};

#endif
