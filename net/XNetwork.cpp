#include "XNetwork.h"

//
// XNetworkUnavailable
//

CString
XNetworkUnavailable::getWhat() const throw()
{
	return format("XNetworkUnavailable", "network library is not available");
}


//
// XNetworkFailed
//

CString
XNetworkFailed::getWhat() const throw()
{
	return format("XNetworkFailed", "cannot initialize network library");
}


//
// XNetworkVersion
//

XNetworkVersion::XNetworkVersion(
	int major,
	int minor) throw() :
	m_major(major),
	m_minor(minor)
{
	// do nothing
}

int
XNetworkVersion::getMajor() const throw()
{
	return m_major;
}

int
XNetworkVersion::getMinor() const throw()
{
	return m_minor;
}

CString
XNetworkVersion::getWhat() const throw()
{
	return format("XNetworkVersion",
								"unsupported network version %d.%d",
								m_major, m_minor);
}


//
// XNetworkFunctionUnavailable
//

XNetworkFunctionUnavailable::XNetworkFunctionUnavailable(
	const char* name) throw()
{
	try {
		m_name = name;
	}
	catch (...) {
		// ignore
	}
}

CString
XNetworkFunctionUnavailable::getWhat() const throw()
{
	return format("XNetworkFunctionUnavailable",
								"missing network function %s",
								m_name.c_str());
}
