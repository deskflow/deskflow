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

// -=- Registry.cxx

#include <rfb_win32/Registry.h>
#include <rfb_win32/Security.h>
#include <rfb_win32/DynamicFn.h>
#include <rdr/MemOutStream.h>
#include <rdr/HexOutstream.h>
#include <rdr/HexInStream.h>
#include <stdlib.h>
#include <rfb/LogWriter.h>

// These flags are required to control access control inheritance,
// but are not defined by VC6's headers.  These definitions comes
// from the Microsoft Platform SDK.
#ifndef PROTECTED_DACL_SECURITY_INFORMATION
#define PROTECTED_DACL_SECURITY_INFORMATION     (0x80000000L)
#endif
#ifndef UNPROTECTED_DACL_SECURITY_INFORMATION
#define UNPROTECTED_DACL_SECURITY_INFORMATION     (0x20000000L)
#endif


using namespace rfb;
using namespace rfb::win32;


static LogWriter vlog("Registry");


RegKey::RegKey() : key(0), freeKey(false), valueNameBufLen(0) {}

RegKey::RegKey(const HKEY k) : key(0), freeKey(false), valueNameBufLen(0) {
  LONG result = RegOpenKeyEx(k, 0, 0, KEY_ALL_ACCESS, &key);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegOpenKeyEx(HKEY)", result);
  vlog.debug("duplicated %x to %x", k, key);
  freeKey = true;
}

RegKey::RegKey(const RegKey& k) : key(0), freeKey(false), valueNameBufLen(0) {
  LONG result = RegOpenKeyEx(k.key, 0, 0, KEY_ALL_ACCESS, &key);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegOpenKeyEx(RegKey&)", result);
  vlog.debug("duplicated %x to %x", k.key, key);
  freeKey = true;
}

RegKey::~RegKey() {
  close();
}


void RegKey::setHKEY(HKEY k, bool fK) {
  vlog.debug("setHKEY(%x,%d)", k, (int)fK);
  close();
  freeKey = fK;
  key = k;
}


bool RegKey::createKey(const RegKey& root, const TCHAR* name) {
  close();
  LONG result = RegCreateKey(root.key, name, &key);
  if (result != ERROR_SUCCESS) {
    vlog.error("RegCreateKey(%x, %s): %x", root.key, name, result);
    throw rdr::SystemException("RegCreateKeyEx", result);
  }
  vlog.debug("createKey(%x,%s) = %x", root.key, (const char*)CStr(name), key);
  freeKey = true;
  return true;
}

void RegKey::openKey(const RegKey& root, const TCHAR* name, bool readOnly) {
  close();
  LONG result = RegOpenKeyEx(root.key, name, 0, readOnly ? KEY_READ : KEY_ALL_ACCESS, &key);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegOpenKeyEx (open)", result);
  vlog.debug("openKey(%x,%s,%s) = %x", root.key, (const char*)CStr(name),
	         readOnly ? "ro" : "rw", key);
  freeKey = true;
}

void RegKey::setDACL(const PACL acl, bool inherit) {
  DWORD result;
  typedef DWORD (WINAPI *_SetSecurityInfo_proto) (HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION, PSID, PSID, PACL, PACL);
  DynamicFn<_SetSecurityInfo_proto> _SetSecurityInfo(_T("advapi32.dll"), "SetSecurityInfo");
  if (!_SetSecurityInfo.isValid())
    throw rdr::SystemException("RegKey::setDACL failed", ERROR_CALL_NOT_IMPLEMENTED);
  if ((result = (*_SetSecurityInfo)(key, SE_REGISTRY_KEY,
    DACL_SECURITY_INFORMATION |
    (inherit ? UNPROTECTED_DACL_SECURITY_INFORMATION : PROTECTED_DACL_SECURITY_INFORMATION),
    0, 0, acl, 0)) != ERROR_SUCCESS)
    throw rdr::SystemException("RegKey::setDACL failed", result);
}

void RegKey::close() {
  if (freeKey) {
    vlog.debug("RegCloseKey(%x)", key);
    RegCloseKey(key);
    key = 0;
  }
}

void RegKey::deleteKey(const TCHAR* name) const {
  LONG result = RegDeleteKey(key, name);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegDeleteKey", result);
}

void RegKey::deleteValue(const TCHAR* name) const {
  LONG result = RegDeleteValue(key, name);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegDeleteValue", result);
}

void RegKey::awaitChange(bool watchSubTree, DWORD filter, HANDLE event) const {
  LONG result = RegNotifyChangeKeyValue(key, watchSubTree, filter, event, event != 0);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegNotifyChangeKeyValue", result);
}


RegKey::operator HKEY() const {return key;}


void RegKey::setExpandString(const TCHAR* valname, const TCHAR* value) const {
  LONG result = RegSetValueEx(key, valname, 0, REG_EXPAND_SZ, (const BYTE*)value, (_tcslen(value)+1)*sizeof(TCHAR));
  if (result != ERROR_SUCCESS) throw rdr::SystemException("setExpandString", result);
}

void RegKey::setString(const TCHAR* valname, const TCHAR* value) const {
  LONG result = RegSetValueEx(key, valname, 0, REG_SZ, (const BYTE*)value, (_tcslen(value)+1)*sizeof(TCHAR));
  if (result != ERROR_SUCCESS) throw rdr::SystemException("setString", result);
}

void RegKey::setBinary(const TCHAR* valname, const void* value, int length) const {
  LONG result = RegSetValueEx(key, valname, 0, REG_BINARY, (const BYTE*)value, length);
  if (result != ERROR_SUCCESS) throw rdr::SystemException("setBinary", result);
}

