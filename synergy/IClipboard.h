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

	// grab ownership of and clear the clipboard of all data.
	// only add() may be called between an open() and its
	// corresponding close().  if open() returns false then
	// the clipboard could not be opened or grabbed;  do not
	// call close() in that case.  iff open() returns true it
	// should have saved the timestamp.  the timestamp should
	// be zero before the first successful open.
	virtual bool		open(Time) = 0;

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

	// returns the timestamp passed to the last successful open().
	virtual Time		getTime() const = 0;

	// returns true iff the clipboard contains data in the given
	// format.
	virtual bool		has(EFormat) const = 0;

	// returns data in the given format.  rturns the empty string
	// if there is no data in that format.
	virtual CString		get(EFormat) const = 0;
};

#endif
