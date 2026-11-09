// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Actual Windows backend descriptor walk, with host API-shape substitutions.
#include "WindowsSecurityApiShim.h"
#include "common/WindowsCredentialFiles.h"
#include <QTest>
using namespace deskflow::lab;
class WindowsCredentialBackendTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void candidateNativeApiPathUsesPrivateAclAndRefusesProduction()
  {
    using namespace winfixture;
    reset();
    fsEnabled = true;
    for (const auto *path :
         {L"C:/", L"C:/Profiles", L"C:/Profiles/DeskflowCandidate", L"C:/Profiles/DeskflowCandidate/tls"})
      seed(path, true);
    const QString path = "C:/Profiles/DeskflowCandidate/tls/deskflow-candidate.pem";
    const auto h = WindowsCredentialFiles::createPrivateFile(path);
    QVERIFY2(h != -1, qPrintable(QString("API calls before refusal: %1").arg(fileCalls)));
    QVERIFY(WindowsCredentialFiles::createPrivateFile(path) == -1);
    CloseHandle(reinterpret_cast<HANDLE>(h));
    QByteArray bytes;
    QVERIFY(WindowsCredentialFiles::readPrivateFile(path, bytes));
    QVERIFY(bytes.isEmpty());
    QVERIFY(
        WindowsCredentialFiles::publishPrivateFile(
            "C:/Profiles/DeskflowCandidate/tls/trusted-servers", "SYNTHETIC-TRUST-NOT-A-CERT"
        )
    );
    QVERIFY(WindowsCredentialFiles::readPrivateFile("C:/Profiles/DeskflowCandidate/tls/trusted-servers", bytes));
    QCOMPARE(bytes, QByteArray("SYNTHETIC-TRUST-NOT-A-CERT"));
    QVERIFY(WindowsCredentialFiles::ensurePrivateDirectory("C:/Profiles/DeskflowCandidate/recovery-sessions"));
    const auto before = fileCalls;
    QVERIFY(WindowsCredentialFiles::createPrivateFile("C:/Profiles/Deskflow/tls/key") == -1);
    QCOMPARE(fileCalls, before);
    sd.control = 0;
    QVERIFY(!WindowsCredentialFiles::readPrivateFile(path, bytes));
    reset();
  }

  void nativeDescriptorValidation_data()
  {
    QTest::addColumn<QString>("condition");
    QTest::addColumn<bool>("directory");
    QTest::addColumn<bool>("expected");
    for (const auto *kind :
         {"valid", "other-owner", "null-dacl", "unprotected", "inherited-ace", "propagating-ace", "deny-ace",
          "other-sid", "short-ace", "bad-acl", "no-ace", "two-aces", "limited-mask", "reparse", "hardlink", "pipe",
          "read-denied", "wrong-type"})
      for (bool dir : {false, true})
        QTest::newRow(qPrintable(QString(kind) + (dir ? "-dir" : "-file")))
            << QString(kind) << dir << (QString(kind) == "valid" || (QString(kind) == "hardlink" && dir));
  }
  void nativeDescriptorValidation()
  {
    QFETCH(QString, condition);
    QFETCH(bool, directory);
    QFETCH(bool, expected);
    winfixture::reset();
    DWORD own = 1;
    using namespace winfixture;
    if (directory)
      info.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    if (condition == "other-owner")
      sd.owner = 2;
    else if (condition == "null-dacl")
      sd.present = FALSE;
    else if (condition == "unprotected")
      sd.control = 0;
    else if (condition == "inherited-ace")
      ace.Header.AceFlags = 16;
    else if (condition == "propagating-ace")
      ace.Header.AceFlags = 3;
    else if (condition == "deny-ace")
      ace.Header.AceType = 1;
    else if (condition == "other-sid")
      ace.SidStart = 2;
    else if (condition == "short-ace")
      ace.Header.AceSize = 4;
    else if (condition == "bad-acl")
      aclValid = false;
    else if (condition == "no-ace")
      sd.acl.AceCount = 0;
    else if (condition == "two-aces")
      sd.acl.AceCount = 2;
    else if (condition == "limited-mask")
      ace.Mask = 1;
    else if (condition == "reparse")
      info.dwFileAttributes |= FILE_ATTRIBUTE_REPARSE_POINT;
    else if (condition == "hardlink")
      info.nNumberOfLinks = 2;
    else if (condition == "pipe")
      type = 3;
    else if (condition == "read-denied")
      securityReadable = false;
    else if (condition == "wrong-type")
      info.dwFileAttributes ^= FILE_ATTRIBUTE_DIRECTORY;
    QCOMPARE(auditWindowsCredentialHandleForTest(&sd, &own, directory), expected);
    QCOMPARE(fileCalls, 0);
  }
};
QTEST_GUILESS_MAIN(WindowsCredentialBackendTests)
#include "WindowsCredentialBackendTests.moc"
