// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Test-only API-shape stand-ins. No Windows ABI or operating-system I/O.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
using BYTE = uint8_t;
using WORD = uint16_t;
using DWORD = uint32_t;
using BOOL = int;
using BOOLEAN = uint8_t;
using WCHAR = wchar_t;
using HANDLE = void *;
using PSID = void *;
using SECURITY_DESCRIPTOR_CONTROL = WORD;
using SECURITY_INFORMATION = DWORD;
struct ACE_HEADER
{
  BYTE AceType = 0, AceFlags = 0;
  WORD AceSize = 12;
};
struct ACCESS_ALLOWED_ACE
{
  ACE_HEADER Header;
  DWORD Mask = 0x1f01ff, SidStart = 1;
};
struct ACL
{
  BYTE AclRevision = 2, Sbz1 = 0;
  WORD AclSize = 32, AceCount = 1, Sbz2 = 0;
};
using PACL = ACL *;
struct SECURITY_DESCRIPTOR
{
  DWORD owner = 1;
  ACL acl;
  WORD control = 0x1000;
  BOOL present = 1;
};
using PSECURITY_DESCRIPTOR = SECURITY_DESCRIPTOR *;
struct SECURITY_ATTRIBUTES
{
  DWORD nLength;
  void *lpSecurityDescriptor;
  BOOL bInheritHandle;
};
struct SID_AND_ATTRIBUTES
{
  PSID Sid;
  DWORD Attributes;
};
struct TOKEN_USER
{
  SID_AND_ATTRIBUTES User;
};
struct BY_HANDLE_FILE_INFORMATION
{
  DWORD dwFileAttributes = 0;
  DWORD unused[6]{};
  DWORD nNumberOfLinks = 1;
};
struct FILE_DISPOSITION_INFO
{
  BOOL DeleteFile;
};
struct FILE_RENAME_INFO
{
  BOOLEAN ReplaceIfExists;
  HANDLE RootDirectory;
  DWORD FileNameLength;
  WCHAR FileName[1];
};
struct LARGE_INTEGER
{
  long long QuadPart;
};
inline constexpr BOOL FALSE = 0, TRUE = 1;
#define INVALID_HANDLE_VALUE reinterpret_cast<HANDLE>(intptr_t(-1))
inline constexpr DWORD FILE_NAME_NORMALIZED = 0, VOLUME_NAME_DOS = 0, TOKEN_QUERY = 8, FILE_ALL_ACCESS = 0x1f01ff,
                       ACL_REVISION = 2, SECURITY_DESCRIPTOR_REVISION = 1, SE_DACL_PROTECTED = 0x1000,
                       FILE_ATTRIBUTE_DIRECTORY = 16, FILE_ATTRIBUTE_REPARSE_POINT = 1024, FILE_TYPE_DISK = 1,
                       SE_FILE_OBJECT = 1, OWNER_SECURITY_INFORMATION = 1, DACL_SECURITY_INFORMATION = 4,
                       ERROR_SUCCESS = 0, ACCESS_ALLOWED_ACE_TYPE = 0, FILE_READ_ATTRIBUTES = 128, FILE_SHARE_READ = 1,
                       FILE_SHARE_WRITE = 2, FILE_FLAG_BACKUP_SEMANTICS = 0x02000000,
                       FILE_FLAG_OPEN_REPARSE_POINT = 0x00200000, READ_CONTROL = 0x20000, OPEN_EXISTING = 3,
                       OPEN_ALWAYS = 4, CREATE_NEW = 1, FILE_ATTRIBUTE_NORMAL = 128,
                       FILE_FLAG_WRITE_THROUGH = 0x80000000, DELETE = 0x10000, GENERIC_READ = 0x80000000,
                       GENERIC_WRITE = 0x40000000, ERROR_FILE_NOT_FOUND = 2, ERROR_PATH_NOT_FOUND = 3,
                       ERROR_SHARING_VIOLATION = 32;
