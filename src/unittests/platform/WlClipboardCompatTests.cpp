/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "WlClipboardCompatTests.h"
#include "deskflow/Clipboard.h"
#include "platform/WlClipboardCollection.h"
#include <QElapsedTimer>
#include <QFile>

void WlClipboardCompatTests::writeFixture(const QString &name, const QByteArray &data)
{
  QFile file(m_directory.filePath(name));
  QVERIFY(file.open(QIODevice::WriteOnly));
  QCOMPARE(file.write(data), data.size());
}

QByteArray WlClipboardCompatTests::readFixture(const QString &name) const
{
  QFile file(m_directory.filePath(name));
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return file.readAll();
}

void WlClipboardCompatTests::initTestCase()
{
  QVERIFY(m_directory.isValid());
  m_originalPath = qgetenv("PATH");
  m_originalOptIn = qgetenv("DESKFLOW_WL_CLIPBOARD_FALLBACK");
  qputenv("DESKFLOW_TEST_CLIPBOARD", m_directory.path().toUtf8());
  const QByteArray script = R"SH(#!/bin/sh
if [ -f "$DESKFLOW_TEST_CLIPBOARD/fail" ]; then exit 1; fi
if [ -f "$DESKFLOW_TEST_CLIPBOARD/hang" ]; then exec /bin/sleep 30; fi
case "$0" in
  *wl-paste)
    case "$1" in
      --list-types) printf 'text/plain;charset=utf-8\n';;
      *) /bin/cat "$DESKFLOW_TEST_CLIPBOARD/data";;
    esac;;
  *wl-copy)
    printf '%s\n' "$@" > "$DESKFLOW_TEST_CLIPBOARD/args"
    /bin/cat > "$DESKFLOW_TEST_CLIPBOARD/data";;
esac
)SH";
  for (const auto &name : {QStringLiteral("wl-copy"), QStringLiteral("wl-paste")}) {
    writeFixture(name, script);
    QVERIFY(QFile::setPermissions(m_directory.filePath(name), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
  }
}

void WlClipboardCompatTests::init()
{
  qputenv("PATH", m_directory.path().toUtf8());
  qputenv("DESKFLOW_WL_CLIPBOARD_FALLBACK", "1");
  QFile::remove(m_directory.filePath("fail"));
  QFile::remove(m_directory.filePath("hang"));
  writeFixture("data", "initial text");
}

void WlClipboardCompatTests::cleanupTestCase()
{
  qputenv("PATH", m_originalPath);
  if (m_originalOptIn.isNull())
    qunsetenv("DESKFLOW_WL_CLIPBOARD_FALLBACK");
  else
    qputenv("DESKFLOW_WL_CLIPBOARD_FALLBACK", m_originalOptIn);
  qunsetenv("DESKFLOW_TEST_CLIPBOARD");
}

void WlClipboardCompatTests::requiresOptIn()
{
  qunsetenv("DESKFLOW_WL_CLIPBOARD_FALLBACK");
  deskflow::WlClipboardCollection disabled;
  QVERIFY(!disabled.isAvailable());
  qputenv("DESKFLOW_WL_CLIPBOARD_FALLBACK", "1");
  deskflow::WlClipboardCollection enabled;
  QVERIFY(enabled.isAvailable());
  QVERIFY(enabled.getClipboard(kClipboardClipboard));
  QVERIFY(enabled.getClipboard(kClipboardSelection));
  QVERIFY(!enabled.getClipboard(kClipboardEnd));
}

void WlClipboardCompatTests::missingTools()
{
  QTemporaryDir empty;
  QVERIFY(empty.isValid());
  qputenv("PATH", empty.path().toUtf8());
  deskflow::WlClipboardCollection backend;
  QVERIFY(!backend.isAvailable());
}

void WlClipboardCompatTests::readsConsecutiveCopies()
{
  deskflow::WlClipboardCollection backend;
  QVERIFY(backend.isAvailable());
  auto *source = backend.getClipboard(kClipboardClipboard);
  for (const auto &text : {QByteArray("First copy"), QByteArray("Tiếng Việt\nDòng thứ hai\n")}) {
    writeFixture("data", text);
    Clipboard result;
    QVERIFY(IClipboard::copy(&result, source));
    QVERIFY(result.open(0));
    QVERIFY(result.has(IClipboard::Format::Text));
    QCOMPARE(result.get(IClipboard::Format::Text), text.toStdString());
    result.close();
  }
}

void WlClipboardCompatTests::writesText_data()
{
  QTest::addColumn<QByteArray>("text");
  QTest::newRow("unicode-and-newlines") << QByteArray("Tiếng Việt\nDòng thứ hai\n");
  QTest::newRow("shell-metacharacters") << QByteArray("$(printf bad) `uname` ' \" ; --primary");
  QTest::newRow("larger-than-argument-limit") << QByteArray(256 * 1024, 'x');
  QTest::newRow("embedded-null") << QByteArray("a\0b", 3);
}

void WlClipboardCompatTests::writesText()
{
  QFETCH(QByteArray, text);
  WlClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::Text, text.toStdString());
  clipboard.close();
  QCOMPARE(readFixture("data"), text);
  QCOMPARE(readFixture("args"), QByteArray("--type\ntext/plain;charset=utf-8\n"));
}

void WlClipboardCompatTests::failedRead()
{
  writeFixture("fail", {});
  WlClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  QVERIFY(!clipboard.has(IClipboard::Format::Text));
  QVERIFY(clipboard.get(IClipboard::Format::Text).empty());
  clipboard.close();
}

void WlClipboardCompatTests::failedWrite()
{
  writeFixture("fail", {});
  WlClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  QVERIFY(!clipboard.empty());
  clipboard.close();
  QCOMPARE(readFixture("data"), QByteArray("initial text"));
}

void WlClipboardCompatTests::timedOutRead()
{
  writeFixture("hang", {});
  WlClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  QElapsedTimer timer;
  timer.start();
  QVERIFY(clipboard.get(IClipboard::Format::Text).empty());
  QVERIFY(timer.elapsed() < 5000);
  clipboard.close();
}

void WlClipboardCompatTests::stopsMonitoring()
{
  deskflow::WlClipboardCollection backend;
  backend.startMonitoring();
  backend.stopMonitoring();
  backend.startMonitoring();
  backend.stopMonitoring();
}

QTEST_GUILESS_MAIN(WlClipboardCompatTests)
