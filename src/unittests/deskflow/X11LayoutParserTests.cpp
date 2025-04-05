/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "X11LayoutParserTests.h"

#include "../../lib/deskflow/unix/X11LayoutsParser.h"

void X11LayoutParserTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(kTestDir));

  QFile correctEvdevFile(kTestCorrectFile);
  QVERIFY(correctEvdevFile.open(QIODevice::WriteOnly));
  correctEvdevFile.write(kCorrectEvContents.toUtf8());
  correctEvdevFile.close();

  QVERIFY(correctEvdevFile.open(QIODevice::ReadOnly));
  QCOMPARE(correctEvdevFile.readAll(), kCorrectEvContents);
  correctEvdevFile.close();

  QFile futureEvdevFile(kTestFutureFile);
  QVERIFY(futureEvdevFile.open(QIODevice::WriteOnly));
  futureEvdevFile.write(kFutureEvContents.toUtf8());
  futureEvdevFile.close();

  QVERIFY(futureEvdevFile.open(QIODevice::ReadOnly));
  QCOMPARE(futureEvdevFile.readAll(), kFutureEvContents);
  futureEvdevFile.close();

  QFile badEvdevFile1(kTestBadFile1);
  QVERIFY(badEvdevFile1.open(QIODevice::WriteOnly));
  badEvdevFile1.write(kBadEv1Contents.toUtf8());
  badEvdevFile1.close();

  QVERIFY(badEvdevFile1.open(QIODevice::ReadOnly));
  QCOMPARE(badEvdevFile1.readAll(), kBadEv1Contents);
  badEvdevFile1.close();

  QFile badEvdevFile2(kTestBadFile2);
  QVERIFY(badEvdevFile2.open(QIODevice::WriteOnly));
  badEvdevFile2.write(kBadEv2Contents.toUtf8());
  badEvdevFile2.close();

  QVERIFY(badEvdevFile2.open(QIODevice::ReadOnly));
  QCOMPARE(badEvdevFile2.readAll(), kBadEv2Contents);
  badEvdevFile2.close();

  QFile badEvdevFile3(kTestBadFile3);
  QVERIFY(badEvdevFile3.open(QIODevice::WriteOnly));
  badEvdevFile3.write(kBadEv3Contents.toUtf8());
  badEvdevFile3.close();

  QVERIFY(badEvdevFile3.open(QIODevice::ReadOnly));
  QCOMPARE(badEvdevFile3.readAll(), kBadEv3Contents);
  badEvdevFile3.close();
}

void X11LayoutParserTests::xmlParse()
{
  const QString badPath = QStringLiteral("%1/%2").arg(kTestDir, "notafile");
  QVERIFY(X11LayoutsParser::getX11LanguageList(badPath.toStdString()).empty());

  // we would like to check the method
  // X11LayoutsParser::getAllLanguageData(kTestCorrectFile.toStdString());
  // however it's logic is flawed making as the return list size is based on number of languges host has installed
  // this is correct on a real system but makes it hard to test the method

  QVERIFY(X11LayoutsParser::getX11LanguageList(kTestBadFile1.toStdString()).empty());
  QVERIFY(X11LayoutsParser::getX11LanguageList(kTestBadFile2.toStdString()).empty());
  QVERIFY(X11LayoutsParser::getX11LanguageList(kTestBadFile3.toStdString()).empty());
}

void X11LayoutParserTests::convertLayouts()
{
  QCOMPARE(X11LayoutsParser::convertLayotToISO(kTestCorrectFile.toStdString(), "us", true), "en");
  QCOMPARE(X11LayoutsParser::convertLayotToISO(kTestBadFile1.toStdString(), "us", true), "");
  QCOMPARE(X11LayoutsParser::convertLayotToISO(kTestFutureFile.toStdString(), "us", true), "");
}

QTEST_MAIN(X11LayoutParserTests)
