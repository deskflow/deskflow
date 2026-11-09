/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfig.h"
#include "common/DesktopWorkspace.h"

#include "common/Hotkey.h"
#include "common/Settings.h"

#include <QAbstractButton>
#include <QPushButton>
#include <QSaveFile>
#include <algorithm>

using enum ComputerConfig::SwitchCorner;
using enum ComputerConfig::Fix;

static const struct
{
  int x;
  int y;
  const char *name;
} neighbourDirs[] = {
    {1, 0, "right"},
    {-1, 0, "left"},
    {0, -1, "up"},
    {0, 1, "down"},

};

const int serverDefaultIndex = 7;

ServerConfig::ServerConfig(int columns, int rows) : m_computers(columns), m_columns(columns), m_rows(rows)
{
  recall();
}

bool ServerConfig::save(const QString &fileName) const
{
  if (!desktopLayoutError().isEmpty())
    return false;
  QSaveFile file(fileName);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;

  QTextStream stream(&file);
  stream << *this;
  stream.flush();
  return stream.status() == QTextStream::Ok && file.commit();
}

bool ServerConfig::operator==(const ServerConfig &sc) const
{
  return m_computers == sc.m_computers && //
         m_Hotkeys == sc.m_Hotkeys && m_desktopLayout == sc.m_desktopLayout && m_monitorLayout == sc.m_monitorLayout &&
         m_workspaceState == sc.m_workspaceState;
}

void ServerConfig::save(QFile &file) const
{
  QTextStream outStream(&file);
  outStream << *this;
}

void ServerConfig::setupComputers()
{
  computers().clear();
  hotkeys().clear();

  // There must always be computer objects for each cell in the computers QList.
  // Unused computers are identified by having an empty name.
  for (int i = 0; i < m_columns * m_rows; i++)
    addComputer(Computer());
}

void ServerConfig::commit()
{
  if (!desktopLayoutError().isEmpty()) {
    qWarning("refusing to commit an invalid desktop layout");
    return;
  }
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
  Settings::setValue(Settings::Server::DesktopLayout, m_desktopLayout);
  Settings::setValue(Settings::Server::MonitorLayout, m_monitorLayout);
  Settings::setValue(Settings::Server::WorkspaceDraft, m_workspaceState);
}

