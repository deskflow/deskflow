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
	void				unmarshall(const CString& data);

	// accessors

	// marshall clipboard data
	CString				marshall() const;

	// IClipboard overrides
	virtual bool		open();
	virtual void		close();
	virtual void		add(EFormat, const CString& data);
	virtual bool		has(EFormat) const;
	virtual CString		get(EFormat) const;

	// accessors

	// transfer all the data in one clipboard to another.  the
	// clipboards can be of any concrete clipboard type (and
	// they don't have to be the same type).
	static void			copy(IClipboard* dst, const IClipboard* src);

  private:
	UInt32				readUInt32(const char*) const;
	void				writeUInt32(CString*, UInt32) const;

  private:
	bool				m_added[kNumFormats];
	CString				m_data[kNumFormats];
};

#endif
