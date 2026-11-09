// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/Settings.h"
#include <QCoreApplication>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <cstdio>
class FirstRunTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void freshProfileGeneratesEditableStableNameWithoutIdentity()
  {
    QTemporaryDir profile;
    QVERIFY(profile.isValid());
    auto run = [&](const QString &name = QString()) {
      QProcess child;
      QStringList args{"--profile", profile.path()};
      if (!name.isNull())
        args << name;
      child.start(QCoreApplication::applicationFilePath(), args);
      if (!child.waitForFinished(10000) || child.exitCode() != 0 || child.exitStatus() != QProcess::NormalExit)
        return QJsonObject{};
      return QJsonDocument::fromJson(child.readAllStandardOutput()).object();
    };
    const auto first = run();
    QVERIFY(!first.isEmpty());
    QVERIFY(!first["name"].toString().isEmpty());
    QVERIFY(
        QRegularExpression(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9_.-]*$")).match(first["name"].toString()).hasMatch()
    );
    QCOMPARE(run()["name"], first["name"]);
    QCOMPARE(run("My-work-PC")["name"].toString(), QString("My-work-PC"));
    QCOMPARE(run()["name"].toString(), QString("My-work-PC"));
    QDirIterator files(profile.path(), QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
      const auto path = files.next();
      QVERIFY2(path.endsWith(".conf") || path.endsWith(".state"), qPrintable(path));
    }
  }
};
int main(int argc, char **argv)
{
  const bool child = argc >= 3 && QByteArray(argv[1]) == "--profile";
  if (child) {
    const QString root = QString::fromLocal8Bit(argv[2]);
    if (!QDir::isAbsolutePath(root) || !QDir(root).exists())
      return 2;
    qputenv("XDG_CONFIG_HOME", root.toUtf8());
    qputenv("XDG_STATE_HOME", (root + "/state").toUtf8());
    QDir().mkpath(root + "/Deskflow");
    QDir().mkpath(root + "/state");
    QFile file(root + "/Deskflow/Deskflow.conf");
    if (!file.exists() && !file.open(QIODevice::WriteOnly))
      return 3;
  }
  QCoreApplication app(argc, argv);
  if (child) {
    if (argc >= 4)
      Settings::setValue(Settings::Core::ComputerName, QString::fromLocal8Bit(argv[3]));
    const QJsonObject data{{"name", Settings::value(Settings::Core::ComputerName).toString()}};
    const auto bytes = QJsonDocument(data).toJson(QJsonDocument::Compact);
    std::fwrite(bytes.constData(), 1, bytes.size(), stdout);
    return 0;
  }
  FirstRunTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "FirstRunSettingsTests.moc"
