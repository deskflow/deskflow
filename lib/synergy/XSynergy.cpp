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
	return format("XIncompatibleClient", "incompatible client %{1}.%{2}",
								CStringUtil::print("%d", m_major).c_str(),
								CStringUtil::print("%d", m_minor).c_str());
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
	return format("XDuplicateClient", "duplicate client %{1}", m_name.c_str());
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
	return format("XUnknownClient", "unknown client %{1}", m_name.c_str());
}
