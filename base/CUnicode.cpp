#include "CUnicode.h"
#include <string.h>

//
// local utility functions
//

inline
static
UInt16
decode16(const UInt8* n)
{
	union x16 {
		UInt8	n8[2];
		UInt16	n16;
	} c;
	c.n8[0] = n[0];
	c.n8[1] = n[1];
	return c.n16;
}

inline
static
UInt32
decode32(const UInt8* n)
{
	union x32 {
		UInt8	n8[4];
		UInt32	n32;
	} c;
	c.n8[0] = n[0];
	c.n8[1] = n[1];
	c.n8[2] = n[2];
	c.n8[3] = n[3];
	return c.n32;
}

//
// CUnicode
//

UInt32					CUnicode::s_invalid = 0x0000ffff;

CString
CUnicode::UTF8ToUCS2(const CString& src)
{
	// get size of input string and reserve some space in output.
	// include UTF8's nul terminator.
	UInt32 n = src.size() + 1;
	CString dst;
	dst.reserve(2 * n);

	// convert each character
	const UInt8* data = reinterpret_cast<const UInt8*>(src.c_str());
	while (n > 0) {
		UInt32 c = fromUTF8(data, n);
		if (c != s_invalid && c < 0x0000ffff) {
			UInt16 ucs2 = static_cast<UInt16>(c);
			dst.append(reinterpret_cast<const char*>(&ucs2), 2);
		}
	}

	return dst;
}

CString
CUnicode::UTF8ToUCS4(const CString& src)
{
	// get size of input string and reserve some space in output.
	// include UTF8's nul terminator.
	UInt32 n = src.size() + 1;
	CString dst;
	dst.reserve(4 * n);

	// convert each character
	const UInt8* data = reinterpret_cast<const UInt8*>(src.c_str());
	while (n > 0) {
		UInt32 c = fromUTF8(data, n);
		if (c != s_invalid) {
			dst.append(reinterpret_cast<const char*>(&c), 4);
		}
	}

	return dst;
}

CString
CUnicode::UTF8ToUTF16(const CString& src)
{
	// get size of input string and reserve some space in output.
	// include UTF8's nul terminator.
	UInt32 n = src.size() + 1;
	CString dst;
	dst.reserve(2 * n);

	// convert each character
	const UInt8* data = reinterpret_cast<const UInt8*>(src.c_str());
	while (n > 0) {
		UInt32 c = fromUTF8(data, n);
		if (c != s_invalid && c < 0x0010ffff) {
			if (c < 0x00010000) {
				UInt16 ucs2 = static_cast<UInt16>(c);
				dst.append(reinterpret_cast<const char*>(&ucs2), 2);
			}
			else {
				c -= 0x00010000;
				UInt16 utf16h = static_cast<UInt16>(c >> 10) + 0xd800;
				UInt16 utf16l = (static_cast<UInt16>(c) & 0x03ff) + 0xdc00;
				dst.append(reinterpret_cast<const char*>(&utf16h), 2);
				dst.append(reinterpret_cast<const char*>(&utf16l), 2);
			}
		}
	}

	return dst;
}

CString
CUnicode::UTF8ToUTF32(const CString& src)
{
	// FIXME -- should ensure dst has no characters over U-0010FFFF
	return UTF8ToUCS4(src);
}

CString
CUnicode::UCS2ToUTF8(const CString& src)
{
	UInt32 n = src.size() >> 1;
	return doUCS2ToUTF8(reinterpret_cast<const UInt8*>(src.data()), n);
}

CString
CUnicode::UCS4ToUTF8(const CString& src)
{
	UInt32 n = src.size() >> 2;
	return doUCS4ToUTF8(reinterpret_cast<const UInt8*>(src.data()), n);
}

CString
CUnicode::UTF16ToUTF8(const CString& src)
{
	UInt32 n = src.size() >> 1;
	return doUTF16ToUTF8(reinterpret_cast<const UInt8*>(src.data()), n);
}

