#ifndef XBASE_H
#define XBASE_H

#include "CString.h"
#include "stdpre.h"
#include <exception>
#include "stdpost.h"

class XBase : public std::exception {
public:
	XBase();
	XBase(const CString& msg);
	virtual ~XBase();

	// std::exception overrides
	virtual const char*	what() const;

protected:
	// returns a human readable string describing the exception
	virtual CString		getWhat() const throw() = 0;

	// look up a message and format it
	virtual CString		format(const char* id,
							const char* defaultFormat, ...) const throw();

private:
	mutable CString		m_what;
};

class MXErrno {
public:
	MXErrno();
	MXErrno(int);

	// manipulators

	// accessors

	int					getErrno() const;
	const char*			getErrstr() const;

private:
	int					m_errno;
};

#endif
