/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "gui/handoff/Transfer.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <stdexcept>

using namespace deskflow::handoff;

class HandoffTests : public QObject
{
  Q_OBJECT
private:
  QByteArray packet(const QJsonArray &files, const QByteArray &payload = {}, const QString &url = {})
  {
    const auto header = QJsonDocument(
                            QJsonObject{{"version", 1}, {"files", files}, {"application", ""}, {"url", url}}
    ).toJson(QJsonDocument::Compact);
    return QByteArray::number(header.size()) + '\n' + header + payload;
  }
  QJsonObject entry(const QString &name, const QString &size = "0")
  {
    return {{"name", name}, {"size", size}};
  }

private Q_SLOTS:
  void roundTrip()
  {
    QTemporaryDir source;
    QTemporaryDir downloads;
    const auto path = source.filePath("Harry's notes ; $(touch nope) ü.txt");
    QFile original(path);
    QVERIFY(original.open(QIODevice::WriteOnly));
    const QByteArray data = QByteArray(200000, 'x') + QByteArray("\0\1\2", 3);
    QCOMPARE(original.write(data), data.size());
    original.close();
    QFile empty(source.filePath("empty.txt"));
    QVERIFY(empty.open(QIODevice::WriteOnly));
    empty.close();
    QBuffer wire;
    QVERIFY(wire.open(QIODevice::ReadWrite));
    send(wire, {{path, empty.fileName()}, "com.apple.TextEdit", {}});
    wire.seek(0);
    const auto received = receive(wire, downloads.path());
    QCOMPARE(received.application, "com.apple.TextEdit");
    QCOMPARE(received.files.size(), 2);
    QFile copied(received.files.first());
    QVERIFY(copied.open(QIODevice::ReadOnly));
    QCOMPARE(copied.readAll(), data);
    QCOMPARE(QFileInfo(copied).fileName(), QFileInfo(original).fileName());
    QCOMPARE(QFileInfo(received.files.last()).size(), 0);
    QVERIFY(original.exists());
    QVERIFY(!(copied.permissions() & (QFile::WriteGroup | QFile::WriteOther)));
    QCOMPARE(QDir(downloads.path()).entryList(QDir::Dirs | QDir::NoDotAndDotDot).size(), 1);
  }

  void urlAndApplication()
  {
    for (const Request request :
         {Request{{}, "com.apple.Safari", QUrl("https://example.org/a?q=ü")}, Request{{}, "com.apple.TextEdit", {}}}) {
      QTemporaryDir downloads;
      QBuffer wire;
      wire.open(QIODevice::ReadWrite);
      send(wire, request);
      wire.seek(0);
      const auto result = receive(wire, downloads.path());
      QCOMPARE(result.url, request.url);
      QCOMPARE(result.application, request.application);
      QVERIFY(QDir(downloads.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty());
    }
  }

  void rejectedPackets_data()
  {
    QTest::addColumn<QByteArray>("bytes");
    for (const auto &name : {"../outside", "/absolute", "..", "a/b", "a\\b", ""})
      QTest::newRow(qPrintable(QString("path %1").arg(name))) << packet({entry(name)}, "DONE");
    QTest::newRow("nul") << packet({entry(QString("x") + QChar(0))}, "DONE");
    QTest::newRow("duplicate") << packet({entry("a"), entry("a")}, "DONE");
    QTest::newRow("case duplicate") << packet({entry("A"), entry("a")}, "DONE");
    QTest::newRow("unicode duplicate") << packet({entry("é"), entry(QString("e") + QChar(0x301))}, "DONE");
    QTest::newRow("negative size") << packet({entry("a", "-1")}, "DONE");
    QTest::newRow("huge size") << packet({entry("a", "1099511627777")}, "DONE");
    QTest::newRow("invalid size") << packet({entry("a", "foo")}, "DONE");
    QTest::newRow("truncated file") << packet({entry("a", "10")}, "hi");
    QTest::newRow("no commit") << packet({entry("a", "2")}, "hi");
    QTest::newRow("wrong commit") << packet({entry("a", "2")}, "hiFAIL");
    QTest::newRow("trailing bytes") << packet({entry("a")}, "DONEoops");
    QTest::newRow("oversized header") << QByteArray("1048577\n");
    QTest::newRow("bad length") << QByteArray("-1\n");
    QTest::newRow("missing length") << QByteArray("\n");
    QTest::newRow("bad json") << QByteArray("4\noops");
    QTest::newRow("nothing") << packet({}, "DONE");
    QTest::newRow("local url") << packet({}, "DONE", "file:///etc/passwd");
    QTest::newRow("javascript url") << packet({}, "DONE", "javascript:alert(1)");
    QTest::newRow("credentials") << packet({}, "DONE", "https://user:password@example.org");
    QTest::newRow("url and file") << packet({entry("a")}, "DONE", "https://example.org");
  }

  void rejectedPackets()
  {
    QFETCH(QByteArray, bytes);
    QTemporaryDir downloads;
    QFile existing(downloads.filePath("existing.txt"));
    QVERIFY(existing.open(QIODevice::WriteOnly));
    existing.write("keep me");
    existing.close();
    QBuffer wire(&bytes);
    wire.open(QIODevice::ReadOnly);
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, receive(wire, downloads.path()));
    QCOMPARE(
        QDir(downloads.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot),
        QStringList{"existing.txt"}
    );
    QVERIFY(existing.open(QIODevice::ReadOnly));
    QCOMPARE(existing.readAll(), "keep me");
  }