CString
CUnicode::UTF32ToUTF8(const CString& src)
{
	UInt32 n = src.size() >> 2;
	return doUTF32ToUTF8(reinterpret_cast<const UInt8*>(src.data()), n);
}

CString
CUnicode::UTF8ToText(const CString& src)
{
	// convert to wide char
	wchar_t* tmp = UTF8ToWideChar(src);

	// get length of multibyte string
	mbstate_t state;
	memset(&state, 0, sizeof(state));
	const wchar_t* scratch = tmp;
	size_t len = wcsrtombs(NULL, &scratch, 0, &state);
	if (len == (size_t)-1) {
		// invalid character in src
		delete[] tmp;
		return CString();
	}

	// convert to multibyte
	scratch = tmp;
	char* dst = new char[len + 1];
	wcsrtombs(dst, &scratch, len + 1, &state);
	CString text(dst);

	// clean up
	delete[] dst;
	delete[] tmp;

	return text;
}

CString
CUnicode::textToUTF8(const CString& src)
{
	// get length of wide char string
	mbstate_t state;
	memset(&state, 0, sizeof(state));
	const char* scratch = src.c_str();
	size_t len = mbsrtowcs(NULL, &scratch, 0, &state);
	if (len == (size_t)-1) {
		// invalid character in src
		return CString();
	}

	// convert multibyte to wide char
	scratch = src.c_str();
	wchar_t* dst = new wchar_t[len + 1];
	mbsrtowcs(dst, &scratch, len + 1, &state);

	// convert to UTF8
	CString utf8 = wideCharToUTF8(dst);

	// clean up
	delete[] dst;

	return utf8;
}

wchar_t*
CUnicode::UTF8ToWideChar(const CString& src)
{
	// convert to platform's wide character encoding.
	// note -- this must include a wide nul character (independent of
	// the CString's nul character).
#if WINDOWS_LIKE
	CString tmp = UTF8ToUCS16(src);
	UInt32 size = tmp.size() >> 1;
#elif UNIX_LIKE
	CString tmp = UTF8ToUCS4(src);
	UInt32 size = tmp.size() >> 2;
#endif

	// copy to a wchar_t array
	wchar_t* dst = new wchar_t[size];
	::memcpy(dst, src.data(), sizeof(wchar_t) * size);
	return dst;
}

CString
CUnicode::wideCharToUTF8(const wchar_t* src)
{
	// convert from platform's wide character encoding.
	// note -- this must include a wide nul character (independent of
	// the CString's nul character).
#if WINDOWS_LIKE
	return doUCS16ToUTF8(reinterpret_cast<const UInt8*>(src), wcslen(src));
#elif UNIX_LIKE
	return doUCS4ToUTF8(reinterpret_cast<const UInt8*>(src), wcslen(src));
#endif
}

CString
CUnicode::doUCS2ToUTF8(const UInt8* data, UInt32 n)
{
	// make some space
	CString dst;
	dst.reserve(n);

	// convert each character
	for (; n > 0; data += 2, --n) {
		UInt32 c = decode16(data);
		toUTF8(dst, c);
	}

	// remove extra trailing nul
	if (dst.size() > 0 && dst[dst.size() - 1] == '\0') {
		dst.resize(dst.size() - 1);
	}

	return dst;
}

CString
CUnicode::doUCS4ToUTF8(const UInt8* data, UInt32 n)
{
	// make some space
	CString dst;
	dst.reserve(n);

	// convert each character
	for (; n > 0; data += 4, --n) {
		UInt32 c = decode32(data);
		toUTF8(dst, c);
	}

	// remove extra trailing nul
	if (dst.size() > 0 && dst[dst.size() - 1] == '\0') {
		dst.resize(dst.size() - 1);
	}

	return dst;
}

