#include "XSynergy.h"

//
// XBadClient
//

CString
XBadClient::getWhat() const throw()
{
	return "XBadClient";
}


//
// XIncompatibleClient
//

XIncompatibleClient::XIncompatibleClient(int major, int minor) :
	m_major(major),
	m_minor(minor)
{
	// do nothing
}

int
XIncompatibleClient::getMajor() const throw()
{
	return m_major;
}

int
XIncompatibleClient::getMinor() const throw()
{
	return m_minor;
}

CString
XIncompatibleClient::getWhat() const throw()
{
	return "XIncompatibleClient";
}


//
// XDuplicateClient
//

XDuplicateClient::XDuplicateClient(const CString& name) :
	m_name(name)
{
	// do nothing
}

const CString&
XDuplicateClient::getName() const throw()
{
	return m_name;
}

CString
XDuplicateClient::getWhat() const throw()
{
	return "XDuplicateClient";
}


//
// XUnknownClient
//

XUnknownClient::XUnknownClient(const CString& name) :
	m_name(name)
{
	// do nothing
}

const CString&
XUnknownClient::getName() const throw()
{
	return m_name;
}

CString
XUnknownClient::getWhat() const throw()
{
	return "XUnknownClient";
}
