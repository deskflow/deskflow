#ifndef XNETWORK_H
#define XNETWORK_H

#include "XBase.h"
#include "CString.h"

//! Generic network exception
/*!
Network exceptions are thrown when initializing the network subsystem
and not during normal network use.
*/
class XNetwork : public XBase { };

//! Network subsystem not available exception
/*!
Thrown when the network subsystem is unavailable, typically because
the necessary shared library is unavailable.
*/
class XNetworkUnavailable : public XNetwork {
protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

//! Network subsystem failed exception
/*!
Thrown when the network subsystem cannot be initialized.
*/
class XNetworkFailed : public XNetwork {
protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

//! Network subsystem vesion unsupported exception
/*!
Thrown when the network subsystem has a version incompatible with
what's expected.
*/
class XNetworkVersion : public XNetwork {
public:
	XNetworkVersion(int major, int minor) throw();

	//! @name accessors
	//@{

	//! Get the network subsystem's major version
	int					getMajor() const throw();
	//! Get the network subsystem's minor version
	int					getMinor() const throw();

	//@}

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();

private:
	int					m_major;
	int					m_minor;
};

//! Network subsystem incomplete exception
/*!
Thrown when the network subsystem is missing an expected and required
function.
*/
class XNetworkFunctionUnavailable : public XNetwork {
public:
	XNetworkFunctionUnavailable(const char* name) throw();

protected:
	// XBase overrides
	virtual CString		getWhat() const throw();

private:
	CString				m_name;
};

#endif
