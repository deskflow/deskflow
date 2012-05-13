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
#include <rfb/secTypes.h>
#include <rfb/util.h>

int rfb::secTypeNum(const char* name)
{
  if (strcasecmp(name, "None") == 0)       return secTypeNone;
  if (strcasecmp(name, "VncAuth") == 0)    return secTypeVncAuth;
  if (strcasecmp(name, "RA2") == 0)        return secTypeRA2;
  if (strcasecmp(name, "RA2ne") == 0)      return secTypeRA2ne;
  if (strcasecmp(name, "SSPI") == 0)       return secTypeSSPI;
  if (strcasecmp(name, "SSPIne") == 0)       return secTypeSSPIne;
  return secTypeInvalid;
}

const char* rfb::secTypeName(int num)
{
  switch (num) {
  case secTypeNone:       return "None";
  case secTypeVncAuth:    return "VncAuth";
  case secTypeRA2:        return "RA2";
  case secTypeRA2ne:      return "RA2ne";
  case secTypeSSPI:       return "SSPI";
  case secTypeSSPIne:     return "SSPIne";
  default:                return "[unknown secType]";
  }
}

bool rfb::secTypeEncrypts(int num)
{
  switch (num) {
  case secTypeRA2:
  case secTypeSSPI:
    return true;
  default:
    return false;
  }
}

std::list<int> rfb::parseSecTypes(const char* types_)
{
  std::list<int> result;
  CharArray types(strDup(types_)), type;
  while (types.buf) {
    strSplit(types.buf, ',', &type.buf, &types.buf);
    int typeNum = secTypeNum(type.buf);
    if (typeNum != secTypeInvalid)
      result.push_back(typeNum);
  }
  return result;
}
