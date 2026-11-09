// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "CredentialStore.h"
#include "CandidatePolicy.h"
#include "PeerAuthentication.h"
#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif
#include "IsolatedSession.h"
#include "WindowsCredentialFiles.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QThread>
#include <mutex>
#include <optional>
#ifndef Q_OS_WIN
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef Q_OS_MACOS
#include <sys/acl.h>
#endif
#endif
namespace deskflow::lab {
namespace {
constexpr auto finalName = "credentials.bundle";
constexpr auto nextName = "credentials.next";
std::mutex runtimeMutex;
std::optional<CredentialMaterial> runtime;
QByteArray encode(const QString &session, const CredentialMaterial &m)
{
  return QJsonDocument(
             QJsonObject{
                 {"schema", 1},
                 {"session", session},
                 {"identity", QString::fromLatin1(m.identity.toBase64())},
                 {"servers", QString::fromLatin1(m.servers.toBase64())},
                 {"clients", QString::fromLatin1(m.clients.toBase64())}
             }
  ).toJson(QJsonDocument::Compact);
}
bool decode(const QByteArray &bytes, const QString &session, CredentialMaterial &m)
{
  if (bytes.size() > 270000)
    return false;
  const auto o = QJsonDocument::fromJson(bytes).object();
  if (o.size() != 5 || o["schema"] != 1 || o["session"] != session)
    return false;
  auto field = [&](const char *key) {
    if (!o[key].isString())
      return QByteArray();
    const auto encoded = o[key].toString().toLatin1();
    auto result = QByteArray::fromBase64(encoded, QByteArray::AbortOnBase64DecodingErrors);
    return result.toBase64() == encoded ? result : QByteArray();
  };
  m = {field("identity"), field("servers"), field("clients")};
  return m.bounded() && encode(session, m) == bytes;
}
#ifndef Q_OS_WIN
bool restricted(int fd, bool directory, int links = 1)
{
  struct stat s{};
  if (fstat(fd, &s) || s.st_uid != geteuid() || (s.st_mode & 0077) ||
      (directory ? !S_ISDIR(s.st_mode) : (!S_ISREG(s.st_mode) || s.st_nlink != links)))
    return false;
#ifdef Q_OS_MACOS
  // acl_get_fd_np returns NULL/ENOENT for an absent ACL too. Query the
  // opened object's security properties explicitly, never treat an arbitrary
  // ACL read failure as "no ACL".
  filesec_t security = filesec_init();
  if (!security)
    return false;
  struct stat secured{};
  int present = 0;
  bool safe = fstatx_np(fd, &secured, security) == 0 && secured.st_dev == s.st_dev && secured.st_ino == s.st_ino &&
              filesec_query_property(security, FILESEC_ACL, &present) == 0;
  if (safe && present) {
    acl_t acl = nullptr;
    safe = filesec_get_property(security, FILESEC_ACL, &acl) == 0 && acl;
    if (safe) {
      acl_entry_t entry = nullptr;
      errno = 0;
      const int result = acl_get_entry(acl, ACL_FIRST_ENTRY, &entry);
      safe = result == -1 && errno == EINVAL && entry == nullptr;
    }
    if (acl)
      acl_free(acl);
  }
  filesec_free(security);
  if (!safe)
    return false; // A restrictive mode alone does not negate an ACL grant.
#endif
  return true;
}
struct Handles
{
  int root = -1, lock = -1;
  ~Handles()
  {
    if (lock >= 0)
      close(lock);
    if (root >= 0)
      close(root);
  }
  QString open(const QString &path, const QString &parent)
  {
    const auto layout = IsolatedSession::inspectLayout(path, parent);
    if (!layout.valid())
      return "invalid-session";
    // Open the already canonical allowed parent and then a single child name.
    int p = ::open(QFile::encodeName(parent), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (p < 0)
      return "unsafe-parent";
    root = openat(p, QFile::encodeName(QFileInfo(path).fileName()), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    close(p);
    if (root < 0 || !restricted(root, true))
      return "unsafe-session-permissions";
    lock = openat(root, "credentials.lock", O_RDWR | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (lock < 0 || !restricted(lock, false))
      return "unsafe-lock";
    if (flock(lock, LOCK_EX | LOCK_NB))
      return "credentials-busy";
    return {};
  }
};
QString removeLeaf(int root, const char *name)
{
  struct stat path{};
  if (fstatat(root, name, &path, AT_SYMLINK_NOFOLLOW))
    return errno == ENOENT ? QString() : QString("cleanup-stat-failed");
  int fd = openat(root, name, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
  if (fd < 0)
    return "unsafe-cleanup-target";
  struct stat opened{};
  const bool safe =
      restricted(fd, false) && !fstat(fd, &opened) && opened.st_ino == path.st_ino && opened.st_dev == path.st_dev;
  close(fd);
  if (!safe)
    return "unsafe-cleanup-target";
  return unlinkat(root, name, 0) ? QString("cleanup-unlink-failed") : QString();
}
#endif
} // namespace
bool CredentialStore::openPrivateCandidateFile(QFile &file)
{
  const auto path = file.fileName();
  const auto profile = QFileInfo(QFileInfo(path).absolutePath()).absolutePath() + "/DeskflowCandidate.conf";
  const auto tls = QFileInfo(path).absolutePath();
  if (!deskflow::candidate::securityProfile(profile, tls + "/deskflow-candidate.pem", true, true) ||
      !QStringList{"deskflow-candidate.pem", "trusted-servers", "trusted-clients"}.contains(QFileInfo(path).fileName()))
    return false;
#ifdef Q_OS_WIN
  const auto raw = WindowsCredentialFiles::createPrivateFile(path);
  if (raw == -1)
    return false;
  const int fd = _open_osfhandle(raw, _O_WRONLY | _O_BINARY);
  if (fd < 0) {
    CloseHandle(reinterpret_cast<HANDLE>(raw));
    return false;
  }
  if (file.open(fd, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle))
    return true;
  _close(fd);
  return false;
#else
  if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly, QFileDevice::ReadOwner | QFileDevice::WriteOwner))
    return false;
  if (restricted(file.handle(), false))
    return true;
  file.close();
  file.remove();
  return false;
#endif
}
bool CredentialMaterial::bounded() const
{
  return !identity.isEmpty() && identity.size() <= 65536 && !servers.isEmpty() && servers.size() <= 65536 &&
         !clients.isEmpty() && clients.size() <= 65536;
}
struct CredentialLease::Impl
{
#ifndef Q_OS_WIN
  std::unique_ptr<Handles> handles;
#else
  std::unique_ptr<WindowsCredentialFiles> windows;
#endif
  CredentialMaterial material;
  QString root;
};
CredentialLease::CredentialLease(std::unique_ptr<Impl> p) : d(std::move(p))
{
}
CredentialLease::~CredentialLease() = default;
const CredentialMaterial &CredentialLease::material() const
{
  return d->material;
}
bool CredentialStore::writeCandidateTrust(const QString &path, const QByteArray &bytes)
{
  const auto tls = QFileInfo(path).absolutePath(), profile = QFileInfo(tls).absolutePath() + "/DeskflowCandidate.conf";
  if (bytes.size() > 65536 || !QStringList{"trusted-servers", "trusted-clients"}.contains(QFileInfo(path).fileName()) ||
      !deskflow::candidate::securityProfile(profile, tls + "/deskflow-candidate.pem", true, true) ||
      !ensurePrivateCandidateDirectory(tls))
    return false;
#ifdef Q_OS_WIN
  return WindowsCredentialFiles::publishPrivateFile(path, bytes);
#elif defined(Q_OS_MACOS)
  if (QFileInfo::exists(path)) {
    int fd = ::open(QFile::encodeName(path), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    const bool safe = fd >= 0 && restricted(fd, false);
    if (fd >= 0)
      close(fd);
    if (!safe)
      return false;
  }
  QSaveFile file(path);
  file.setDirectWriteFallback(false);
  if (!file.open(QIODevice::WriteOnly) || !file.setPermissions(QFile::ReadOwner | QFile::WriteOwner) ||
      file.write(bytes) != bytes.size())
    return false;
  return file.commit();
#else
  return false;
#endif
}
QString CredentialStore::candidateSessionsDirectory(const QString &settings)
{
  if (!deskflow::candidate::settingsPath(settings))
    return {};
  const auto root = QFileInfo(settings).absolutePath() + "/recovery-sessions";
  return deskflow::candidate::plainPath(root) ? root : QString();
}
bool CredentialStore::recoverCandidateSessions(const QString &settings)
{
  const auto parent = candidateSessionsDirectory(settings);
  if (parent.isEmpty())
    return false;
  if (!QFileInfo::exists(parent))
    return true;
  if (!ensurePrivateCandidateDirectory(parent))
    return false;
  const auto entries = QDir(parent).entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System);
  int pending = 0;
  for (const auto &entry : entries) {
    if (!deskflow::auth::identifier(entry))
      continue;
    const auto root = parent + '/' + entry;
    if (QFileInfo(root).isSymLink())
      return false;
    // Completed probes retain only non-secret metadata. They must not exhaust
    // the stale-secret recovery bound after 128 normal reconnections.
    const QFileInfo bundle(root + "/credentials.bundle"), next(root + "/credentials.next");
    if (!bundle.exists() && !bundle.isSymLink() && !next.exists() && !next.isSymLink())
      continue;
    if (++pending > 128)
      return false;
    const auto error = recover(root, parent);
    if (!error.isEmpty() && error != "credentials-busy")
      return false;
  }
  return true;
}
bool CredentialStore::ensurePrivateCandidateDirectory(const QString &path)
{
  if (!deskflow::candidate::plainPath(path) ||
      (QFileInfo(path).fileName() != "tls" && QFileInfo(path).fileName() != "recovery-sessions") ||
      !deskflow::candidate::settingsPath(QFileInfo(path).absolutePath() + "/DeskflowCandidate.conf"))
    return false;
#ifdef Q_OS_WIN
  return WindowsCredentialFiles::ensurePrivateDirectory(path);
#elif defined(Q_OS_MACOS)
  const auto parent = QFileInfo(path).absolutePath();
  int base = ::open(QFile::encodeName(parent), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  if (base < 0)
    return false;
  const auto leaf = QFile::encodeName(QFileInfo(path).fileName());
  const int made = mkdirat(base, leaf, 0700);
  if (made && errno != EEXIST) {
    close(base);
    return false;
  }
  int fd = openat(base, leaf, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  close(base);
  const bool ok = fd >= 0 && restricted(fd, true);
  if (fd >= 0)
    close(fd);
  return ok;
#else
  return false;
#endif
}
std::unique_ptr<CredentialLease> CredentialStore::acquireCandidateProfile(
    const QString &settings, const QString &identity, const QString &expected, QString *error
)
{
  auto fail = [&](const char *code) -> std::unique_ptr<CredentialLease> {
    if (error)
      *error = QString::fromLatin1(code);
    return {};
  };
  const auto pin = deskflow::auth::fingerprint(expected);
  if (pin.isEmpty() || pin != expected || !deskflow::candidate::securityProfile(settings, identity, true, true))
    return fail("invalid-candidate-profile");
  const auto tls = QFileInfo(identity).absolutePath();
  auto read = [&](const QString &path, QByteArray &bytes, bool optional) {
#ifdef Q_OS_WIN
    return WindowsCredentialFiles::readPrivateFile(path, bytes, optional);
#elif defined(Q_OS_MACOS)
    int fd = ::open(QFile::encodeName(path), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
      return optional && errno == ENOENT;
    char actual[PATH_MAX]{};
    if (!restricted(fd, false) || fcntl(fd, F_GETPATH, actual) != 0 || QFile::decodeName(actual) != path) {
      close(fd);
      return false;
    }
    QFile f;
    if (!f.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
      close(fd);
      return false;
    }
    if (f.size() > 65536)
      return false;
    bytes = f.read(65537);
    return bytes.size() <= 65536 && f.error() == QFileDevice::NoError;
#else
    return false;
#endif
  };
  auto impl = std::make_unique<CredentialLease::Impl>();
  auto &m = impl->material;
  if (!read(identity, m.identity, false) || !read(tls + "/trusted-servers", m.servers, false) ||
      !read(tls + "/trusted-clients", m.clients, true))
    return fail("unsafe-candidate-credentials");
  auto hex = pin;
  hex.remove(':');
  bool trusted = false;
  for (auto line : m.servers.split('\n'))
    if (line.trimmed() == "v2:sha256:" + hex.toLatin1())
      trusted = true;
  if (!trusted)
    return fail("candidate-identity-not-trusted");
  if (m.clients.isEmpty())
    m.clients = "\n";
  if (!m.bounded())
    return fail("invalid-candidate-material");
  // root intentionally empty: a profile snapshot cannot be activated as an
  // isolated session. Only the bounded per-attempt provisioning path may use it.
  if (error)
    error->clear();
  return std::unique_ptr<CredentialLease>(new CredentialLease(std::move(impl)));
}
QString CredentialStore::provision(const QString &root, const QString &parent, const CredentialMaterial &material)
{
  if (!material.bounded())
    return "invalid-material";
#ifdef Q_OS_WIN
  WindowsCredentialFiles files;
  const auto error = files.open(root, parent);
  return error.isEmpty() ? files.publish(encode(QFileInfo(root).fileName(), material)) : error;
#elif !defined(Q_OS_MACOS)
  return "native-platform-acl-unavailable";
#else
  Handles h;
  auto error = h.open(root, parent);
  if (!error.isEmpty())
    return error;
  int fd = openat(h.root, nextName, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
  if (fd < 0)
    return "staging-exists-or-denied";
  auto bytes = encode(QFileInfo(root).fileName(), material);
  bool ok = restricted(fd, false);
  qsizetype offset = 0;
  while (ok && offset < bytes.size()) {
    const auto n = write(fd, bytes.constData() + offset, bytes.size() - offset);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0) {
      ok = false;
      break;
    }
    offset += n;
  }
  ok = ok && fsync(fd) == 0;
  close(fd);
  // linkat publishes the complete inode without replacing any existing leaf.
  // The exclusive OS lease prevents a cooperating reader seeing link count 2.
  if (ok)
    ok = linkat(h.root, nextName, h.root, finalName, 0) == 0;
  if (!ok) {
    removeLeaf(h.root, nextName);
    return "atomic-publication-failed";
  }
  if (unlinkat(h.root, nextName, 0) || fsync(h.root))
    return "publication-cleanup-required";
  return {};
#endif
}
std::unique_ptr<CredentialLease> CredentialStore::acquire(const QString &root, const QString &parent, QString *error)
{
  auto fail = [&](const char *code) -> std::unique_ptr<CredentialLease> {
    if (error)
      *error = QString::fromLatin1(code);
    return {};
  };
#ifdef Q_OS_WIN
  auto impl = std::make_unique<CredentialLease::Impl>();
  impl->windows = std::make_unique<WindowsCredentialFiles>();
  auto problem = impl->windows->open(root, parent);
  QByteArray bytes;
  if (problem.isEmpty())
    problem = impl->windows->read(bytes);
  if (!problem.isEmpty()) {
    if (error)
      *error = problem;
    return {};
  }
  if (!decode(bytes, QFileInfo(root).fileName(), impl->material))
    return fail("invalid-credential-bundle");
  impl->root = root;
  if (error)
    error->clear();
  return std::unique_ptr<CredentialLease>(new CredentialLease(std::move(impl)));
#elif !defined(Q_OS_MACOS)
  return fail("native-platform-acl-unavailable");
#else
  auto impl = std::make_unique<CredentialLease::Impl>();
  impl->handles = std::make_unique<Handles>();
  const auto problem = impl->handles->open(root, parent);
  if (!problem.isEmpty()) {
    if (error)
      *error = problem;
    return {};
  }
  impl->root = root;
  auto &h = *impl->handles;
  int fd = openat(h.root, finalName, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
  if (fd < 0)
    return fail("missing-or-redirected-credentials");
  if (!restricted(fd, false)) {
    close(fd);
    return fail("unsafe-credential-permissions");
  }
  QFile file;
  if (!file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
    close(fd);
    return fail("credential-read-failed");
  }
  if (file.size() > 270000)
    return fail("credential-size-limit");
  const auto bytes = file.read(270001);
  if (!decode(bytes, QFileInfo(root).fileName(), impl->material))
    return fail("invalid-credential-bundle");
  if (error)
    error->clear();
  return std::unique_ptr<CredentialLease>(new CredentialLease(std::move(impl)));
#endif
}
QString CredentialStore::recover(const QString &root, const QString &parent)
{
#ifdef Q_OS_WIN
  WindowsCredentialFiles files;
  const auto error = files.open(root, parent);
  return error.isEmpty() ? files.clear() : error;
#elif !defined(Q_OS_MACOS)
  return "native-platform-acl-unavailable";
#else
  Handles h;
  auto error = h.open(root, parent);
  if (!error.isEmpty())
    return error;
  // If a crash happened between link and unlink, both leaves name the same
  // inode. Remove staging only after verifying both exact names and owner.
  struct stat a{}, b{};
  if (!fstatat(h.root, nextName, &a, AT_SYMLINK_NOFOLLOW) && !fstatat(h.root, finalName, &b, AT_SYMLINK_NOFOLLOW) &&
      S_ISREG(a.st_mode) && a.st_uid == geteuid() && !(a.st_mode & 0077) && a.st_nlink == 2 && a.st_dev == b.st_dev &&
      a.st_ino == b.st_ino) {
    const int fd = openat(h.root, nextName, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    struct stat opened{};
    const bool safe = fd >= 0 && restricted(fd, false, 2) && !fstat(fd, &opened) && opened.st_dev == a.st_dev &&
                      opened.st_ino == a.st_ino;
    if (fd >= 0)
      close(fd);
    if (!safe)
      return "unsafe-cleanup-target";
    if (unlinkat(h.root, nextName, 0))
      return "cleanup-unlink-failed";
  }
  error = removeLeaf(h.root, nextName);
  if (!error.isEmpty())
    return error;
  error = removeLeaf(h.root, finalName);
  if (!error.isEmpty())
    return error;
  return fsync(h.root) ? QString("cleanup-sync-failed") : QString();
#endif
}
QString CredentialStore::createSession(const QString &root, const QString &parent, const QJsonObject &manifest)
{
  if (QFileInfo(parent).canonicalFilePath() != parent || QFileInfo(root).absolutePath() != parent ||
      QFileInfo(root).exists() || QFileInfo(root).isSymLink() ||
      !QRegularExpression("\\A[0-9a-f]{32}\\z").match(QFileInfo(root).fileName()).hasMatch())
    return "invalid-new-session";
  const auto bytes = QJsonDocument(manifest).toJson(QJsonDocument::Compact);
  if (bytes.size() > 4096 || manifest["session"] != QFileInfo(root).fileName())
    return "invalid-new-session";
#ifdef Q_OS_WIN
  const auto result = WindowsCredentialFiles::createSession(root, parent, bytes);
  if (!result.isEmpty())
    return result;
#elif defined(Q_OS_MACOS)
  int base = ::open(QFile::encodeName(parent), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  if (base < 0)
    return "unsafe-parent";
  const auto leaf = QFile::encodeName(QFileInfo(root).fileName());
  const int made = mkdirat(base, leaf, 0700);
  close(base);
  if (made)
    return "session-create-failed";
  for (const auto *name : {"config/DeskflowLab/tls", "state", "runtime", "logs"})
    if (!QDir().mkpath(root + '/' + name))
      return "session-create-failed";
  int fd =
      ::open(QFile::encodeName(root + "/session.json"), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
  if (fd < 0)
    return "session-create-failed";
  const auto count = write(fd, bytes.constData(), bytes.size());
  const bool synced = fsync(fd) == 0;
  close(fd);
  if (count != bytes.size() || !synced)
    return "session-create-failed";
#else
  return "native-platform-acl-unavailable";
#endif
  return IsolatedSession::inspect(root, parent).valid() ? QString() : QString("invalid-new-session");
}
bool CredentialStore::activate(const CredentialLease &lease)
{
  const auto *session = IsolatedSession::active();
  if (!QCoreApplication::instance() || QThread::currentThread() != QCoreApplication::instance()->thread() || !session ||
      lease.d->root != session->root)
    return false;
  std::lock_guard lock(runtimeMutex);
  if (runtime || !lease.material().bounded())
    return false;
  runtime = lease.material();
  return true;
}
const CredentialMaterial *CredentialStore::active()
{
  return runtime ? &*runtime : nullptr;
}
} // namespace deskflow::lab
