#ifndef CPROTOCOLUTIL_H
#define CPROTOCOLUTIL_H

#include "BasicTypes.h"
#include "XIO.h"
#include <stdarg.h>

class IInputStream;
class IOutputStream;

class CProtocolUtil {
public:
	// write formatted binary data to a stream.  fmt consists of
	// regular characters and format specifiers.  format specifiers
	// begin with %.  all characters not part of a format specifier
	// are regular and are transmitted unchanged.
	//
	// format specifiers are:
	//   %%   -- writes %
	//   %1i  -- converts integer argument to 1 byte integer
	//   %2i  -- converts integer argument to 2 byte integer in NBO
	//   %4i  -- converts integer argument to 4 byte integer in NBO
	//   %s   -- converts CString* to stream of bytes
	//   %S   -- converts integer N and const UInt8* to stream of N bytes
	static void			writef(IOutputStream*,
							const char* fmt, ...);

	// read formatted binary data from a buffer.  this performs the
	// reverse operation of writef().
	//
	// format specifiers are:
	//   %%   -- read (and discard) a %
	//   %1i  -- reads a 1 byte integer; argument is a SInt32* or UInt32*
	//   %2i  -- reads an NBO 2 byte integer;  arg is SInt32* or UInt32*
	//   %4i  -- reads an NBO 4 byte integer;  arg is SInt32* or UInt32*
	//   %s   -- reads bytes;  argument must be a CString*, *not* a char*
	static void			readf(IInputStream*,
							const char* fmt, ...);

private:
	static UInt32		getLength(const char* fmt, va_list);
	static void			writef(void*, const char* fmt, va_list);
	static UInt32		eatLength(const char** fmt);
	static void			read(IInputStream*, void*, UInt32);
};

class XIOReadMismatch : public XIO {
public:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

#endif