CString
CUnicode::doUTF16ToUTF8(const UInt8* data, UInt32 n)
{
	// make some space
	CString dst;
	dst.reserve(n);

	// convert each character
	for (; n > 0; data += 2, --n) {
		UInt32 c = decode16(data);
		if (c < 0x0000d800 || c > 0x0000dfff) {
			toUTF8(dst, c);
		}
		else if (n == 1) {
			// error -- missing second word
		}
		else if (c >= 0x0000d800 && c <= 0x0000dbff) {
			UInt32 c2 = decode16(data);
			data += 2;
			--n;
			if (c2 < 0x0000dc00 || c2 > 0x0000dfff) {
				// error -- [d800,dbff] not followed by [dc00,dfff]
			}
			else {
				c = (((c - 0x0000d800) << 10) | (c2 - 0x0000dc00)) + 0x00010000;
				toUTF8(dst, c);
			}
		}
		else {
			// error -- [dc00,dfff] without leading [d800,dbff]
		}
	}

	// remove extra trailing nul
	if (dst.size() > 0 && dst[dst.size() - 1] == '\0') {
		dst.resize(dst.size() - 1);
	}

	return dst;
}

CString
CUnicode::doUTF32ToUTF8(const UInt8* data, UInt32 n)
{
	// FIXME -- should check that src has no characters over U-0010FFFF
	return doUCS4ToUTF8(data, n);
}

UInt32
CUnicode::fromUTF8(const UInt8*& data, UInt32& n)
{
	assert(data != NULL);
	assert(n    != 0);

	// compute character encoding length, checking for overlong
	// sequences (i.e. characters that don't use the shortest
	// possible encoding).
	UInt32 size;
	if (data[0] < 0x80) {
		// 0xxxxxxx
		size = 1;
	}
	else if (data[0] < 0xc0) {
		// 10xxxxxx -- in the middle of a multibyte character.  skip
		// until we find a start byte and return error.
		do {
			--n;
			++data;
		} while (n > 0 && (data[0] & 0xc0) == 0x80);
		return s_invalid;
	}
	else if (data[0] < 0xe0) {
		// 110xxxxx
		size = 2;
	}
	else if (data[0] < 0xf0) {
		// 1110xxxx
		size = 3;
	}
	else if (data[0] < 0xf8) {
		// 11110xxx
		size = 4;
	}
	else if (data[0] < 0xfc) {
		// 111110xx
		size = 5;
	}
	else if (data[0] < 0xfe) {
		// 1111110x
		size = 6;
	}
	else {
		// invalid sequence.  dunno how many bytes to skip so skip one.
		--n;
		++data;
		return s_invalid;
	}

	// make sure we have enough data
	if (size > n) {
		data += n;
		n     = 0;
		return s_invalid;
	}

	// extract character
	UInt32 c;
	switch (size) {
	case 1:
		c = static_cast<UInt32>(data[0]);
		break;

	case 2:
		c = ((static_cast<UInt32>(data[0]) & 0x1f) <<  6) |
			((static_cast<UInt32>(data[1]) & 0x3f)      );
		break;

	case 3:
		c = ((static_cast<UInt32>(data[0]) & 0x0f) << 12) |
			((static_cast<UInt32>(data[1]) & 0x3f) <<  6) |
			((static_cast<UInt32>(data[2]) & 0x3f)      );
		break;

	case 4:
		c = ((static_cast<UInt32>(data[0]) & 0x07) << 18) |
			((static_cast<UInt32>(data[1]) & 0x3f) << 12) |
			((static_cast<UInt32>(data[1]) & 0x3f) <<  6) |
			((static_cast<UInt32>(data[1]) & 0x3f)      );
		break;

	case 5:
		c = ((static_cast<UInt32>(data[0]) & 0x03) << 24) |
			((static_cast<UInt32>(data[1]) & 0x3f) << 18) |
			((static_cast<UInt32>(data[1]) & 0x3f) << 12) |
			((static_cast<UInt32>(data[1]) & 0x3f) <<  6) |
			((static_cast<UInt32>(data[1]) & 0x3f)      );
		break;

	case 6:
		c = ((static_cast<UInt32>(data[0]) & 0x01) << 30) |
			((static_cast<UInt32>(data[1]) & 0x3f) << 24) |
			((static_cast<UInt32>(data[1]) & 0x3f) << 18) |
			((static_cast<UInt32>(data[1]) & 0x3f) << 12) |
			((static_cast<UInt32>(data[1]) & 0x3f) <<  6) |
			((static_cast<UInt32>(data[1]) & 0x3f)      );
		break;

	default:
		assert(0 && "invalid size");
	}

	// update parameters
	data += size;
	n    -= size;

	// check for characters that didn't use the smallest possible encoding
	static UInt32 s_minChar[] = {
		0,
		0x00000000,
		0x00000080,
		0x00000800,
		0x00010000,
		0x00200000,
		0x04000000
	};
	if (c < s_minChar[size]) {
		return s_invalid;
	}

	// check that all bytes after the first have the pattern 10xxxxxx.
	UInt8 a = 0x80;
	switch (size) {
	case 6:
		a |= data[5];
		// fall through

	case 5:
		a |= data[4];
		// fall through

	case 4:
		a |= data[3];
		// fall through

	case 3:
		a |= data[2];
		// fall through

	case 2:
		a |= data[1];
	}
	if ((a & 0xc0) != 0x80) {
		return s_invalid;
	}

	return c;
}

