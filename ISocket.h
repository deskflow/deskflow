#ifndef ISOCKET_H
#define ISOCKET_H

#include "BasicTypes.h"

class IJob;
class CString;

class ISocket {
  public:
	// d'tor closes the socket
	ISocket() { }
	virtual ~ISocket() { }

	// manipulators

	// set the job to invoke when the socket is readable or writable.
	// a socket that has connected after a call to connect() becomes
	// writable.  a socket that is ready to accept a connection after
	// a call to listen() becomes readable.  the socket returned by
	// accept() does not have any jobs assigned to it.
	virtual void		setReadJob(IJob* adoptedJob) = 0;
	virtual void		setWriteJob(IJob* adoptedJob) = 0;

	// open/close.  connect() begins connecting to the given host but
	// doesn't wait for the connection to complete.  listen() begins
	// listening on the given interface and port;  if hostname is
	// empty then listen on all interfaces.  accept() waits for a
	// connection on the listening interface and returns a new
	// socket for the connection.
	virtual void		connect(const CString& hostname, UInt16 port) = 0;
	virtual void		listen(const CString& hostname, UInt16 port) = 0;
	virtual ISocket*	accept() = 0;

	// read data from socket.  returns without waiting if not enough
	// data is available.  returns the number of bytes actually read,
	// which is zero if there were no bytes to read and -1 if the
	// remote end of the socket has disconnected.
	virtual SInt32		read(void* buffer, SInt32 numBytes) = 0;

	// write data to socket.  waits until all data has been written.
	virtual void		write(const void* buffer, SInt32 numBytes) = 0;
};

#endif
