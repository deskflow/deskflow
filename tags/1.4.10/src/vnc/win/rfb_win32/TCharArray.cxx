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

#include <rfb_win32/TCharArray.h>

namespace rfb {

  WCHAR* wstrDup(const WCHAR* s) {
    if (!s) return 0;
    WCHAR* t = new WCHAR[wcslen(s)+1];
    memcpy(t, s, sizeof(WCHAR)*(wcslen(s)+1));
    return t;
  }
  void wstrFree(WCHAR* s) {delete [] s;}

  char* strDup(const WCHAR* s) {
    if (!s) return 0;
    int len = wcslen(s);
    char* t = new char[len+1];
    t[WideCharToMultiByte(CP_ACP, 0, s, len, t, len, 0, 0)] = 0;
    return t;
  }

  WCHAR* wstrDup(const char* s) {
    if (!s) return 0;
    int len = strlen(s);
    WCHAR* t = new WCHAR[len+1];
    t[MultiByteToWideChar(CP_ACP, 0, s, len, t, len)] = 0;
    return t;
  }


  bool wstrSplit(const WCHAR* src, const WCHAR limiter, WCHAR** out1, WCHAR** out2, bool fromEnd) {
    WCharArray out1old, out2old;
    if (out1) out1old.buf = *out1;
    if (out2) out2old.buf = *out2;
    int len = wcslen(src);
    int i=0, increment=1, limit=len;
    if (fromEnd) {
      i=len-1; increment = -1; limit = -1;
    }
    while (i!=limit) {
      if (src[i] == limiter) {
        if (out1) {
          *out1 = new WCHAR[i+1];
          if (i) memcpy(*out1, src, sizeof(WCHAR)*i);
          (*out1)[i] = 0;
        }
        if (out2) {
          *out2 = new WCHAR[len-i];
          if (len-i-1) memcpy(*out2, &src[i+1], sizeof(WCHAR)*(len-i-1));
          (*out2)[len-i-1] = 0;
        }
        return true;
      }
      i+=increment;
    }
    if (out1) *out1 = wstrDup(src);
    if (out2) *out2 = 0;
    return false;
  }

  bool wstrContains(const WCHAR* src, WCHAR c) {
    int l=wcslen(src);
    for (int i=0; i<l; i++)
      if (src[i] == c) return true;
    return false;
  }

};