inline constexpr int TokenUser = 1, FileDispositionInfo = 4, FileRenameInfo = 3;
namespace winfixture {
struct File
{
  std::wstring path;
  std::string bytes;
  bool directory = false, deleted = false;
};
inline bool fsEnabled = false;
inline std::map<std::wstring, File> fs;
inline DWORD lastError = ERROR_FILE_NOT_FOUND;
inline File *file(HANDLE h)
{
  for (auto &[key, value] : fs)
    if (&value == h)
      return &value;
  return nullptr;
}
inline void seed(const std::wstring &path, bool directory)
{
  fs[path] = File{path, "", directory, false};
}
inline SECURITY_DESCRIPTOR sd;
inline ACCESS_ALLOWED_ACE ace;
inline BY_HANDLE_FILE_INFORMATION info;
inline DWORD type = FILE_TYPE_DISK;
inline bool securityReadable = true, aclValid = true;
inline int fileCalls = 0;
inline void reset()
{
  sd = {};
  ace = {};
  info = {};
  type = FILE_TYPE_DISK;
  securityReadable = aclValid = true;
  fileCalls = 0;
  fsEnabled = false;
  fs.clear();
  lastError = ERROR_FILE_NOT_FOUND;
}
} // namespace winfixture
inline BOOL CloseHandle(HANDLE)
{
  return TRUE;
}
inline DWORD GetFinalPathNameByHandleW(HANDLE h, WCHAR *out, DWORD size, DWORD)
{
  auto *f = winfixture::file(h);
  if (!f || f->path.size() >= size)
    return 0;
  std::copy(f->path.begin(), f->path.end(), out);
  return f->path.size();
}
inline HANDLE GetCurrentProcess()
{
  return &winfixture::sd;
}
inline BOOL OpenProcessToken(HANDLE, DWORD, HANDLE *h)
{
  *h = &winfixture::sd;
  return TRUE;
}
inline BOOL GetTokenInformation(HANDLE, int, void *data, DWORD, DWORD *size)
{
  *size = sizeof(TOKEN_USER);
  if (data)
    static_cast<TOKEN_USER *>(data)->User.Sid = &winfixture::sd.owner;
  return data != nullptr;
}
inline BOOL IsValidSid(PSID sid)
{
  return sid && *static_cast<DWORD *>(sid) != 0;
}
inline DWORD GetLengthSid(PSID)
{
  return sizeof(DWORD);
}
inline BOOL EqualSid(PSID a, PSID b)
{
  return a && b && *static_cast<DWORD *>(a) == *static_cast<DWORD *>(b);
}
inline BOOL InitializeAcl(PACL, DWORD, DWORD)
{
  return TRUE;
}
inline BOOL AddAccessAllowedAceEx(PACL, DWORD, DWORD, DWORD, PSID)
{
  return TRUE;
}
inline BOOL InitializeSecurityDescriptor(SECURITY_DESCRIPTOR *, DWORD)
{
  return TRUE;
}
inline BOOL SetSecurityDescriptorOwner(SECURITY_DESCRIPTOR *, PSID, BOOL)
{
  return TRUE;
}
inline BOOL SetSecurityDescriptorDacl(SECURITY_DESCRIPTOR *, BOOL, PACL, BOOL)
{
  return TRUE;
}
inline BOOL SetSecurityDescriptorControl(SECURITY_DESCRIPTOR *, WORD, WORD)
{
  return TRUE;
}
inline BOOL GetFileInformationByHandle(HANDLE h, BY_HANDLE_FILE_INFORMATION *p)
{
  *p = winfixture::info;
  if (auto *f = winfixture::file(h)) {
    p->dwFileAttributes =
        (p->dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY) | (f->directory ? FILE_ATTRIBUTE_DIRECTORY : 0);
  }
  return TRUE;
}
inline DWORD GetFileType(HANDLE)
{
  return winfixture::type;
}
inline DWORD GetSecurityInfo(HANDLE, DWORD, DWORD, PSID *owner, PSID *, PACL *acl, PACL *, PSECURITY_DESCRIPTOR *sd)
{
  if (!winfixture::securityReadable)
    return 5;
  *sd = &winfixture::sd;
  *owner = &winfixture::sd.owner;
  *acl = winfixture::sd.present ? &winfixture::sd.acl : nullptr;
  return 0;
}
inline BOOL GetSecurityDescriptorControl(SECURITY_DESCRIPTOR *sd, WORD *control, DWORD *revision)
{
  *control = sd->control;
  *revision = 1;
  return TRUE;
}
inline BOOL GetSecurityDescriptorDacl(SECURITY_DESCRIPTOR *sd, BOOL *present, PACL *acl, BOOL *defaults)
{
  *present = sd->present;
  *acl = sd->present ? &sd->acl : nullptr;
  *defaults = FALSE;
  return TRUE;
}
inline BOOL IsValidAcl(PACL)
{
  return winfixture::aclValid;
}
inline BOOL GetAce(PACL, DWORD, void **raw)
{
  *raw = &winfixture::ace;
  return TRUE;
}
inline void *LocalFree(void *)
{
  return nullptr;
}
inline HANDLE CreateFileW(const WCHAR *path, DWORD, DWORD, SECURITY_ATTRIBUTES *, DWORD disposition, DWORD, HANDLE)
{
  using namespace winfixture;
  ++fileCalls;
  if (!fsEnabled)
    return INVALID_HANDLE_VALUE;
  auto it = fs.find(path);
  if (it != fs.end() && it->second.deleted) {
    fs.erase(it);
    it = fs.end();
  }
  if (it == fs.end()) {
    if (disposition == OPEN_EXISTING) {
      lastError = ERROR_FILE_NOT_FOUND;
      return INVALID_HANDLE_VALUE;
    }
    seed(path, false);
    it = fs.find(path);
  } else if (disposition == CREATE_NEW) {
    lastError = 80;
    return INVALID_HANDLE_VALUE;
  }
  return &it->second;
}
inline BOOL CreateDirectoryW(const WCHAR *path, SECURITY_ATTRIBUTES *)
{
  using namespace winfixture;
  ++fileCalls;
  if (!fsEnabled || fs.count(path))
    return FALSE;
  seed(path, true);
  return TRUE;
}
inline BOOL SetFileInformationByHandle(HANDLE h, int kind, void *data, DWORD)
{
  using namespace winfixture;
  ++fileCalls;
  auto *f = file(h);
  if (!fsEnabled || !f)
    return FALSE;
  if (kind == FileDispositionInfo) {
    f->deleted = true;
    return TRUE;
  }
  if (kind == FileRenameInfo) {
    auto *rename = static_cast<FILE_RENAME_INFO *>(data);
    std::wstring dest(rename->FileName, rename->FileNameLength / sizeof(wchar_t));
    if (fs.count(dest) && !rename->ReplaceIfExists)
      return FALSE;
    auto copy = *f;
    copy.path = dest;
    fs[dest] = copy;
    f->deleted = true;
    return TRUE;
  }
  return FALSE;
}
inline BOOL WriteFile(HANDLE h, const void *bytes, DWORD count, DWORD *out, void *)
{
  using namespace winfixture;
  ++fileCalls;
  auto *f = file(h);
  if (!fsEnabled || !f)
    return FALSE;
  f->bytes.append(static_cast<const char *>(bytes), count);
  *out = count;
  return TRUE;
}
inline BOOL ReadFile(HANDLE h, void *bytes, DWORD count, DWORD *out, void *)
{
  using namespace winfixture;
  ++fileCalls;
  auto *f = file(h);
  if (!fsEnabled || !f)
    return FALSE;
  *out = std::min<DWORD>(count, f->bytes.size());
  std::memcpy(bytes, f->bytes.data(), *out);
  return TRUE;
}
inline BOOL FlushFileBuffers(HANDLE h)
{
  ++winfixture::fileCalls;
  return winfixture::fsEnabled && winfixture::file(h);
}
inline BOOL GetFileSizeEx(HANDLE h, LARGE_INTEGER *size)
{
  auto *f = winfixture::file(h);
  if (!f)
    return FALSE;
  size->QuadPart = f->bytes.size();
  return TRUE;
}
inline DWORD GetLastError()
{
  return winfixture::lastError;
}