void ServerConfig::recall()
{
  qDebug("recalling server config");

  settings().beginGroup("internalConfig");

  m_columns = Settings::value(Settings::Server::GridWidth).toInt();
  m_rows = Settings::value(Settings::Server::GridHeight).toInt();

  // we need to know the number of columns and rows before we can set up
  // ourselves
  setupComputers();

  int numComputers = settings().beginReadArray("screens");
  Q_ASSERT(numComputers <= computers().size());
  for (int i = 0; i < numComputers; i++) {
    settings().setArrayIndex(i);
    computers()[i].loadSettings(settings());
    if (getServerName() == computers()[i].name()) {
      computers()[i].markAsServer();
    }
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
  m_desktopLayout = Settings::value(Settings::Server::DesktopLayout).toString();
  m_monitorLayout = Settings::value(Settings::Server::MonitorLayout).toByteArray();
  m_workspaceState = Settings::value(Settings::Server::WorkspaceDraft).toByteArray();
}

bool ServerConfig::setWorkspace(const QByteArray &state, const QByteArray &layout, bool apply)
{
  const auto decoded = deskflow::display::WorkspaceState::decode(state);
  if (!decoded || (apply && (!deskflow::display::Layout::decode(layout) || decoded->layout.encode() != layout)))
    return false;
  for (const auto &placement : decoded->layout.placements)
    if (!computerExists(placement.device))
      return false;
  m_workspaceState = state;
  if (apply)
    m_monitorLayout = layout;
  return true;
}

deskflow::gui::MonitorLayoutDraft ServerConfig::monitorLayoutDraft(bool reset) const
{
  auto topologies =
      deskflow::display::decodeTopologies(Settings::value(Settings::Server::MonitorTopologies).toByteArray());
  QStringList devices;
  for (const auto &computer : m_computers)
    if (!computer.isNull())
      devices.append(computer.name());
  return {std::move(topologies), devices, reset ? QByteArray() : m_monitorLayout};
}

deskflow::gui::DesktopLayout ServerConfig::desktopLayout() const
{
  if (hasDesktopLayout())
    return deskflow::gui::DesktopLayout::decode(m_desktopLayout);
  deskflow::gui::DesktopLayout result;
  for (int i = 0; i < m_computers.size(); ++i) {
    if (!m_computers[i].isNull())
      result.desktops.push_back(
          {m_computers[i].name().toStdString(), (i % m_columns) * 1440.0, (i / m_columns) * 900.0, 1440, 900}
      );
  }
  return result;
}

QString ServerConfig::desktopLayoutError() const
{
  if (!hasDesktopLayout())
    return {}; // Legacy grid output remains byte-for-byte compatible.
  const auto layout = desktopLayout();
  if (!layout.error.isEmpty())
    return layout.error;
  QStringList names;
  for (const auto &computer : m_computers)
    if (!computer.isNull())
      names.append(computer.name());
  return deskflow::gui::DesktopLayout::validate(layout.desktops, names);
}

bool ServerConfig::setDesktopLayout(const std::vector<deskflow::experience::Desktop> &desktops)
{
  QStringList names;
  for (const auto &computer : m_computers)
    if (!computer.isNull())
      names.append(computer.name());
  if (!deskflow::gui::DesktopLayout::validate(desktops, names).isEmpty())
    return false;
  m_desktopLayout = deskflow::gui::DesktopLayout::encode(desktops);
  return true;
}

int ServerConfig::adjacentComputerIndex(int idx, int deltaColumn, int deltaRow) const
{
  if (computers()[idx].isNull())
    return -1;

  // if we're at the left or right end of the table, don't find results going
  // further left or right
  if ((deltaColumn > 0 && (idx + 1) % m_columns == 0) || (deltaColumn < 0 && idx % m_columns == 0))
    return -1;

  int arrayPos = idx + deltaColumn + deltaRow * m_columns;

  if (arrayPos >= computers().size() || arrayPos < 0)
    return -1;

  return arrayPos;
}

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config)
{
  if (!config.desktopLayoutError().isEmpty()) {
    outStream.setStatus(QTextStream::WriteFailed);
    return outStream;
  }
  if (config.hasDesktopLayout()) {
    outStream << QString::fromStdString(deskflow::experience::exportLinks(config.desktopLayout().desktops)) << Qt::endl;
  } else {
    outStream << "section: links" << Qt::endl;

    for (int i = 0; const auto &computer : config.computers()) {
      if (!computer.isNull()) {
        outStream << "\t" << computer.name() << ":\n";
        for (const auto &neighbour : std::as_const(neighbourDirs)) {
          int idx = config.adjacentComputerIndex(i, neighbour.x, neighbour.y);
          if (idx != -1 && !config.computers()[idx].isNull())
            outStream << "\t\t" << neighbour.name << " = " << config.computers()[idx].name() << Qt::endl;
        }
      }
      i++;
    }

    outStream << "end" << Qt::endl << Qt::endl;
  }

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
      if (hasDesktopLayout() && desktopLayoutError().isEmpty()) {
        auto layout = desktopLayout().desktops;
        for (auto &desktop : layout)
          if (desktop.name == computer.name().toStdString())
            desktop.name = Settings::value(Settings::Core::ComputerName).toString().toStdString();
        m_desktopLayout = deskflow::gui::DesktopLayout::encode(layout);
      }
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

bool ServerConfig::isFull() const
{
  bool isFull = true;

  for (const auto &computer : computers()) {
    if (computer.isNull()) {
      isFull = false;
      break;
    }
  }

  return isFull;
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
  if (clientName.isEmpty() || computerExists(clientName))
    return;
  const auto oldLayout = desktopLayout();
  const bool extendLayout = hasDesktopLayout() && desktopLayoutError().isEmpty() && !computerExists(clientName);
  int serverIndex = -1;
  const auto computerName = Settings::value(Settings::Core::ComputerName).toString();

  if (findComputerName(computerName, serverIndex)) {
    m_computers[serverIndex].markAsServer();
  } else {
    fixNoServer(computerName, serverIndex);
  }

  m_computers.addComputerByPriority(Computer(clientName));
  if (extendLayout && computerExists(clientName)) {
    auto desktops = oldLayout.desktops;
    const auto rightmost = std::max_element(desktops.begin(), desktops.end(), [](const auto &a, const auto &b) {
      return a.x + a.width < b.x + b.width;
    });
    desktops.push_back({clientName.toStdString(), rightmost->x + rightmost->width, rightmost->y, 1440, 900});
    setDesktopLayout(desktops);
  }
}

void ServerConfig::ensureLocalServer()
{
  const auto name = Settings::value(Settings::Core::ComputerName).toString();
  int index = -1;
  if (findComputerName(name, index))
    m_computers[index].markAsServer();
  else
    fixNoServer(name, index);
}

QString ServerConfig::renameLayoutClient(const QString &oldName, const QString &newName)
{
  int index = -1;
  auto layout = desktopLayout();
  if (!hasDesktopLayout() || !layout.error.isEmpty() || !findComputerName(oldName, index))
    return QObject::tr("Select a computer in a readable custom layout.");
  if (m_computers[index].isServer())
    return QObject::tr("Rename this computer from the main window.");
  if (oldName == newName)
    return {};
  const auto nameError = deskflow::gui::DesktopLayout::validate({{newName.toStdString(), 0, 0, 1, 1}}, {newName});
  if (!nameError.isEmpty())
    return QObject::tr("Use a unique name with letters, numbers, dots, dashes or underscores.");
  for (const auto &computer : std::as_const(m_computers))
    if (computer.name().compare(newName, Qt::CaseInsensitive) == 0 ||
        computer.aliases().contains(newName, Qt::CaseInsensitive))
      return QObject::tr("That name or alias is already in use.");
  for (auto &desktop : layout.desktops)
    if (desktop.name == oldName.toStdString())
      desktop.name = newName.toStdString();
  m_computers[index].setName(newName);
  m_desktopLayout = deskflow::gui::DesktopLayout::encode(layout.desktops);
  for (auto &hotkey : m_Hotkeys) {
    for (int i = 0; i < hotkey.actions().size(); ++i) {
      auto &action = hotkey.actionAt(i);
      if (action.switchComputerName() == oldName)
        action.setSwitchComputerName(newName);
      if (action.typeComputerNames().contains(oldName)) {
        action.removeComputer(oldName);
        action.addComputer(newName);
      }
    }
  }
  return {};
}

QString ServerConfig::removeLayoutClient(const QString &name)
{
  int index = -1;
  auto layout = desktopLayout();
  if (!hasDesktopLayout() || !layout.error.isEmpty() || !findComputerName(name, index))
    return QObject::tr("Select a computer in a readable custom layout.");
  if (m_computers[index].isServer())
    return QObject::tr("The local server must stay in the layout.");
  auto names = std::as_const(m_computers)[index].aliases();
  names.append(name);
  for (const auto &hotkey : std::as_const(m_Hotkeys))
    for (const auto &action : hotkey.actions())
      for (const auto &target : names)
        if (action.switchComputerName() == target || action.typeComputerNames().contains(target))
          return QObject::tr("Edit shortcuts that refer to this computer before removing it.");
  std::erase_if(layout.desktops, [&](const auto &desktop) { return desktop.name == name.toStdString(); });
  m_computers[index] = Computer();
  // Removing a bridge may leave a disconnected draft. The editor can repair it,
  // while the existing validation prevents saving it to the core.
  m_desktopLayout = deskflow::gui::DesktopLayout::encode(layout.desktops);
  return {};
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

bool ServerConfig::fixNoServer(const QString &name, int &index)
{
  bool fixed = false;
  if (computers()[serverDefaultIndex].isNull()) {
    m_computers[serverDefaultIndex].setName(name);
    m_computers[serverDefaultIndex].markAsServer();
    index = serverDefaultIndex;
    fixed = true;
  }

  return fixed;
}

QSettingsProxy &ServerConfig::settings()
{
  return Settings::proxy();
}
