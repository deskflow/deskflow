#ifndef XBASE_H
#define XBASE_H

#include "CString.h"
#include <exception>

class XBase : public std::exception {
  public:
	XBase();
	virtual ~XBase();

	// accessors

	// return the name of the exception type
	virtual const char*	getType() const;

	// format and return formatString by replacing positional
	// arguments (%1, %2, etc.).  default returns formatString
	// unchanged.  subclasses should document what positional
	// arguments they replace.
	virtual CString		format(const CString& formatString) const;

	// std::exception overrides
	virtual const char*	what() const;
};

#define XNAME(_n)											\
  public:													\
	virtual const char*	getType() const { return #_n; }

#endif
