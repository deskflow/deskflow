#ifndef ICLIPBOARD_H
#define ICLIPBOARD_H

#include "IInterface.h"
#include "BasicTypes.h"

class CString;

class IClipboard : public IInterface {
public:
	// timestamp type.  timestamps are in milliseconds from some
	// arbitrary starting time.  timestamps will wrap around to 0
	// after about 49 3/4 days.
	typedef UInt32 Time;

	// known clipboard formats.  kNumFormats must be last and
	// formats must be sequential starting from zero.
	enum EFormat { kText, kNumFormats };

	// manipulators

	// take ownership of the clipboard and clear all data from it.
	// must be called between an open() and close().  if returns
	// false then the clipboard ownership could not be taken;  the
	// clipboard should not be emptied in this case.
	virtual bool		empty() = 0;

	// add data in the given format to the clipboard.  data is
	// passed as a string but the contents are generally not
	// interpreted.  may only be called after a successful empty().
	virtual void		add(EFormat, const CString& data) = 0;

	// accessors

	// open the clipboard.  return true iff the clipboard could
	// be opened.  if open() returns true then it must be followed
	// by a close() at some later time;  if it returns false then
	// close() must not be called.
	virtual bool		open(Time) const = 0;

	// close the clipboard.  close() must match a preceding open().
	// this signals that the clipboard has been filled with all the
	// necessary data.  it does not mean the clipboard ownership
	// should be released.
	virtual void		close() const = 0;

	// returns the timestamp passed to the last successful open().
	virtual Time		getTime() const = 0;

	// returns true iff the clipboard contains data in the given
	// format.  must be called between an open() and close().
	virtual bool		has(EFormat) const = 0;

	// returns data in the given format.  rturns the empty string
	// if there is no data in that format.  must be called between
	// an open() and close().
	virtual CString		get(EFormat) const = 0;
};

#endif
