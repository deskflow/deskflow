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

#ifndef __RFB_HOSTNAME_H__
#define __RFB_HOSTNAME_H__

#include <stdlib.h>
#include <rdr/Exception.h>
#include <rfb/util.h>

namespace rfb {

  static void getHostAndPort(const char* hi, char** host, int* port, int basePort=5900) {
    CharArray portBuf;
    CharArray hostBuf;
    if (hi[0] == '[') {
      if (!strSplit(&hi[1], ']', &hostBuf.buf, &portBuf.buf))
        throw rdr::Exception("unmatched [ in host");
    } else {
      portBuf.buf = strDup(hi);
    }
    if (strSplit(portBuf.buf, ':', hostBuf.buf ? 0 : &hostBuf.buf, &portBuf.buf)) {
      if (portBuf.buf[0] == ':') {
        *port = atoi(&portBuf.buf[1]);
      } else {
        *port = atoi(portBuf.buf);
        if (*port < 100) *port += basePort;
      }
    } else {
      *port = basePort;
    }
    if (strlen(hostBuf.buf) == 0)
      *host = strDup("localhost");
    else
      *host = hostBuf.takeBuf();
  }

};

#endif // __RFB_HOSTNAME_H__
