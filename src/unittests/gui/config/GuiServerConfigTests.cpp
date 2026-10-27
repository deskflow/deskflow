/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "GuiServerConfigTests.h"

#include "common/Settings.h"
#include "gui/config/ServerConfig.h"

namespace {

void writeScreens(const QStringList &names)
{
  auto &settings = Settings::proxy();
  for (const auto &name : names)
    settings.remove(QStringLiteral("computer_%1").arg(name));
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

void GuiServerConfigTests::recall_legacyGrid_geometryFromCell()
{
  // The default 5x3 grid, with the server in the middle cell and one computer either side.
  QStringList cells(15);
  cells[6] = "left";
  cells[7] = "srv";
  cells[8] = "right";
  writeScreens(cells);

  const ServerConfig config;

  QCOMPARE(
      config.computers().rects(), (QMap<QString, QRect>{
                                      {"left", QRect(160, 90, 160, 90)},
                                      {"srv", QRect(320, 90, 160, 90)},
                                      {"right", QRect(480, 90, 160, 90)},
                                  })
  );
}

void GuiServerConfigTests::recall_legacyGrid_geometrySaved()
{
  QStringList cells(15);
  cells[7] = "srv";
  writeScreens(cells);

  const ServerConfig config;

  QCOMPARE(Settings::value(Settings::Computer::Geometry.arg("srv")).toRect(), QRect(320, 90, 160, 90));
}

void GuiServerConfigTests::addClient_newClient_leftOfServer()
{
  ServerConfig config;

  config.addClient("laptop");

  QCOMPARE(std::as_const(config).computers().rects().value("laptop"), QRect(-160, 0, 160, 90));
}

void GuiServerConfigTests::commit_addedClient_geometryInComputerSettings()
{
  ServerConfig config;
  config.addClient("laptop");

  config.commit();

  QCOMPARE(
      Settings::value(Settings::Computer::Geometry.arg("laptop")).toRect(),
      std::as_const(config).computers()[1].geometry()
  );
}

void GuiServerConfigTests::recall_committedLayout_sameComputers()
{
  ServerConfig config;
  config.addClient("laptop");
  config.commit();

  const ServerConfig reloaded;

  QCOMPARE(reloaded.computers(), std::as_const(config).computers());
}

QTEST_MAIN(GuiServerConfigTests)
