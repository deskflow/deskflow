#include "XScreen.h"

//
// XScreenOpenFailure
//

CString
XScreenOpenFailure::getWhat() const throw()
{
	return format("XScreenOpenFailure", "unable to open screen");
}


//
// XScreenUnavailable
//

XScreenUnavailable::XScreenUnavailable(double timeUntilRetry) :
	m_timeUntilRetry(timeUntilRetry)
{
	// do nothing
}

XScreenUnavailable::~XScreenUnavailable()
{
	// do nothing
}

double
XScreenUnavailable::getRetryTime() const
{
	return m_timeUntilRetry;
}

CString
XScreenUnavailable::getWhat() const throw()
{
	return format("XScreenUnavailable", "unable to open screen");
}
