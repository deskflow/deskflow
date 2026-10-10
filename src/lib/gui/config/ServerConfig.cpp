/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfig.h"

#include "common/Hotkey.h"
#include "common/Settings.h"
#include "gui/config/LayoutLinks.h"

#include <QAbstractButton>
#include <QPushButton>

using enum ComputerConfig::SwitchCorner;
using enum ComputerConfig::Fix;

ServerConfig::ServerConfig()
{
  recall();
}

bool ServerConfig::save(const QString &fileName) const
{
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;

  save(file);
  file.close();

  return true;
}

bool ServerConfig::operator==(const ServerConfig &sc) const
{
  return m_computers == sc.m_computers && //
         m_Hotkeys == sc.m_Hotkeys;       //
}

void ServerConfig::save(QFile &file) const
{
  QTextStream outStream(&file);
  outStream << *this;
}

void ServerConfig::commit()
{
  qDebug("committing server config");

  settings().beginGroup("internalConfig");
  settings().remove("");

  settings().beginWriteArray("screens");
  for (int i = 0; i < computers().size(); i++) {
    settings().setArrayIndex(i);
    const auto &computer = computers()[i];
    computer.saveSettings(settings());
    auto computerName = Settings::value(Settings::Core::ComputerName).toString();
    if (computer.isServer() && computerName != computer.name()) {
      Settings::setValue(Settings::Core::ComputerName, computer.name());
    }
  }
  settings().endArray();

  settings().beginWriteArray("hotkeys");
  for (int i = 0; i < hotkeys().size(); i++) {
    settings().setArrayIndex(i);
    hotkeys()[i].saveSettings(settings().get());
  }
  settings().endArray();

  settings().endGroup();
}

void ServerConfig::recall()
{
  qDebug("recalling server config");

  settings().beginGroup("internalConfig");

  computers().clear();
  hotkeys().clear();

  // Layouts saved before free-form placement only stored the grid cell index.
  const int legacyColumns = Settings::value(Settings::Server::GridWidth).toInt();
  const auto cellSize = ComputerList::kDefaultSize;

  int numComputers = settings().beginReadArray("screens");
  for (int i = 0; i < numComputers; i++) {
    settings().setArrayIndex(i);
    Computer computer;
    computer.loadSettings(settings());
    if (computer.isNull())
      continue;
    if (!computer.geometry().isValid()) {
      const QPoint cell(i % legacyColumns, i / legacyColumns);
      computer.setGeometry(QRect(QPoint(cell.x() * cellSize.width(), cell.y() * cellSize.height()), cellSize));
    }
    if (getServerName() == computer.name()) {
      computer.markAsServer();
    }
    addComputer(computer);
  }
  settings().endArray();

  int numHotkeys = settings().beginReadArray("hotkeys");
  for (int i = 0; i < numHotkeys; i++) {
    settings().setArrayIndex(i);
    Hotkey h;
    h.loadSettings(settings().get());
    hotkeys().append(h);
  }
  settings().endArray();

  settings().endGroup();
}

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config)
{
  using namespace deskflow::gui::layout;

  QList<Placed> placed;
  for (const auto &computer : config.computers()) {
    if (!computer.isNull())
      placed.append({computer.name(), computer.geometry()});
  }
  const auto links = computeLinks(placed);

  outStream << "section: links" << Qt::endl;
  for (const auto &[name, _] : std::as_const(placed)) {
    outStream << "\t" << name << ":\n";
    for (const auto &link : links) {
      if (link.from == name)
        outStream << "\t\t" << formatLink(link) << Qt::endl;
    }
  }
  outStream << "end" << Qt::endl << Qt::endl;

  outStream << "section: options" << Qt::endl;

  for (const Hotkey &hotkey : config.hotkeys())
    outStream << hotkey;

  outStream << "end" << Qt::endl << Qt::endl;

  return outStream;
}

int ServerConfig::numComputers() const
{
  int rval = 0;

  for (const Computer &s : computers()) {
    if (!s.isNull())
      rval++;
  }

  return rval;
}

QString ServerConfig::getServerName() const
{
  return Settings::value(Settings::Core::ComputerName).toString();
}

void ServerConfig::updateServerName()
{
  for (auto &computer : computers()) {
    if (computer.isServer()) {
      computer.setName(Settings::value(Settings::Core::ComputerName).toString());
      break;
    }
  }
}

QString ServerConfig::configFile() const
{
  return Settings::value(Settings::Server::ExternalConfigFile).toString();
}

bool ServerConfig::useExternalConfig() const
{
  return Settings::value(Settings::Server::ExternalConfig).toBool();
}

bool ServerConfig::computerExists(const QString &computerName) const
{
  bool isExists = false;

  for (const auto &computer : computers()) {
    if (!computer.isNull() && computer.name() == computerName) {
      isExists = true;
      break;
    }
  }

  return isExists;
}

void ServerConfig::addClient(const QString &clientName)
{
  ensureServer();
  m_computers.addComputerByPriority(Computer(clientName));
}

void ServerConfig::ensureServer()
{
  const auto serverName = getServerName();
  int index = -1;
  if (findComputerName(serverName, index)) {
    m_computers[index].markAsServer();
    return;
  }

  Computer server(serverName);
  server.markAsServer();
  const QRect origin(QPoint(), ComputerList::kDefaultSize);
  server.setGeometry(
      deskflow::gui::layout::overlapsAny(origin, m_computers.rects())
          ? m_computers.freeSpotNextTo(origin, origin.size())
          : origin
  );
  m_computers.append(server);
}

bool ServerConfig::resizeComputer(const QString &name, const QSize &size)
{
  return m_computers.resizeComputer(name, size);
}

void ServerConfig::setConfigFile(const QString &configFile) const
{
  Settings::setValue(Settings::Server::ExternalConfigFile, configFile);
}

void ServerConfig::setUseExternalConfig(bool useExternalConfig) const
{
  Settings::setValue(Settings::Server::ExternalConfig, useExternalConfig);
}

bool ServerConfig::findComputerName(const QString &name, int &index)
{
  bool found = false;
  for (int i = 0; i < computers().size(); i++) {
    if (!computers()[i].isNull() && computers()[i].name().compare(name) == 0) {
      index = i;
      found = true;
      break;
    }
  }
  return found;
}

QSettingsProxy &ServerConfig::settings()
{
  return Settings::proxy();
}
