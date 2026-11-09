// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <memory>
namespace deskflow::lab {
// Extracted native descriptor facts, also used by hostile-descriptor fixtures.
struct WindowsAclFacts
{
  bool disk = false, directory = false, reparse = true, ownerMatches = false, protectedDacl = false,
       daclPresent = false;
  bool allAcesOwnerAllow = false, inheritedOrPropagating = true;
  unsigned aceCount = 0, links = 0;
  bool permits(bool expectedDirectory) const
  {
    return disk && directory == expectedDirectory && !reparse && ownerMatches && protectedDacl && daclPresent &&
           aceCount == 1 && allAcesOwnerAllow && !inheritedOrPropagating && (expectedDirectory || links == 1);
  }
};
#if defined(Q_OS_WIN) || defined(DESKFLOW_WINDOWS_CREDENTIAL_TEST)
#ifdef DESKFLOW_WINDOWS_CREDENTIAL_TEST
bool auditWindowsCredentialHandleForTest(void *, void *, bool);
#endif
class WindowsCredentialFiles
{
public:
  WindowsCredentialFiles();
  ~WindowsCredentialFiles();
  QString open(const QString &root, const QString &parent);
  QString publish(const QByteArray &);
  QString read(QByteArray &);
  QString clear();
  static bool publishPrivateFile(const QString &path, const QByteArray &bytes);
  static qintptr createPrivateFile(const QString &path);
  static bool ensurePrivateDirectory(const QString &path);
  static bool readPrivateFile(const QString &path, QByteArray &bytes, bool missingAllowed = false);
  static QString createSession(const QString &root, const QString &parent, const QByteArray &manifest);

private:
  struct Impl;
  std::unique_ptr<Impl> d;
};
#endif
} // namespace deskflow::lab
