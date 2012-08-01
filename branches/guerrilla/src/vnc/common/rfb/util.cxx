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
#include <rfb/util.h>

namespace rfb {

  char* strDup(const char* s) {
    if (!s) return 0;
    int l = strlen(s);
    char* r = new char[l+1];
    memcpy(r, s, l+1);
    return r;
  };

  void strFree(char* s) {
    delete [] s;
  }


  bool strSplit(const char* src, const char limiter, char** out1, char** out2, bool fromEnd) {
    CharArray out1old, out2old;
    if (out1) out1old.buf = *out1;
    if (out2) out2old.buf = *out2;
    int len = strlen(src);
    int i=0, increment=1, limit=len;
    if (fromEnd) {
      i=len-1; increment = -1; limit = -1;
    }
    while (i!=limit) {
      if (src[i] == limiter) {
        if (out1) {
          *out1 = new char[i+1];
          if (i) memcpy(*out1, src, i);
          (*out1)[i] = 0;
        }
        if (out2) {
          *out2 = new char[len-i];
          if (len-i-1) memcpy(*out2, &src[i+1], len-i-1);
          (*out2)[len-i-1] = 0;
        }
        return true;
      }
      i+=increment;
    }
    if (out1) *out1 = strDup(src);
    if (out2) *out2 = 0;
    return false;
  }

  bool strContains(const char* src, char c) {
    int l=strlen(src);
    for (int i=0; i<l; i++)
      if (src[i] == c) return true;
    return false;
  }

  void strCopy(char* dest, const char* src, int destlen) {
    if (src)
      strncpy(dest, src, destlen-1);
    dest[src ? destlen-1 : 0] = 0;
  }

};
