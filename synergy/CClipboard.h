#ifndef CCLIPBOARD_H
#define CCLIPBOARD_H

//
// CClipboard -- stores clipboard data in a memory buffer
//

#include "IClipboard.h"
#include "CString.h"

class CClipboard : public IClipboard {
public:
	CClipboard();
	virtual ~CClipboard();

	// manipulators

	// unmarshall clipboard data
	void				unmarshall(const CString& data, Time);

	// accessors

	// marshall clipboard data
	CString				marshall() const;

	// IClipboard overrides
	virtual bool		empty();
	virtual void		add(EFormat, const CString& data);
	virtual bool		open(Time) const;
	virtual void		close() const;
	virtual Time		getTime() const;
	virtual bool		has(EFormat) const;
	virtual CString		get(EFormat) const;

	// accessors

	// transfer all the data in one clipboard to another.  the
	// clipboards can be of any concrete clipboard type (and
	// they don't have to be the same type).  this also sets
	// the timestamp to time, if provided, or the time in src.
	// returns true iff the copy succeeded.
	static bool			copy(IClipboard* dst, const IClipboard* src);
	static bool			copy(IClipboard* dst, const IClipboard* src, Time);

private:
	UInt32				readUInt32(const char*) const;
	void				writeUInt32(CString*, UInt32) const;

private:
	mutable bool		m_open;
	mutable Time		m_time;
	bool				m_owner;
	Time				m_timeOwned;
	bool				m_added[kNumFormats];
	CString				m_data[kNumFormats];
};

#endif
