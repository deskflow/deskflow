/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- TCharArray.h

// This library contains the wide-character equivalent of CharArray, named
// WCharArray.  In addition to providing wide-character equivalents of
// the char* string manipulation functions (strDup, strFree, etc), special
// versions of those functions are provided which attempt to convert from
// one format to the other.
//    e.g. char* t = "hello world"; WCHAR* w = wstrDup(t);
//    Results in w containing the wide-character text "hello world".
// For convenience, the WStr and CStr classes are also provided.  These
// accept an existing (const) WCHAR* or char* null-terminated string and
// create a read-only copy of that in the desired format.  The new copy
// will actually be the original copy if the format has not changed, otherwise
// it will be a new buffer owned by the WStr/CStr.

// In addition to providing wide character functions, this header defines
// TCHAR* handling classes & functions.  TCHAR is defined at compile time to
// either char or WCHAR.  Programs can treat this as a third data type and
// call TStr() whenever a TCHAR* is required but a char* or WCHAR* is supplied,
// and TStr will do the right thing.

#ifndef __RFB_WIN32_TCHARARRAY_H__
#define __RFB_WIN32_TCHARARRAY_H__

#include <windows.h>
#include <tchar.h>
#include <rfb/util.h>
#include <rfb/Password.h>

namespace rfb {

  // -=- String duplication and cleanup functions.
  //     These routines also handle conversion between WCHAR* and char*

  char* strDup(const WCHAR* s);
  WCHAR* wstrDup(const WCHAR* s);
  WCHAR* wstrDup(const char* s);
  void wstrFree(WCHAR* s);

  bool wstrSplit(const WCHAR* src, const WCHAR limiter, WCHAR** out1, WCHAR** out2, bool fromEnd=false);
  bool wstrContains(const WCHAR* src, WCHAR c);

  // -=- Temporary format conversion classes
  //     CStr accepts WCHAR* or char* and behaves like a char*
  //     WStr accepts WCHAR* or char* and behaves like a WCHAR*

  struct WStr {
    WStr(const char* s) : buf(wstrDup(s)), free_(true) {}
    WStr(const WCHAR* s) : buf(s), free_(false) {}
    ~WStr() {if (free_) wstrFree((WCHAR*)buf);}
    operator const WCHAR*() {return buf;}
    const WCHAR* buf;
    bool free_;
  };

  struct CStr {
    CStr(const char* s) : buf(s), free_(false) {}
    CStr(const WCHAR* s) : buf(strDup(s)), free_(true) {}
    ~CStr() {if (free_) strFree((char*)buf);}
    operator const char*() {return buf;}
    const char* buf;
    bool free_;
  };

  // -=- Class to handle cleanup of arrays of native Win32 characters
  class WCharArray {
  public:
    WCharArray() : buf(0) {}
    WCharArray(char* str) : buf(wstrDup(str)) {strFree(str);} // note: assumes ownership
    WCharArray(WCHAR* str) : buf(str) {}                      // note: assumes ownership
    WCharArray(int len) {
      buf = new WCHAR[len];
    }
    ~WCharArray() {
      delete [] buf;
    }
    // Get the buffer pointer & clear it (i.e. caller takes ownership)
    WCHAR* takeBuf() {WCHAR* tmp = buf; buf = 0; return tmp;}
    void replaceBuf(WCHAR* str) {delete [] buf; buf = str;}
    WCHAR* buf;
  };

  // -=- Wide-character-based password-buffer handler.  Zeroes the password
  //     buffer when deleted or replaced.
  class WPlainPasswd : public WCharArray {
  public:
    WPlainPasswd() {}
    WPlainPasswd(WCHAR* str) : WCharArray(str) {}
    ~WPlainPasswd() {replaceBuf(0);}
    void replaceBuf(WCHAR* str) {
      if (buf)
        memset(buf, 0, sizeof(WCHAR)*wcslen(buf));
      WCharArray::replaceBuf(str);
    }
  };
    
#ifdef _UNICODE
#define tstrDup wstrDup
#define tstrFree wstrFree
#define tstrSplit wstrSplit
#define tstrContains wstrContains
  typedef WCharArray TCharArray;
  typedef WStr TStr;
  typedef WPlainPasswd TPlainPasswd;
#else
#define tstrDup strDup
#define tstrFree strFree
#define tstrSplit strSplit
#define tstrContains strContains
  typedef CharArray TCharArray;
  typedef CStr TStr;
  typedef PlainPasswd TPlainPasswd;
#endif

};

#endif