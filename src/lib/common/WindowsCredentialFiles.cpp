// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "WindowsCredentialFiles.h"
#if defined(Q_OS_WIN) || defined(DESKFLOW_WINDOWS_CREDENTIAL_TEST)
#include "CandidatePolicy.h"
#include "IsolatedSession.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#ifdef DESKFLOW_WINDOWS_CREDENTIAL_TEST
#include "WindowsSecurityApiShim.h"
#else
#include <aclapi.h>
#include <windows.h>
#endif
#include <cstddef>
#include <cstring>
#include <vector>
namespace deskflow::lab {
namespace {
struct Handle
{
  HANDLE h = INVALID_HANDLE_VALUE;
  Handle() = default;
  explicit Handle(HANDLE value) : h(value)
  {
  }
  ~Handle()
  {
    if (h != INVALID_HANDLE_VALUE && h)
      CloseHandle(h);
  }
  Handle(const Handle &) = delete;
  Handle &operator=(const Handle &) = delete;
  Handle(Handle &&other) noexcept : h(other.h)
  {
    other.h = INVALID_HANDLE_VALUE;
  }
  Handle &operator=(Handle &&other) noexcept
  {
    if (this != &other) {
      if (h != INVALID_HANDLE_VALUE && h)
        CloseHandle(h);
      h = other.h;
      other.h = INVALID_HANDLE_VALUE;
    }
    return *this;
  }
  bool valid() const
  {
    return h != INVALID_HANDLE_VALUE && h;
  }
};
std::wstring native(const QString &p)
{
  return QDir::toNativeSeparators(p).toStdWString();
}
// Keep drive paths platform independent for the offline API-shape fixture.
// QFileInfo::absolutePath() would interpret a Windows path as relative on macOS.
QString parentPath(const QString &p)
{
  const auto slash = p.lastIndexOf('/');
  return slash == 2 ? p.left(3) : p.left(slash);
}
bool localPath(const QString &path)
{
  if (!QRegularExpression("\\A[A-Za-z]:/").match(path).hasMatch() || path != QDir::cleanPath(path) ||
      path.mid(2).contains(':'))
    return false;
  for (const auto &part : path.mid(3).split('/'))
    if (part.isEmpty() || part.endsWith('.') || part.endsWith(' '))
      return false;
  return true;
}
bool candidateLocalPath(const QString &path)
{
  if (!localPath(path))
    return false;
  for (const auto &part : path.split('/'))
    if (part.compare("Deskflow", Qt::CaseInsensitive) == 0 || part.compare("Deskflow.app", Qt::CaseInsensitive) == 0)
      return false;
  return true;
}
QString finalPath(HANDLE handle)
{
  std::vector<wchar_t> text(32768);
  const DWORD n = GetFinalPathNameByHandleW(
      handle, text.data(), static_cast<DWORD>(text.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS
  );
  if (!n || n >= text.size())
    return {};
  auto result = QDir::fromNativeSeparators(QString::fromWCharArray(text.data(), n));
  if (result.startsWith("//?/"))
    result.remove(0, 4);
  return result;
}
struct Security
{
  QByteArray token, aclBytes;
  SECURITY_DESCRIPTOR descriptor{};
  SECURITY_ATTRIBUTES attributes{};
  PSID sid = nullptr;
  bool prepare()
  {
    HANDLE raw = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &raw))
      return false;
    Handle tokenHandle(raw);
    DWORD needed = 0;
    GetTokenInformation(raw, TokenUser, nullptr, 0, &needed);
    if (needed < sizeof(TOKEN_USER) || needed > 65536)
      return false;
    token.resize(needed);
    if (!GetTokenInformation(raw, TokenUser, token.data(), needed, &needed))
      return false;
    sid = reinterpret_cast<TOKEN_USER *>(token.data())->User.Sid;
    if (!IsValidSid(sid))
      return false;
    const DWORD bytes = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(sid);
    aclBytes.resize(bytes);
    auto *acl = reinterpret_cast<PACL>(aclBytes.data());
    if (!InitializeAcl(acl, bytes, ACL_REVISION) ||
        !AddAccessAllowedAceEx(acl, ACL_REVISION, 0, FILE_ALL_ACCESS, sid) ||
        !InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION) ||
        !SetSecurityDescriptorOwner(&descriptor, sid, FALSE) ||
        !SetSecurityDescriptorDacl(&descriptor, TRUE, acl, FALSE) ||
        !SetSecurityDescriptorControl(&descriptor, SE_DACL_PROTECTED, SE_DACL_PROTECTED))
      return false;
    attributes = {sizeof(SECURITY_ATTRIBUTES), &descriptor, FALSE};
    return true;
  }
};
bool safeHandle(HANDLE handle, PSID expected, bool directory)
{
  BY_HANDLE_FILE_INFORMATION info{};
  if (!GetFileInformationByHandle(handle, &info))
    return false;
  WindowsAclFacts facts;
  facts.disk = GetFileType(handle) == FILE_TYPE_DISK;
  facts.directory = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  facts.reparse = (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
  facts.links = info.nNumberOfLinks;
  PSECURITY_DESCRIPTOR sd = nullptr;
  PSID owner = nullptr;
  PACL acl = nullptr;
  if (GetSecurityInfo(
          handle, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, &owner, nullptr, &acl,
          nullptr, &sd
      ) != ERROR_SUCCESS)
    return false;
  facts.ownerMatches = owner && IsValidSid(owner) && EqualSid(owner, expected);
  SECURITY_DESCRIPTOR_CONTROL control{};
  DWORD revision = 0;
  facts.protectedDacl = GetSecurityDescriptorControl(sd, &control, &revision) && (control & SE_DACL_PROTECTED);
  BOOL present = FALSE, defaulted = FALSE;
  PACL fromDescriptor = nullptr;
  facts.daclPresent = GetSecurityDescriptorDacl(sd, &present, &fromDescriptor, &defaulted) && present && acl &&
                      fromDescriptor == acl && IsValidAcl(acl);
  if (facts.daclPresent) {
    facts.aceCount = acl->AceCount;
    facts.allAcesOwnerAllow = true;
    facts.inheritedOrPropagating = false;
    for (DWORD i = 0; i < acl->AceCount; ++i) {
      void *raw = nullptr;
      if (!GetAce(acl, i, &raw)) {
        facts.allAcesOwnerAllow = false;
        break;
      }
      const auto *header = static_cast<ACE_HEADER *>(raw);
      if (header->AceType != ACCESS_ALLOWED_ACE_TYPE || header->AceSize < sizeof(ACCESS_ALLOWED_ACE)) {
        facts.allAcesOwnerAllow = false;
        break;
      }
      const auto *ace = static_cast<ACCESS_ALLOWED_ACE *>(raw);
      auto *sid = const_cast<DWORD *>(&ace->SidStart);
      facts.inheritedOrPropagating |= header->AceFlags != 0;
      if (!IsValidSid(sid) || GetLengthSid(sid) > header->AceSize - offsetof(ACCESS_ALLOWED_ACE, SidStart) ||
          !EqualSid(sid, expected) || ace->Mask != FILE_ALL_ACCESS)
        facts.allAcesOwnerAllow = false;
    }
  }
  LocalFree(sd);
  return facts.permits(directory);
}
// Hold every ancestor without FILE_SHARE_DELETE. This prevents junction/rename
// replacement during full-path CreateFile/rename calls; no NT private API used.
bool lockChain(const QString &path, std::vector<Handle> &chain)
{
  if (!localPath(path))
    return false;
  QString current = path.left(3);
  QStringList components = path.mid(3).split('/');
  components.prepend(QString());
  for (const auto &part : components) {
    if (!part.isEmpty())
      current = QDir(current).filePath(part);
    Handle handle(CreateFileW(
        native(current).c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr
    ));
    BY_HANDLE_FILE_INFORMATION info{};
    if (!handle.valid() || !GetFileInformationByHandle(handle.h, &info) ||
        !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        finalPath(handle.h).compare(current, Qt::CaseInsensitive) != 0)
      return false;
    chain.push_back(std::move(handle));
  }
  return true;
}
bool eraseHandle(HANDLE handle)
{
  FILE_DISPOSITION_INFO disposition{};
  disposition.DeleteFile = TRUE;
  return SetFileInformationByHandle(handle, FileDispositionInfo, &disposition, sizeof(disposition));
}
bool writeBytes(HANDLE handle, const QByteArray &bytes)
{
  DWORD offset = 0;
  while (offset < static_cast<DWORD>(bytes.size())) {
    DWORD count = 0;
    if (!WriteFile(handle, bytes.constData() + offset, static_cast<DWORD>(bytes.size()) - offset, &count, nullptr) ||
        !count)
      return false;
    offset += count;
  }
  return FlushFileBuffers(handle);
}
} // namespace
#ifdef DESKFLOW_WINDOWS_CREDENTIAL_TEST
bool auditWindowsCredentialHandleForTest(void *handle, void *sid, bool directory)
{
  return safeHandle(handle, sid, directory);
}
#endif
struct WindowsCredentialFiles::Impl
{
  Security security;
  QString root;
  std::vector<Handle> ancestors;
  Handle lock;
  Handle file(const QString &name, DWORD access, DWORD disposition)
  {
    return Handle(CreateFileW(
        native(root + '/' + name).c_str(), access | READ_CONTROL, 0, &security.attributes, disposition,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_WRITE_THROUGH, nullptr
    ));
  }
  QString remove(const QString &name)
  {
    auto f = file(name, DELETE | FILE_READ_ATTRIBUTES, OPEN_EXISTING);
    if (!f.valid())
      return GetLastError() == ERROR_FILE_NOT_FOUND ? QString() : QString("unsafe-cleanup-target");
    if (!safeHandle(f.h, security.sid, false) || finalPath(f.h).compare(root + '/' + name, Qt::CaseInsensitive) != 0)
      return "unsafe-cleanup-target";
    return eraseHandle(f.h) ? QString() : QString("cleanup-unlink-failed");
  }
};
WindowsCredentialFiles::WindowsCredentialFiles() : d(std::make_unique<Impl>())
{
}
WindowsCredentialFiles::~WindowsCredentialFiles() = default;
QString WindowsCredentialFiles::open(const QString &root, const QString &parent)
{
  if (!IsolatedSession::inspectLayout(root, parent).valid() || !d->security.prepare() || !lockChain(root, d->ancestors))
    return "invalid-session";
  d->root = root;
  Handle rootHandle(CreateFileW(
      native(root).c_str(), READ_CONTROL | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr
  ));
  if (!rootHandle.valid() || !safeHandle(rootHandle.h, d->security.sid, true))
    return "unsafe-session-permissions";
  d->lock = d->file("credentials.lock", GENERIC_READ | GENERIC_WRITE, OPEN_ALWAYS);
  if (!d->lock.valid())
    return GetLastError() == ERROR_SHARING_VIOLATION ? QString("credentials-busy") : QString("unsafe-lock");
  if (!safeHandle(d->lock.h, d->security.sid, false))
    return "unsafe-lock";
  return {};
}
QString WindowsCredentialFiles::publish(const QByteArray &bytes)
{
  if (bytes.size() > 270000 || bytes.isEmpty())
    return "invalid-material";
  auto f = d->file("credentials.next", GENERIC_WRITE | DELETE, CREATE_NEW);
  if (!f.valid())
    return "staging-exists-or-denied";
  if (!safeHandle(f.h, d->security.sid, false) || !writeBytes(f.h, bytes)) {
    eraseHandle(f.h);
    return "atomic-publication-failed";
  }
  const auto target = native(d->root + "/credentials.bundle");
  std::vector<unsigned char> buffer(sizeof(FILE_RENAME_INFO) + (target.size() + 1) * sizeof(wchar_t));
  auto *info = reinterpret_cast<FILE_RENAME_INFO *>(buffer.data());
  info->ReplaceIfExists = FALSE;
  info->RootDirectory = nullptr;
  info->FileNameLength = static_cast<DWORD>(target.size() * sizeof(wchar_t));
  std::memcpy(info->FileName, target.c_str(), (target.size() + 1) * sizeof(wchar_t));
  if (!SetFileInformationByHandle(f.h, FileRenameInfo, info, static_cast<DWORD>(buffer.size()))) {
    eraseHandle(f.h);
    return "atomic-publication-failed";
  }
  return {};
}
QString WindowsCredentialFiles::read(QByteArray &bytes)
{
  auto f = d->file("credentials.bundle", GENERIC_READ, OPEN_EXISTING);
  if (!f.valid())
    return "missing-or-redirected-credentials";
  if (!safeHandle(f.h, d->security.sid, false))
    return "unsafe-credential-permissions";
  LARGE_INTEGER size{};
  if (!GetFileSizeEx(f.h, &size) || size.QuadPart < 1 || size.QuadPart > 270000)
    return "credential-size-limit";
  bytes.resize(static_cast<qsizetype>(size.QuadPart));
  DWORD got = 0;
  if (!ReadFile(f.h, bytes.data(), static_cast<DWORD>(bytes.size()), &got, nullptr) ||
      got != static_cast<DWORD>(bytes.size()))
    return "credential-read-failed";
  return {};
}
QString WindowsCredentialFiles::clear()
{
  auto error = d->remove("credentials.next");
  return error.isEmpty() ? d->remove("credentials.bundle") : error;
}
bool WindowsCredentialFiles::publishPrivateFile(const QString &path, const QByteArray &bytes)
{
  if (bytes.size() > 65536 || !candidateLocalPath(path))
    return false;
  Security security;
  std::vector<Handle> chain;
  if (!security.prepare() || !lockChain(parentPath(path), chain))
    return false;
  // A pre-existing database must already have the expected protected ACL.
  QByteArray existing;
  if (!readPrivateFile(path, existing, true))
    return false;
  const auto temp = path + ".next";
  Handle f(CreateFileW(
      native(temp).c_str(), GENERIC_WRITE | READ_CONTROL | DELETE, 0, &security.attributes, CREATE_NEW,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_WRITE_THROUGH, nullptr
  ));
  if (!f.valid())
    return false;
  if (!safeHandle(f.h, security.sid, false) || finalPath(f.h).compare(temp, Qt::CaseInsensitive) != 0 ||
      !writeBytes(f.h, bytes)) {
    eraseHandle(f.h);
    return false;
  }
  const auto target = native(path);
  std::vector<unsigned char> buffer(sizeof(FILE_RENAME_INFO) + (target.size() + 1) * sizeof(wchar_t));
  auto *info = reinterpret_cast<FILE_RENAME_INFO *>(buffer.data());
  info->ReplaceIfExists = TRUE;
  info->RootDirectory = nullptr;
  info->FileNameLength = static_cast<DWORD>(target.size() * sizeof(wchar_t));
  std::memcpy(info->FileName, target.c_str(), (target.size() + 1) * sizeof(wchar_t));
  const bool ok = SetFileInformationByHandle(f.h, FileRenameInfo, info, static_cast<DWORD>(buffer.size()));
  if (!ok)
    eraseHandle(f.h);
  return ok;
}
qintptr WindowsCredentialFiles::createPrivateFile(const QString &path)
{
  if (!candidateLocalPath(path))
    return -1;
  Security security;
  std::vector<Handle> chain;
  if (!security.prepare() || !lockChain(parentPath(path), chain))
    return -1;
  Handle f(CreateFileW(
      native(path).c_str(), GENERIC_WRITE | READ_CONTROL | DELETE, 0, &security.attributes, CREATE_NEW,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_WRITE_THROUGH, nullptr
  ));
  if (!f.valid())
    return -1;
  if (!safeHandle(f.h, security.sid, false) || finalPath(f.h).compare(path, Qt::CaseInsensitive) != 0) {
    eraseHandle(f.h);
    return -1;
  }
  auto result = reinterpret_cast<qintptr>(f.h);
  f.h = INVALID_HANDLE_VALUE;
  return result;
}
bool WindowsCredentialFiles::ensurePrivateDirectory(const QString &path)
{
  if (!candidateLocalPath(path))
    return false;
  Security security;
  std::vector<Handle> chain;
  if (!security.prepare() || !lockChain(parentPath(path), chain))
    return false;
  // Existing objects are only inspected, never have their ACL widened/replaced.
  Handle f(CreateFileW(
      native(path).c_str(), READ_CONTROL | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr
  ));
  if (!f.valid()) {
    if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND)
      return false;
    if (!CreateDirectoryW(native(path).c_str(), &security.attributes))
      return false;
    f = Handle(CreateFileW(
        native(path).c_str(), READ_CONTROL | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr
    ));
  }
  return f.valid() && safeHandle(f.h, security.sid, true) && finalPath(f.h).compare(path, Qt::CaseInsensitive) == 0;
}
bool WindowsCredentialFiles::readPrivateFile(const QString &path, QByteArray &bytes, bool missingAllowed)
{
  bytes.clear();
  if (!candidateLocalPath(path))
    return false;
  Security security;
  std::vector<Handle> chain;
  if (!security.prepare() || !lockChain(parentPath(path), chain))
    return false;
  Handle f(CreateFileW(
      native(path).c_str(), GENERIC_READ | READ_CONTROL, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr
  ));
  if (!f.valid())
    return missingAllowed && GetLastError() == ERROR_FILE_NOT_FOUND;
  if (!safeHandle(f.h, security.sid, false) || finalPath(f.h).compare(path, Qt::CaseInsensitive) != 0)
    return false;
  LARGE_INTEGER size{};
  if (!GetFileSizeEx(f.h, &size) || size.QuadPart < 0 || size.QuadPart > 65536)
    return false;
  bytes.resize(static_cast<qsizetype>(size.QuadPart));
  DWORD got = 0;
  return ReadFile(f.h, bytes.data(), static_cast<DWORD>(bytes.size()), &got, nullptr) &&
         got == static_cast<DWORD>(bytes.size());
}
QString WindowsCredentialFiles::createSession(const QString &root, const QString &parent, const QByteArray &manifest)
{
  Security security;
  std::vector<Handle> chain;
  if (!security.prepare() || !lockChain(parent, chain) || QFileInfo(root).absolutePath() != parent ||
      QFileInfo(root).exists() ||
      !QRegularExpression("\\A[0-9a-f]{32}\\z").match(QFileInfo(root).fileName()).hasMatch())
    return "invalid-new-session";
  // Create only new owner-only, non-inheriting objects. Never rewrite an ACL
  // on an existing directory, credential file, production path or ancestor.
  if (!CreateDirectoryW(native(root).c_str(), &security.attributes))
    return "session-create-failed";
  std::vector<Handle> rootChain;
  if (!lockChain(root, rootChain))
    return "session-create-failed";
  for (const auto *name : {"config", "config/DeskflowLab", "config/DeskflowLab/tls", "state", "runtime", "logs"})
    if (!CreateDirectoryW(native(root + '/' + name).c_str(), &security.attributes))
      return "session-create-failed";
  Handle f(CreateFileW(
      native(root + "/session.json").c_str(), GENERIC_WRITE | READ_CONTROL | DELETE, 0, &security.attributes,
      CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_WRITE_THROUGH, nullptr
  ));
  if (!f.valid() || !safeHandle(f.h, security.sid, false) || !writeBytes(f.h, manifest))
    return "session-create-failed";
  return {};
}
} // namespace deskflow::lab
#endif
