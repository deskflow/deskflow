/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "GuiServerConfigTests.h"

#include "common/Settings.h"
#include "gui/config/ServerConfig.h"

#include <QTextStream>

namespace {

QString linksSection(const ServerConfig &config)
{
  QString text;
  QTextStream stream(&text);
  stream << config;
  return text.left(text.indexOf(QStringLiteral("section: options")));
}

void writeScreens(const QStringList &names)
{
  auto &settings = Settings::proxy();
  settings.beginGroup("internalConfig");
  settings.remove("");
  settings.beginWriteArray("screens");
  for (int i = 0; i < names.size(); i++) {
    settings.setArrayIndex(i);
    settings.setValue("name", names[i]);
  }
  settings.endArray();
  settings.endGroup();
}

} // namespace

void GuiServerConfigTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));
  QFile::remove(m_settingsFile);

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);
  Settings::setValue(Settings::Core::ComputerName, "srv");
}

void GuiServerConfigTests::init()
{
  writeScreens({});
}

void GuiServerConfigTests::migratesLegacyGrid()
{
  // The default 5x3 grid, with the server in the middle cell and one computer either side.
  QStringList cells(15);
  cells[6] = "left";
  cells[7] = "srv";
  cells[8] = "right";
  writeScreens(cells);

  const ServerConfig config;
  QCOMPARE(
      linksSection(config), QStringLiteral("section: links\n"
                                           "\tleft:\n"
                                           "\t\tright = srv\n"
                                           "\tsrv:\n"
                                           "\t\tright = right\n"
                                           "\t\tleft = left\n"
                                           "\tright:\n"
                                           "\t\tleft = srv\n"
                                           "end\n\n")
  );
}

void GuiServerConfigTests::addClientPlacesNextToServer()
{
  ServerConfig config;
  config.addClient("laptop");

  QCOMPARE(
      linksSection(config), QStringLiteral("section: links\n"
                                           "\tsrv:\n"
                                           "\t\tleft = laptop\n"
                                           "\tlaptop:\n"
                                           "\t\tright = srv\n"
                                           "end\n\n")
  );
}

void GuiServerConfigTests::geometryRoundTrip()
{
  ServerConfig config;
  config.addClient("laptop");
  QVERIFY(config.resizeComputer("laptop", QSize(1920, 540)));
  config.commit();

  const ServerConfig reloaded;
  QCOMPARE(reloaded.computers(), std::as_const(config).computers());
}

void GuiServerConfigTests::resizeKeepsNoOverlap()
{
  ServerConfig config;
  config.addClient("laptop"); // placed left of the server, at x = -1920
  QVERIFY(config.resizeComputer("laptop", QSize(2560, 1440)));

  // Growing to the right would overlap the server, so the laptop is pushed past it.
  QCOMPARE(std::as_const(config).computers()[1].geometry(), QRect(1920, 0, 2560, 1440));
  QVERIFY(!config.resizeComputer("laptop", QSize(2560, 1440)));
  QVERIFY(!config.resizeComputer("missing", QSize(10, 10)));
}

QTEST_MAIN(GuiServerConfigTests)
