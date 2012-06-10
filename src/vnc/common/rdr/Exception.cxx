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
#include <rdr/Exception.h>
#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
//#include <winsock2.h>
#endif

using namespace rdr;

SystemException::SystemException(const char* s, int err_)
  : Exception(s), err(err_)
{
  strncat(str_, ": ", len-1-strlen(str_));
#ifdef _WIN32
  // Windows error messages are crap, so use unix ones for common errors.
  const char* msg = 0;
  switch (err) {
  case WSAECONNREFUSED: msg = "Connection refused";       break;
  case WSAETIMEDOUT:    msg = "Connection timed out";     break;
  case WSAECONNRESET:   msg = "Connection reset by peer"; break;
  case WSAECONNABORTED: msg = "Connection aborted";       break;
  }
  if (msg) {
    strncat(str_, msg, len-1-strlen(str_));
  } else {
#ifdef UNICODE
    WCHAR* tmsg = new WCHAR[len-strlen(str_)];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  0, err, 0, tmsg, len-1-strlen(str_), 0);
    WideCharToMultiByte(CP_ACP, 0, tmsg, wcslen(tmsg)+1,
		              str_+strlen(str_), len-strlen(str_), 0, 0);
    delete [] tmsg;
#else
    char* currStr = str_+strlen(str_);
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  0, err, 0, currStr, len-1-strlen(str_), 0);
#endif
    int l = strlen(str_);
    if ((l >= 2) && (str_[l-2] == '\r') && (str_[l-1] == '\n'))
      str_[l-2] = 0;
  }

#else
  strncat(str_, strerror(err), len-1-strlen(str_));
#endif
  strncat(str_, " (", len-1-strlen(str_));
  char buf[20];
#ifdef WIN32
  if (err < 0)
    sprintf(buf, "%x", err);
  else
#endif
    sprintf(buf,"%d",err);
  strncat(str_, buf, len-1-strlen(str_));
  strncat(str_, ")", len-1-strlen(str_));
}
