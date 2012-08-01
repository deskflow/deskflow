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
#include <string.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#endif
#include <rfb/encodings.h>

int rfb::encodingNum(const char* name)
{
  if (strcasecmp(name, "raw") == 0)      return encodingRaw;
  if (strcasecmp(name, "copyRect") == 0) return encodingCopyRect;
  if (strcasecmp(name, "RRE") == 0)      return encodingRRE;
  if (strcasecmp(name, "CoRRE") == 0)    return encodingCoRRE;
  if (strcasecmp(name, "hextile") == 0)  return encodingHextile;
  if (strcasecmp(name, "ZRLE") == 0)     return encodingZRLE;
  return -1;
}

const char* rfb::encodingName(unsigned int num)
{
  switch (num) {
  case encodingRaw:      return "raw";
  case encodingCopyRect: return "copyRect";
  case encodingRRE:      return "RRE";
  case encodingCoRRE:    return "CoRRE";
  case encodingHextile:  return "hextile";
  case encodingZRLE:     return "ZRLE";
  default:               return "[unknown encoding]";
  }
}
