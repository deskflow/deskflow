#include "stdpre.h"
#if !defined(CONFIG_PLATFORM_LINUX)
#include <istream>
#else
#include <iostream>
#endif
#include "stdpost.h"

#if defined(CONFIG_PLATFORM_WIN32) && defined(_MSC_VER)
inline
std::istream& operator>>(std::istream& s, SInt8& i)
{ return s >> (signed char&)i; }
inline
std::istream& operator>>(std::istream& s, SInt16& i)
{ return s >> (short&)i; }
inline
std::istream& operator>>(std::istream& s, SInt32& i)
{ return s >> (int&)i; }
inline
std::istream& operator>>(std::istream& s, UInt8& i)
{ return s >> (unsigned char&)i; }
inline
std::istream& operator>>(std::istream& s, UInt16& i)
{ return s >> (unsigned short&)i; }
inline
std::istream& operator>>(std::istream& s, UInt32& i)
{ return s >> (unsigned int&)i; }
#endif
