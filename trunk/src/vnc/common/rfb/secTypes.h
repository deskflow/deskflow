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
// secTypes.h - constants for the various security types.
//

#ifndef __RFB_SECTYPES_H__
#define __RFB_SECTYPES_H__

#include <list>

namespace rfb {
  const int secTypeInvalid = 0;
  const int secTypeNone    = 1;
  const int secTypeVncAuth = 2;

  const int secTypeRA2     = 5;
  const int secTypeRA2ne   = 6;

  const int secTypeSSPI    = 7;
  const int secTypeSSPIne    = 8;

  const int secTypeTight   = 16;
  const int secTypeUltra   = 17;
  const int secTypeTLS     = 18;

  // result types

  const int secResultOK = 0;
  const int secResultFailed = 1;
  const int secResultTooMany = 2; // deprecated

  const char* secTypeName(int num);
  int secTypeNum(const char* name);
  bool secTypeEncrypts(int num);
  std::list<int> parseSecTypes(const char* types);
}

#endif