  void senderRejectsDirectoriesAndSymlinks()
  {
    QTemporaryDir source;
    QBuffer wire;
    wire.open(QIODevice::ReadWrite);
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, send(wire, {{source.path()}, {}, {}}));
    QFile file(source.filePath("file"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    QVERIFY(QFile::link(file.fileName(), source.filePath("link")));
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, send(wire, {{source.filePath("link")}, {}, {}}));
    QVERIFY(wire.data().isEmpty());
  }

  void cancellation()
  {
    bool cancelled = false;
    auto thread = QThread::create([&] {
      QThread::currentThread()->requestInterruption();
      QBuffer wire;
      wire.open(QIODevice::ReadWrite);
      try {
        send(wire, {{}, "com.apple.TextEdit", {}});
      } catch (const std::runtime_error &) {
        cancelled = wire.data().isEmpty();
      }
    });
    thread->start();
    QVERIFY(thread->wait(5000));
    delete thread;
    QVERIFY(cancelled);
  }

  void separateTransfersNeverOverwrite()
  {
    QTemporaryDir source;
    QTemporaryDir downloads;
    QFile original(source.filePath("same.txt"));
    QVERIFY(original.open(QIODevice::WriteOnly));
    original.write("original");
    original.close();
    QStringList copies;
    for (int i = 0; i < 2; ++i) {
      QBuffer wire;
      wire.open(QIODevice::ReadWrite);
      send(wire, {{original.fileName()}, {}, {}});
      wire.seek(0);
      copies.append(receive(wire, downloads.path()).files.first());
    }
    QVERIFY(copies.first() != copies.last());
    for (const auto &path : copies) {
      QFile file(path);
      QVERIFY(file.open(QIODevice::ReadOnly));
      QCOMPARE(file.readAll(), "original");
    }
  }

  void destinations()
  {
    for (const auto &host : {"mac-studio.local", "harry@mac-mini.local", "my-ssh-alias", "192.168.1.5"}) {
      QVERIFY(validDestination(host));
      const auto arguments = sshArguments(host);
      QVERIFY(arguments.contains("-oStrictHostKeyChecking=yes"));
      QVERIFY(arguments.contains("-oBatchMode=yes"));
      QCOMPARE(arguments.at(arguments.size() - 2), host);
      QCOMPARE(arguments.last(), "'/Applications/Deskflow.app/Contents/MacOS/deskflow-handoff'");
    }
    for (const auto &host : {"", "-oProxyCommand=oops", "a;touch /tmp/oops", "user@-host", "a\nb", "a b", "$(oops)"}) {
      QVERIFY(!validDestination(host));
      QVERIFY_THROWS_EXCEPTION(std::runtime_error, sshArguments(host));
    }
  }
};

QTEST_GUILESS_MAIN(HandoffTests)
#include "HandoffTests.moc"
