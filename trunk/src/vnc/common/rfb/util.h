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

//
// util.h - miscellaneous useful bits
//

#ifndef __RFB_UTIL_H__
#define __RFB_UTIL_H__

#include <limits.h>
#include <string.h>

namespace rfb {

  // -=- Class to handle cleanup of arrays of characters
  class CharArray {
  public:
    CharArray() : buf(0) {}
    CharArray(char* str) : buf(str) {} // note: assumes ownership
    CharArray(int len) {
      buf = new char[len];
    }
    ~CharArray() {
      delete [] buf;
    }
    // Get the buffer pointer & clear it (i.e. caller takes ownership)
    char* takeBuf() {char* tmp = buf; buf = 0; return tmp;}
    void replaceBuf(char* b) {delete [] buf; buf = b;}
    char* buf;
  private:
    CharArray(const CharArray&);
    CharArray& operator=(const CharArray&);
  };

  char* strDup(const char* s);
  void strFree(char* s);

  // Returns true if split successful.  Returns false otherwise.
  // ALWAYS *copies* first part of string to out1 buffer.
  // If limiter not found, leaves out2 alone (null) and just copies to out1.
  // If out1 or out2 non-zero, calls strFree and zeroes them.
  // If fromEnd is true, splits at end of string rather than beginning.
  // Either out1 or out2 may be null, in which case the split will not return
  // that part of the string.  Obviously, setting both to 0 is not useful...
  bool strSplit(const char* src, const char limiter, char** out1, char** out2, bool fromEnd=false);

  // Returns true if src contains c
  bool strContains(const char* src, char c);

  // Copies src to dest, up to specified length-1, and guarantees termination
  void strCopy(char* dest, const char* src, int destlen);


  // HELPER functions for timeout handling

  // soonestTimeout() is a function to help work out the soonest of several
  //   timeouts.
  inline void soonestTimeout(int* timeout, int newTimeout) {
    if (newTimeout && (!*timeout || newTimeout < *timeout))
      *timeout = newTimeout;
  }

  // secsToMillis() turns seconds into milliseconds, capping the value so it
  //   can't wrap round and become -ve
  inline int secsToMillis(int secs) {
    return (secs < 0 || secs > (INT_MAX/1000) ? INT_MAX : secs * 1000);
  }
}

// Some platforms (e.g. Windows) include max() and min() macros in their
// standard headers, but they are also standard C++ template functions, so some
// C++ headers will undefine them.  So we steer clear of the names min and max
// and define __rfbmin and __rfbmax instead.

#ifndef __rfbmax
#define __rfbmax(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __rfbmin
#define __rfbmin(a,b) (((a) < (b)) ? (a) : (b))
#endif

#endif
