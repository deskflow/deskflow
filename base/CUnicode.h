#ifndef CUNICODE_H
#define CUNICODE_H

#include "CString.h"
#include "BasicTypes.h"
#include <wchar.h>

class CUnicode {
public:
	// returns true iff the string contains a valid sequence of UTF-8
	// encoded characters.
	static bool			isUTF8(const CString&);

	// convert from UTF-8 encoding to other encodings.  if errors is
	// not NULL then it gets true if any characters could not be
	// encoded in the target encoding and false otherwise.  note
	// that decoding errors do not set errors to error.  UTF8ToText()
	// converts to the current locale's (multibyte) encoding.
	static CString		UTF8ToUCS2(const CString&, bool* errors = NULL);
	static CString		UTF8ToUCS4(const CString&, bool* errors = NULL);
	static CString		UTF8ToUTF16(const CString&, bool* errors = NULL);
	static CString		UTF8ToUTF32(const CString&, bool* errors = NULL);
	static CString		UTF8ToText(const CString&, bool* errors = NULL);

	// convert from some encoding to UTF-8.  if errors is not NULL
	// then it gets true if any characters could not be decoded and
	// false otherwise.  textToUTF8() converts from the current
	// locale's (multibyte) encoding.
	static CString		UCS2ToUTF8(const CString&, bool* errors = NULL);
	static CString		UCS4ToUTF8(const CString&, bool* errors = NULL);
	static CString		UTF16ToUTF8(const CString&, bool* errors = NULL);
	static CString		UTF32ToUTF8(const CString&, bool* errors = NULL);
	static CString		textToUTF8(const CString&, bool* errors = NULL);

private:
	// convert UTF8 to wchar_t string (using whatever encoding is native
	// to the platform).  caller must delete[] the returned string.  the
	// string is *not* nul terminated;  the length (in characters) is
	// returned in size.
	static wchar_t*		UTF8ToWideChar(const CString&,
							UInt32& size, bool* errors);

	// convert nul terminated wchar_t string (in platform's native
	// encoding) to UTF8.
	static CString		wideCharToUTF8(const wchar_t*,
							UInt32 size, bool* errors);

	// internal conversion to UTF8
	static CString		doUCS2ToUTF8(const UInt8* src, UInt32 n, bool* errors);
	static CString		doUCS4ToUTF8(const UInt8* src, UInt32 n, bool* errors);
	static CString		doUTF16ToUTF8(const UInt8* src, UInt32 n, bool* errors);
	static CString		doUTF32ToUTF8(const UInt8* src, UInt32 n, bool* errors);

	// convert characters to/from UTF8
	static UInt32		fromUTF8(const UInt8*& src, UInt32& size);
	static void			toUTF8(CString& dst, UInt32 c, bool* errors);

private:
	static UInt32		s_invalid;
	static UInt32		s_replacement;
};

#endif
