#ifndef CUNICODE_H
#define CUNICODE_H

#include "CString.h"
#include "BasicTypes.h"
#include <wchar.h>

class CUnicode {
public:
	static CString		UTF8ToUCS2(const CString&);
	static CString		UTF8ToUCS4(const CString&);
	static CString		UTF8ToUTF16(const CString&);
	static CString		UTF8ToUTF32(const CString&);

	static CString		UCS2ToUTF8(const CString&);
	static CString		UCS4ToUTF8(const CString&);
	static CString		UTF16ToUTF8(const CString&);
	static CString		UTF32ToUTF8(const CString&);

	// convert UTF-8 to/from the current locale's encoding
	static CString		UTF8ToText(const CString&);
	static CString		textToUTF8(const CString&);

private:
	// convert UTF8 to nul terminated wchar_t string (using whatever
	// encoding is native to the platform).  caller must delete[]
	// the returned string.
	static wchar_t*		UTF8ToWideChar(const CString&);

	// convert nul terminated wchar_t string (in platform's native
	// encoding) to UTF8.
	static CString		wideCharToUTF8(const wchar_t*);

	// internal conversion to UTF8
	static CString		doUCS2ToUTF8(const UInt8* src, UInt32 n);
	static CString		doUCS4ToUTF8(const UInt8* src, UInt32 n);
	static CString		doUTF16ToUTF8(const UInt8* src, UInt32 n);
	static CString		doUTF32ToUTF8(const UInt8* src, UInt32 n);

	// convert characters to/from UTF8
	static UInt32		fromUTF8(const UInt8*& src, UInt32& size);
	static void			toUTF8(CString& dst, const UInt32 c);

private:
	static UInt32		s_invalid;
};

#endif
