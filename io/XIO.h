#ifndef XIO_H
#define XIO_H

#include "XBase.h"
#include "BasicTypes.h"

class XIO : public XBase { };

class XIOErrno : public XIO, public MXErrno {
  public:
	XIOErrno();
	XIOErrno(int);
};

class XIOClose: public XIOErrno {
  protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

class XIOClosed : public XIO {
  protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

class XIOEndOfStream : public XIO {
  protected:
	// XBase overrides
	virtual CString		getWhat() const throw();
};

#endif