void
CUnicode::toUTF8(CString& dst, const UInt32 c)
{
	UInt8 data[6];

	if (c < 0x00000080) {
		data[0] = static_cast<UInt8>(c);
		dst.append(reinterpret_cast<char*>(data), 1);
	}
	else if (c < 0x00000800) {
		data[0] = static_cast<UInt8>((c >>  6) & 0x0000001f) + 0xc0;
		data[1] = static_cast<UInt8>(c         & 0x0000003f) + 0x80;
		dst.append(reinterpret_cast<char*>(data), 2);
	}
	else if (c < 0x00010000) {
		data[0] = static_cast<UInt8>((c >> 12) & 0x0000000f) + 0xe0;
		data[1] = static_cast<UInt8>((c >>  6) & 0x0000003f) + 0x80;
		data[2] = static_cast<UInt8>(c         & 0x0000003f) + 0x80;
		dst.append(reinterpret_cast<char*>(data), 3);
	}
	else if (c < 0x00200000) {
		data[0] = static_cast<UInt8>((c >> 18) & 0x00000007) + 0xf0;
		data[1] = static_cast<UInt8>((c >> 12) & 0x0000003f) + 0x80;
		data[2] = static_cast<UInt8>((c >>  6) & 0x0000003f) + 0x80;
		data[3] = static_cast<UInt8>(c         & 0x0000003f) + 0x80;
		dst.append(reinterpret_cast<char*>(data), 4);
	}
	else if (c < 0x04000000) {
		data[0] = static_cast<UInt8>((c >> 24) & 0x00000003) + 0xf8;
		data[1] = static_cast<UInt8>((c >> 18) & 0x0000003f) + 0x80;
		data[2] = static_cast<UInt8>((c >> 12) & 0x0000003f) + 0x80;
		data[3] = static_cast<UInt8>((c >>  6) & 0x0000003f) + 0x80;
		data[4] = static_cast<UInt8>(c         & 0x0000003f) + 0x80;
		dst.append(reinterpret_cast<char*>(data), 5);
	}
	else if (c < 0x80000000) {
		data[0] = static_cast<UInt8>((c >> 30) & 0x00000001) + 0xfc;
		data[1] = static_cast<UInt8>((c >> 24) & 0x0000003f) + 0x80;
		data[2] = static_cast<UInt8>((c >> 18) & 0x0000003f) + 0x80;
		data[3] = static_cast<UInt8>((c >> 12) & 0x0000003f) + 0x80;
		data[4] = static_cast<UInt8>((c >>  6) & 0x0000003f) + 0x80;
		data[5] = static_cast<UInt8>(c         & 0x0000003f) + 0x80;
		dst.append(reinterpret_cast<char*>(data), 6);
	}
	else {
		// invalid character
	}
}
