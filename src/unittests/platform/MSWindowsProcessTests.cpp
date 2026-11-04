/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"
#include "platform/MSWindowsProcess.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>

using deskflow::platform::MSWindowsProcess;

class MSWindowsProcessTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void command_data()
  {
    QTest::addColumn<QString>("path");
    QTest::newRow("english") << QStringLiteral("C:\\Users\\TestUser");
    QTest::newRow("korean") << QString::fromWCharArray(L"C:\\Users\\\uD608\uC561\uAC80\uC0AC");
    QTest::newRow("spaces") << QStringLiteral("C:\\Users\\Test User");
    QTest::newRow("japanese") << QString::fromWCharArray(L"C:\\Users\\\u65E5\u672C\u8A9E");
    QTest::newRow("chinese") << QString::fromWCharArray(L"C:\\Users\\\u4E2D\u6587\u7528\u6237");
    QTest::newRow("special") << QStringLiteral("C:\\Users\\Test & (User)! + # %");
    QTest::newRow("supplementary") << QString::fromUtf8("C:\\Users\\\xF0\x9F\xA7\xAA");
  }

  void command()
  {
    QFETCH(QString, path);
    const auto command = QStringLiteral(
                             "\"C:\\Program Files\\Deskflow\\deskflow-core.exe\" client --settings "
                             "\"%1\\AppData\\Roaming\\Deskflow\\Deskflow.conf\""
    )
                             .arg(path);
    QCOMPARE(MSWindowsProcess::commandFromUtf8(command.toStdString()), command.toStdWString());
  }

  void emptyCommand()
  {
    QVERIFY(MSWindowsProcess::commandFromUtf8({}).empty());
  }

  void invalidUtf8_data()
  {
    QTest::addColumn<QByteArray>("bytes");
    QTest::newRow("truncated") << QByteArray::fromHex("eab0");
    QTest::newRow("continuation") << QByteArray::fromHex("80");
    QTest::newRow("overlong") << QByteArray::fromHex("c0af");
    QTest::newRow("surrogate") << QByteArray::fromHex("eda080");
    QTest::newRow("out-of-range") << QByteArray::fromHex("f4908080");
  }

  void invalidUtf8()
  {
    QFETCH(QByteArray, bytes);
    QVERIFY_EXCEPTION_THROWN(MSWindowsProcess::commandFromUtf8(bytes.toStdString()), std::invalid_argument);
  }

  void embeddedNul()
  {
    QVERIFY_EXCEPTION_THROWN(MSWindowsProcess::commandFromUtf8(std::string("abc\0def", 7)), std::invalid_argument);
  }

  void lengthLimit()
  {
    QCOMPARE(MSWindowsProcess::commandFromUtf8(std::string(32766, 'x')).size(), size_t(32766));
    QVERIFY_EXCEPTION_THROWN(MSWindowsProcess::commandFromUtf8(std::string(32767, 'x')), std::length_error);
    const auto korean = QString(32766, QChar(0xD608)).toStdString();
    QCOMPARE(MSWindowsProcess::commandFromUtf8(korean).size(), size_t(32766));
    QVERIFY_EXCEPTION_THROWN(MSWindowsProcess::commandFromUtf8(korean + "x"), std::length_error);
    QVERIFY_EXCEPTION_THROWN(MSWindowsProcess::commandFromUtf8(std::string(1000000, 'x')), std::length_error);
  }

  void boundedView()
  {
    const std::string buffer = "command trailing data";
    QCOMPARE(MSWindowsProcess::commandFromUtf8(std::string_view(buffer.data(), 7)), std::wstring(L"command"));
  }

  void launchArguments_data()
  {
    command_data();
  }

  void launchArguments()
  {
    QFETCH(QString, path);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto captureFile = directory.filePath(QString::fromWCharArray(L"\uD608\uC561\uAC80\uC0AC result.json"));
    for (const auto &mode : {QStringLiteral("client"), QStringLiteral("server")}) {
      const auto command =
          QStringLiteral(
              "\"%1\" --capture-command \"%2\" %3 --settings \"%4\\AppData\\Roaming\\Deskflow\\Deskflow.conf\""
          )
              .arg(QCoreApplication::applicationFilePath(), captureFile, mode, path);
      MSWindowsProcess process(MSWindowsProcess::commandFromUtf8(command.toStdString()));
      QVERIFY(process.startInForeground());
      QCOMPARE(process.waitForExit(), DWORD(0));
      QFile file(captureFile);
      QVERIFY(file.open(QIODevice::ReadOnly));
      const auto arguments = QJsonDocument::fromJson(file.readAll()).array();
      QCOMPARE(arguments.at(3).toString(), mode);
      QCOMPARE(arguments.at(4).toString(), QStringLiteral("--settings"));
      QCOMPARE(arguments.at(5).toString(), path + QStringLiteral("\\AppData\\Roaming\\Deskflow\\Deskflow.conf"));
    }
  }

private:
  Log m_log;
};

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  const auto arguments = app.arguments();
  if (arguments.size() > 2 && arguments.at(1) == QStringLiteral("--capture-command")) {
    QFile file(arguments.at(2));
    if (!file.open(QIODevice::WriteOnly)) {
      return 1;
    }
    return file.write(QJsonDocument(QJsonArray::fromStringList(arguments)).toJson()) < 0 ? 1 : 0;
  }

  MSWindowsProcessTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "MSWindowsProcessTests.moc"
