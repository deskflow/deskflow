#ifndef ICLIPBOARD_H
#define ICLIPBOARD_H

#include "IInterface.h"
#include "BasicTypes.h"

class CString;

class IClipboard : public IInterface {
  public:
	enum EFormat { kText, kNum };

	// manipulators

	// grab ownership of and clear the clipboard of all data.
	// only add() may be called between an open() and its
	// corresponding close().
	virtual void		open() = 0;

	// close the clipboard.  close() must match a preceding open().
	// this signals that the clipboard has been filled with all the
	// necessary data.  it does not mean the clipboard ownership
	// should be released.
	virtual void		close() = 0;

	// add data in the given format to the clipboard.  data is
	// passed as a string but the contents are generally not
	// interpreted.  may only be called between an open() and
	// a close().
	virtual void		add(EFormat, const CString& data) = 0;

	// accessors

	// returns true iff the clipboard contains data in the given
	// format.
	virtual bool		has(EFormat) const = 0;

	// returns data in the given format.  rturns the empty string
	// if there is no data in that format.
	virtual CString		get(EFormat) const = 0;
};

#endif

