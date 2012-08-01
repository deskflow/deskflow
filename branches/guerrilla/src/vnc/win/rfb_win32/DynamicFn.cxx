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

#include <rfb_win32/DynamicFn.h>
#include <rfb_win32/TCharArray.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace win32;

static LogWriter vlog("DynamicFn");


DynamicFnBase::DynamicFnBase(const TCHAR* dllName, const char* fnName) : dllHandle(0), fnPtr(0) {
  dllHandle = LoadLibrary(dllName);
  if (!dllHandle) {
    vlog.info("DLL %s not found (%d)", (const char*)CStr(dllName), GetLastError());
    return;
  }
  fnPtr = GetProcAddress(dllHandle, fnName);
  if (!fnPtr)
    vlog.info("proc %s not found in %s (%d)", fnName, (const char*)CStr(dllName), GetLastError());
}

DynamicFnBase::~DynamicFnBase() {
  if (dllHandle)
    FreeLibrary(dllHandle);
}


