#include "XIO.h"

//
// XIOErrno
//

XIOErrno::XIOErrno() :
	MXErrno()
{
	// do nothing
}

XIOErrno::XIOErrno(int err) :
	MXErrno(err)
{
	// do nothing
}


//
// XIOClose
//

CString
XIOClose::getWhat() const throw()
{
	return format("XIOClose", "close: %{1}", XIOErrno::getErrstr());
}


//
// XIOClosed
//

CString
XIOClosed::getWhat() const throw()
{
	return format("XIOClosed", "already closed");
}


//
// XIOEndOfStream
//

CString
XIOEndOfStream::getWhat() const throw()
{
	return format("XIOEndOfStream", "reached end of stream");
}
