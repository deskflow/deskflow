/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class X11LayoutParserTests : public QObject
{
  Q_OBJECT
private slots:
  // Test are run in order top to bottom
  void initTestCase();
  void xmlParse();
  void convertLayouts();

private:
  Arch m_arch;
  Log m_log;
  const QString kTestDir = "tmp/test";
  const QString kTestCorrectFile = "tmp/test/correctEvdev.xml";
  const QString kTestFutureFile = "tmp/test/evdevFromFuture.xml";
  const QString kTestBadFile1 = "tmp/test/evdevBad1.xml";
  const QString kTestBadFile2 = "tmp/test/evdevBad2.xml";
  const QString kTestBadFile3 = "tmp/test/evdevBad3.xml";

  const QString kCorrectEvContents = QStringLiteral(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<xkbConfigRegistry version=\"1.1\">\n"
      "  <layoutList>\n"
      "    <layout>\n"
      "      <configItem>\n"
      "        <name>us</name>\n"
      "        <!-- Keyboard indicator for English layouts -->\n"
      "        <shortDescription>en</shortDescription>\n"
      "        <description>English (US)</description>\n"
      "        <languageList>\n"
      "          <iso639Id>eng</iso639Id>\n"
      "        </languageList>\n"
      "      </configItem>\n"
      "      <variantList>\n"
      "        <variant>\n"
      "          <configItem>\n"
      "            <name>eng</name>\n"
      "            <shortDescription>eng</shortDescription>\n"
      "            <description>Cherokee</description>\n"
      "            <languageList>\n"
      "              <iso639Id>eng</iso639Id>\n"
      "            </languageList>\n"
      "          </configItem>\n"
      "        </variant>\n"
      "      </variantList>\n"
      "    </layout>\n"
      "    <layout>\n"
      "      <configItem>\n"
      "        <name>ru</name>\n"
      "        <!-- Keyboard indicator for Russian layouts -->\n"
      "        <shortDescription>ru</shortDescription>\n"
      "        <description>Russian</description>\n"
      "        <languageList>\n"
      "          <iso639Id>rus</iso639Id>\n"
      "        </languageList>\n"
      "      </configItem>\n"
      "    </layout>\n"
      "  </layoutList>\n"
      "</xkbConfigRegistry>\n"
  );

  const QString kFutureEvContents = QStringLiteral(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<xkbConfigRegistry version=\"1.1\">\n"
      "  <layoutList>\n"
      "    <layout>\n"
      "      <configItem>\n"
      "        <name>futureLangName</name>\n"
      "        <languageList>\n"
      "          <iso639Id>fln</iso639Id>\n"
      "        </languageList>\n"
      "      </configItem>\n"
      "    </layout>\n"
      "  </layoutList>\n"
      "</xkbConfigRegistry>\n"
  );

  const QString kBadEv1Contents = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

  const QString kBadEv2Contents = QStringLiteral(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<xkbConfigRegistry version=\"1.1\">\n"
      "</xkbConfigRegistry>"
  );

  const QString kBadEv3Contents = QStringLiteral(
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<xkbConfigRegistry version=\"1.1\">\n"
      "  <layoutList>\n"
      "    <layout>\n"
      "    </layout>\n"
      "  </layoutList>\n"
      "</xkbConfigRegistry>\n"
  );
};