void RegKey::setInt(const TCHAR* valname, int value) const {
  LONG result = RegSetValueEx(key, valname, 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
  if (result != ERROR_SUCCESS) throw rdr::SystemException("setInt", result);
}

void RegKey::setBool(const TCHAR* valname, bool value) const {
  setInt(valname, value ? 1 : 0);
}

TCHAR* RegKey::getString(const TCHAR* valname) const {return getRepresentation(valname);}
TCHAR* RegKey::getString(const TCHAR* valname, const TCHAR* def) const {
  try {
    return getString(valname);
  } catch(rdr::Exception) {
    return tstrDup(def);
  }
}

void RegKey::getBinary(const TCHAR* valname, void** data, int* length) const {
  TCharArray hex = getRepresentation(valname);
  if (!rdr::HexInStream::hexStrToBin(CStr(hex.buf), (char**)data, length))
    throw rdr::Exception("getBinary failed");
}
void RegKey::getBinary(const TCHAR* valname, void** data, int* length, void* def, int deflen) const {
  try {
    getBinary(valname, data, length);
  } catch(rdr::Exception) {
    if (deflen) {
      *data = new char[deflen];
      memcpy(*data, def, deflen);
    } else
      *data = 0;
    *length = deflen;
  }
}

int RegKey::getInt(const TCHAR* valname) const {
  TCharArray tmp = getRepresentation(valname);
  return _ttoi(tmp.buf);
}
int RegKey::getInt(const TCHAR* valname, int def) const {
  try {
    return getInt(valname);
  } catch(rdr::Exception) {
    return def;
  }
}

bool RegKey::getBool(const TCHAR* valname) const {
  return getInt(valname) > 0;
}
bool RegKey::getBool(const TCHAR* valname, bool def) const {
  return getInt(valname, def ? 1 : 0) > 0;
}

static inline TCHAR* terminateData(char* data, int length)
{
  // We must terminate the string, just to be sure.  Stupid Win32...
  int len = length/sizeof(TCHAR);
  TCharArray str(len+1);
  memcpy(str.buf, data, length);
  str.buf[len] = 0;
  return str.takeBuf();
}

TCHAR* RegKey::getRepresentation(const TCHAR* valname) const {
  DWORD type, length;
  LONG result = RegQueryValueEx(key, valname, 0, &type, 0, &length);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("get registry value length", result);
  CharArray data(length);
  result = RegQueryValueEx(key, valname, 0, &type, (BYTE*)data.buf, &length);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("get registry value", result);

  switch (type) {
  case REG_BINARY:
    {
      TCharArray hex = rdr::HexOutStream::binToHexStr(data.buf, length);
      return hex.takeBuf();
    }
  case REG_SZ:
    if (length) {
      return terminateData(data.buf, length);
    } else {
      return tstrDup(_T(""));
    }
  case REG_DWORD:
    {
      TCharArray tmp(16);
      _stprintf(tmp.buf, _T("%u"), *((DWORD*)data.buf));
      return tmp.takeBuf();
    }
  case REG_EXPAND_SZ:
    {
    if (length) {
      TCharArray str(terminateData(data.buf, length));
      DWORD required = ExpandEnvironmentStrings(str.buf, 0, 0);
      if (required==0)
        throw rdr::SystemException("ExpandEnvironmentStrings", GetLastError());
      TCharArray result(required);
      length = ExpandEnvironmentStrings(str.buf, result.buf, required);
      if (required<length)
        rdr::Exception("unable to expand environment strings");
      return result.takeBuf();
    } else {
      return tstrDup(_T(""));
    }
    }
  default:
    throw rdr::Exception("unsupported registry type");
  }
}

bool RegKey::isValue(const TCHAR* valname) const {
  try {
    TCharArray tmp = getRepresentation(valname);
    return true;
  } catch(rdr::Exception) {
    return false;
  }
}

const TCHAR* RegKey::getValueName(int i) {
  DWORD maxValueNameLen;
  LONG result = RegQueryInfoKey(key, 0, 0, 0, 0, 0, 0, 0, &maxValueNameLen, 0, 0, 0);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegQueryInfoKey", result);
  if (valueNameBufLen < maxValueNameLen + 1) {
    valueNameBufLen = maxValueNameLen + 1;
    delete [] valueName.buf;
    valueName.buf = new TCHAR[valueNameBufLen];
  }
  DWORD length = valueNameBufLen;
  result = RegEnumValue(key, i, valueName.buf, &length, NULL, 0, 0, 0);
  if (result == ERROR_NO_MORE_ITEMS) return 0;
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegEnumValue", result);
  return valueName.buf;
}

const TCHAR* RegKey::getKeyName(int i) {
  DWORD maxValueNameLen;
  LONG result = RegQueryInfoKey(key, 0, 0, 0, 0, &maxValueNameLen, 0, 0, 0, 0, 0, 0);
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegQueryInfoKey", result);
  if (valueNameBufLen < maxValueNameLen + 1) {
    valueNameBufLen = maxValueNameLen + 1;
    delete [] valueName.buf;
    valueName.buf = new TCHAR[valueNameBufLen];
  }
  DWORD length = valueNameBufLen;
  result = RegEnumKeyEx(key, i, valueName.buf, &length, NULL, 0, 0, 0);
  if (result == ERROR_NO_MORE_ITEMS) return 0;
  if (result != ERROR_SUCCESS)
    throw rdr::SystemException("RegEnumKey", result);
  return valueName.buf;
}
