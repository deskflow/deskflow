#include "XSynergy.h"

//
// XBadClient
//

CString					XBadClient::getWhat() const throw()
{
	return "XBadClient";
}

//
//
//

XIncompatibleClient::XIncompatibleClient(int major, int minor) :
								m_major(major),
								m_minor(minor)
{
	// do nothing
}

int						XIncompatibleClient::getMajor() const throw()
{
	return m_major;
}

int						XIncompatibleClient::getMinor() const throw()
{
	return m_minor;
}

CString					XIncompatibleClient::getWhat() const throw()
{
	return "XIncompatibleClient";
}

