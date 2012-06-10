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

// -=- Security.cxx

#include <rfb_win32/Security.h>
#include <rfb_win32/DynamicFn.h>
#include <rfb/LogWriter.h>

#include <lmcons.h>
#include <Accctrl.h>
#include <list>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("SecurityWin32");


Trustee::Trustee(const TCHAR* name,
                 TRUSTEE_FORM form,
                 TRUSTEE_TYPE type) {
  pMultipleTrustee = 0;
  MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
  TrusteeForm = form;
  TrusteeType = type;
  ptstrName = (TCHAR*)name;
}


ExplicitAccess::ExplicitAccess(const TCHAR* name,
                               TRUSTEE_FORM type,
                               DWORD perms,
                               ACCESS_MODE mode,
                               DWORD inherit) {
  Trustee = rfb::win32::Trustee(name, type);
  grfAccessPermissions = perms;
  grfAccessMode = mode;
  grfInheritance = inherit;
}


AccessEntries::AccessEntries() : entries(0), entry_count(0) {}

AccessEntries::~AccessEntries() {
  delete [] entries;
}

void AccessEntries::allocMinEntries(int count) {
  if (count > entry_count) {
    EXPLICIT_ACCESS* new_entries = new EXPLICIT_ACCESS[entry_count+1];
    if (entries) {
      memcpy(new_entries, entries, sizeof(EXPLICIT_ACCESS) * entry_count);
      delete entries;
    }
    entries = new_entries;
  }
}

void AccessEntries::addEntry(const TCHAR* trusteeName,
                             DWORD permissions,
                             ACCESS_MODE mode) {
  allocMinEntries(entry_count+1);
  ZeroMemory(&entries[entry_count], sizeof(EXPLICIT_ACCESS));
  entries[entry_count] = ExplicitAccess(trusteeName, TRUSTEE_IS_NAME, permissions, mode);
  entry_count++;
}

void AccessEntries::addEntry(const PSID sid,
                             DWORD permissions,
                             ACCESS_MODE mode) {
  allocMinEntries(entry_count+1);
  ZeroMemory(&entries[entry_count], sizeof(EXPLICIT_ACCESS));
  entries[entry_count] = ExplicitAccess((TCHAR*)sid, TRUSTEE_IS_SID, permissions, mode);
  entry_count++;
}


PSID Sid::copySID(const PSID sid) {
  if (!IsValidSid(sid))
    throw rdr::Exception("invalid SID in copyPSID");
  PSID buf = (PSID)new rdr::U8[GetLengthSid(sid)];
  if (!CopySid(GetLengthSid(sid), buf, sid))
    throw rdr::SystemException("CopySid failed", GetLastError());
  return buf;
}

void Sid::setSID(const PSID sid) {
  delete [] buf;
  buf = (rdr::U8*)copySID(sid);
}

void Sid::getUserNameAndDomain(TCHAR** name, TCHAR** domain) {
  DWORD nameLen = 0;
  DWORD domainLen = 0;
  SID_NAME_USE use;
  LookupAccountSid(0, (PSID)buf, 0, &nameLen, 0, &domainLen, &use);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    throw rdr::SystemException("Unable to determine SID name lengths", GetLastError());
  vlog.info("nameLen=%d, domainLen=%d, use=%d", nameLen, domainLen, use);
  *name = new TCHAR[nameLen];
  *domain = new TCHAR[domainLen];
  if (!LookupAccountSid(0, (PSID)buf, *name, &nameLen, *domain, &domainLen, &use))
    throw rdr::SystemException("Unable to lookup account SID", GetLastError());
}


Sid::Administrators::Administrators() {
  PSID sid = 0;
  SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
  if (!AllocateAndInitializeSid(&ntAuth, 2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &sid)) 
    throw rdr::SystemException("Sid::Administrators", GetLastError());
  setSID(sid);
  FreeSid(sid);
}

Sid::SYSTEM::SYSTEM() {
  PSID sid = 0;
  SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
  if (!AllocateAndInitializeSid(&ntAuth, 1,
                                SECURITY_LOCAL_SYSTEM_RID,
                                0, 0, 0, 0, 0, 0, 0, &sid))
          throw rdr::SystemException("Sid::SYSTEM", GetLastError());
  setSID(sid);
  FreeSid(sid);
}

Sid::FromToken::FromToken(HANDLE h) {
  DWORD required = 0;
  GetTokenInformation(h, TokenUser, 0, 0, &required);
  rdr::U8Array tmp(required);
  if (!GetTokenInformation(h, TokenUser, tmp.buf, required, &required))
    throw rdr::SystemException("GetTokenInformation", GetLastError());
  TOKEN_USER* tokenUser = (TOKEN_USER*)tmp.buf;
  setSID(tokenUser->User.Sid);
}


PACL rfb::win32::CreateACL(const AccessEntries& ae, PACL existing_acl) {
  typedef DWORD (WINAPI *_SetEntriesInAcl_proto) (ULONG, PEXPLICIT_ACCESS, PACL, PACL*);
#ifdef UNICODE
  const char* fnName = "SetEntriesInAclW";
#else
  const char* fnName = "SetEntriesInAclA";
#endif
  DynamicFn<_SetEntriesInAcl_proto> _SetEntriesInAcl(_T("advapi32.dll"), fnName);
  if (!_SetEntriesInAcl.isValid())
    throw rdr::SystemException("CreateACL failed; no SetEntriesInAcl", ERROR_CALL_NOT_IMPLEMENTED);
  PACL new_dacl;
  DWORD result;
  if ((result = (*_SetEntriesInAcl)(ae.entry_count, ae.entries, existing_acl, &new_dacl)) != ERROR_SUCCESS)
    throw rdr::SystemException("SetEntriesInAcl", result);
  return new_dacl;
}


PSECURITY_DESCRIPTOR rfb::win32::CreateSdWithDacl(const PACL dacl) {
  SECURITY_DESCRIPTOR absSD;
  if (!InitializeSecurityDescriptor(&absSD, SECURITY_DESCRIPTOR_REVISION))
    throw rdr::SystemException("InitializeSecurityDescriptor", GetLastError());
  Sid::SYSTEM owner;
  if (!SetSecurityDescriptorOwner(&absSD, owner, FALSE))
    throw rdr::SystemException("SetSecurityDescriptorOwner", GetLastError());
  Sid::Administrators group;
  if (!SetSecurityDescriptorGroup(&absSD, group, FALSE))
    throw rdr::SystemException("SetSecurityDescriptorGroupp", GetLastError());
  if (!SetSecurityDescriptorDacl(&absSD, TRUE, dacl, FALSE))
    throw rdr::SystemException("SetSecurityDescriptorDacl", GetLastError());
  DWORD sdSize = GetSecurityDescriptorLength(&absSD);
  SecurityDescriptorPtr sd(sdSize);
  if (!MakeSelfRelativeSD(&absSD, sd, &sdSize))
    throw rdr::SystemException("MakeSelfRelativeSD", GetLastError());
  return sd.takeSD();
}
