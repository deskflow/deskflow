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

#include <stdio.h>
#include <rfb/KeyRemapper.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("KeyRemapper");

KeyRemapper KeyRemapper::defInstance;

#ifdef __RFB_THREADING_IMPL
static Mutex mappingLock;
#endif

void KeyRemapper::setMapping(const char* m) {
#ifdef __RFB_THREADING_IMPL
  Lock l(mappingLock);
#endif
  mapping.clear();
  while (m[0]) {
    int from, to;
    char bidi;
    const char* nextComma = strchr(m, ',');
    if (!nextComma)
      nextComma = m + strlen(m);
    if (sscanf(m, "0x%x%c>0x%x", &from,
               &bidi, &to) == 3) {
      if (bidi != '-' && bidi != '<')
        vlog.error("warning: unknown operation %c>, assuming ->", bidi);
      mapping[from] = to;
      if (bidi == '<')
        mapping[to] = from;
    } else {
      vlog.error("warning: bad mapping %.*s", nextComma-m, m);
    }
    m = nextComma;
    if (nextComma[0])
      m++;
  }
}

rdr::U32 KeyRemapper::remapKey(rdr::U32 key) const {
#ifdef __RFB_THREADING_IMPL
  Lock l(mappingLock);
#endif
  std::map<rdr::U32,rdr::U32>::const_iterator i = mapping.find(key);
  if (i != mapping.end())
    return i->second;
  return key;
}


class KeyMapParameter : public StringParameter {
public:
  KeyMapParameter()
    : StringParameter("RemapKeys", "Comma-separated list of incoming keysyms to remap.  Mappings are expressed as two hex values, prefixed by 0x, and separated by ->", "") {
    setParam(value);
  }
  bool setParam(const char* v) {
    KeyRemapper::defInstance.setMapping(v);
    return StringParameter::setParam(v);
  }
} defaultParam;


