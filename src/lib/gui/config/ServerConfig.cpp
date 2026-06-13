/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Mikhail Slyusarev <slyusarevmikhail@gmail.com>
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfig.h"

#include "Hotkey.h"
#include "common/Settings.h"

#include <QAbstractButton>
#include <QPushButton>

#include <algorithm>

using enum ScreenConfig::Modifier;
using enum ScreenConfig::SwitchCorner;
using enum ScreenConfig::Fix;

const int serverDefaultIndex = 7;

// formats a link range as "(start,end)" percentages, omitted for a full edge
static QString formatInterval(double start, double end)
{
  if (start == 0.0 && end == 1.0)
    return {};
  return QStringLiteral("(%1,%2)").arg(qRound(start * 100)).arg(qRound(end * 100));
}

ServerConfig::ServerConfig(int columns, int rows) : m_Screens(columns), m_columns(columns), m_rows(rows)
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
  return m_Screens == sc.m_Screens &&                                   //
         m_Heartbeat == sc.m_Heartbeat &&                               //
         m_RelativeMouseMoves == sc.m_RelativeMouseMoves &&             //
         m_Win32KeepForeground == sc.m_Win32KeepForeground &&           //
         m_SwitchDelay == sc.m_SwitchDelay &&                           //
         m_SwitchDoubleTap == sc.m_SwitchDoubleTap &&                   //
         m_SwitchCornerSize == sc.m_SwitchCornerSize &&                 //
         m_SwitchCorners == sc.m_SwitchCorners &&                       //
         m_Hotkeys == sc.m_Hotkeys &&                                   //
         m_DefaultLockToScreenState == sc.m_DefaultLockToScreenState && //
         m_DisableLockToScreen == sc.m_DisableLockToScreen &&           //
         m_ClipboardSharing == sc.m_ClipboardSharing &&                 //
         m_ClipboardSharingSize == sc.m_ClipboardSharingSize;
}

void ServerConfig::save(QFile &file) const
{
  QTextStream outStream(&file);
  outStream << *this;
}

void ServerConfig::setupScreens()
{
  switchCorners().clear();
  screens().clear();
  hotkeys().clear();

  // m_NumSwitchCorners is used as a fixed size array. See Screen::init()
  for (int i = 0; i < static_cast<int>(NumSwitchCorners); i++)
    switchCorners() << false;

  // There must always be screen objects for each cell in the screens QList.
  // Unused screens are identified by having an empty name.
  for (int i = 0; i < m_columns * m_rows; i++)
    addScreen(Screen());
}

void ServerConfig::commit()
{
  qDebug("committing server config");

  settings().beginGroup("internalConfig");
  settings().remove("");

  settings().setValue("heartbeat", heartbeat());
  settings().setValue("relativeMouseMoves", relativeMouseMoves());
  settings().setValue("win32KeepForeground", win32KeepForeground());
  settings().setValue("switchDelay", switchDelay());
  settings().setValue("switchDoubleTap", switchDoubleTap());
  settings().setValue("switchCornerSize", switchCornerSize());
  settings().setValue("defaultLockToScreenState", defaultLockToScreenState());
  settings().setValue("disableLockToScreen", disableLockToScreen());
  settings().setValue("clipboardSharing", clipboardSharing());
  settings().setValue("clipboardSharingSize", QVariant::fromValue(clipboardSharingSize()));

  writeSettings(settings(), switchCorners(), "switchCorner");

  settings().beginWriteArray("screens");
  for (int i = 0; i < screens().size(); i++) {
    settings().setArrayIndex(i);
    const auto &screen = screens()[i];
    screen.saveSettings(settings());
    auto screenName = Settings::value(Settings::Core::ComputerName).toString();
    if (screen.isServer() && screenName != screen.name()) {
      Settings::setValue(Settings::Core::ComputerName, screen.name());
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

  m_columns = Settings::value(Settings::Server::GridWidth).toInt();
  m_rows = Settings::value(Settings::Server::GridHeight).toInt();

  // we need to know the number of columns and rows before we can set up
  // ourselves
  setupScreens();

  setHeartbeat(settings().value("heartbeat", 5000).toInt());
  setRelativeMouseMoves(settings().value("relativeMouseMoves", false).toBool());
  setWin32KeepForeground(settings().value("win32KeepForeground", false).toBool());
  setSwitchDelay(settings().value("switchDelay", 250).toInt());
  setSwitchDoubleTap(settings().value("switchDoubleTap", 250).toInt());
  setSwitchCornerSize(settings().value("switchCornerSize").toInt());
  setDefaultLockToScreenState(settings().value("defaultLockToScreenState", false).toBool());
  setDisableLockToScreen(settings().value("disableLockToScreen", false).toBool());
  setClipboardSharingSize(
      settings().value("clipboardSharingSize", (int)ServerConfig::defaultClipboardSharingSize()).toULongLong()
  );
  setClipboardSharing(settings().value("clipboardSharing", true).toBool());

  readSettings(settings(), switchCorners(), "switchCorner", false, static_cast<int>(NumSwitchCorners));

  int numScreens = settings().beginReadArray("screens");
  Q_ASSERT(numScreens <= screens().size());
  for (int i = 0; i < numScreens; i++) {
    settings().setArrayIndex(i);
    screens()[i].loadSettings(settings());
    if (getServerName() == screens()[i].name()) {
      screens()[i].markAsServer();
    }
  }
  settings().endArray();

  // clamp spans that no longer fit the grid or would cover another screen
  const int numRows = screens().size() / m_columns;
  for (int i = 0; i < screens().size(); i++) {
    auto &screen = screens()[i];
    if (screen.isNull())
      continue;
    const int column = i % m_columns;
    const int row = i / m_columns;

    int maxWidth = m_columns - column;
    for (int next = 1; next < maxWidth; next++) {
      if (!screens()[i + next].isNull()) {
        maxWidth = next;
        break;
      }
    }
    screen.setWidth(std::min(screen.width(), maxWidth));

    int maxHeight = numRows - row;
    for (int next = 1; next < maxHeight; next++) {
      if (!screens()[i + next * m_columns].isNull()) {
        maxHeight = next;
        break;
      }
    }
    screen.setHeight(std::min(screen.height(), maxHeight));
  }

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

QString ServerConfig::linksSection(int idx) const
{
  const int row = idx / m_columns;
  const int column = idx % m_columns;
  const int width = screens()[idx].width();
  const int height = screens()[idx].height();
  const auto lineTemplate = QStringLiteral("\t\t%1%2 = %3%4\n");

  QString out;

  // Walk one edge of the screen. A spanning screen can border several
  // screens along an edge, so each link covers the overlapping part of both
  // edges as a range. Vertical edges run along rows, horizontal ones along
  // columns; adjacentLine is the column or row just past that edge.
  const auto walkEdge = [&](const QString &dirName, bool verticalEdge, int adjacentLine) {
    const int start = verticalEdge ? row : column;
    const int extent = verticalEdge ? height : width;
    for (int p = start; p < start + extent;) {
      const int i = verticalEdge ? screens().screenIndexAt(adjacentLine, p) : screens().screenIndexAt(p, adjacentLine);
      if (i == -1) {
        p++;
        continue;
      }
      const int neighbourStart = verticalEdge ? i / m_columns : i % m_columns;
      const int neighbourExtent = verticalEdge ? screens()[i].height() : screens()[i].width();
      const double overlapStart = std::max(start, neighbourStart);
      const double overlapEnd = std::min(start + extent, neighbourStart + neighbourExtent);
      out.append(lineTemplate.arg(
          dirName, formatInterval((overlapStart - start) / extent, (overlapEnd - start) / extent), screens()[i].name(),
          formatInterval(
              (overlapStart - neighbourStart) / neighbourExtent, (overlapEnd - neighbourStart) / neighbourExtent
          )
      ));
      p = neighbourStart + neighbourExtent;
    }
  };

  walkEdge(QStringLiteral("right"), true, column + width);
  walkEdge(QStringLiteral("left"), true, column - 1);
  walkEdge(QStringLiteral("up"), false, row - 1);
  walkEdge(QStringLiteral("down"), false, row + height);

  return out;
}

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config)
{
  outStream << "section: screens" << Qt::endl;

  for (const Screen &s : config.screens()) {
    if (!s.isNull())
      outStream << s.screensSection();
  }

  outStream << "end" << Qt::endl << Qt::endl;

  outStream << "section: aliases" << Qt::endl;

  for (const Screen &s : config.screens()) {
    if (!s.isNull())
      outStream << s.aliasesSection();
  }

  outStream << "end" << Qt::endl << Qt::endl;

  outStream << "section: links" << Qt::endl;

  for (int i = 0; const auto &screen : config.screens()) {
    if (!screen.isNull())
      outStream << "\t" << screen.name() << ":\n" << config.linksSection(i);
    i++;
  }

  outStream << "end" << Qt::endl << Qt::endl;

  outStream << "section: options" << Qt::endl;

  if (Settings::value(Settings::Server::EnableHeatbeat).toBool())
    outStream << "\t" << "heartbeat = " << config.heartbeat() << Qt::endl;

  outStream << "\t"
            << "relativeMouseMoves = " << (config.relativeMouseMoves() ? "true" : "false") << Qt::endl;
  outStream << "\t"
            << "win32KeepForeground = " << (config.win32KeepForeground() ? "true" : "false") << Qt::endl;
  outStream << "\t"
            << "defaultLockToScreenState = " << (config.defaultLockToScreenState() ? "true" : "false") << Qt::endl;
  outStream << "\t"
            << "disableLockToScreen = " << (config.disableLockToScreen() ? "true" : "false") << Qt::endl;
  outStream << "\t"
            << "clipboardSharing = " << (config.clipboardSharing() ? "true" : "false") << Qt::endl;
  outStream << "\t"
            << "clipboardSharingSize = " << config.clipboardSharingSize() << Qt::endl;

  if (Settings::value(Settings::Server::EnableSwitchDelay).toBool())
    outStream << "\t"
              << "switchDelay = " << config.switchDelay() << Qt::endl;

  if (Settings::value(Settings::Server::EnableSwitchDoubleTap).toBool())
    outStream << "\t"
              << "switchDoubleTap = " << config.switchDoubleTap() << Qt::endl;

  outStream << "\t"
            << "switchCorners = none ";
  for (int i = 0; i < config.switchCorners().size(); i++)
    if (config.switchCorners()[i])
      outStream << "+" << ServerConfig::switchCornerName(i) << " ";
  outStream << Qt::endl;

  outStream << "\t"
            << "switchCornerSize = " << config.switchCornerSize() << Qt::endl;

  for (const Hotkey &hotkey : config.hotkeys())
    outStream << hotkey;

  outStream << "end" << Qt::endl << Qt::endl;

  return outStream;
}

int ServerConfig::numScreens() const
{
  int rval = 0;

  for (const Screen &s : screens()) {
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
  for (auto &screen : screens()) {
    if (screen.isServer()) {
      screen.setName(Settings::value(Settings::Core::ComputerName).toString());
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

  for (const auto &screen : screens()) {
    if (screen.isNull()) {
      isFull = false;
      break;
    }
  }

  return isFull;
}

bool ServerConfig::screenExists(const QString &screenName) const
{
  bool isExists = false;

  for (const auto &screen : screens()) {
    if (!screen.isNull() && screen.name() == screenName) {
      isExists = true;
      break;
    }
  }

  return isExists;
}

void ServerConfig::addClient(const QString &clientName)
{
  int serverIndex = -1;
  const auto screenName = Settings::value(Settings::Core::ComputerName).toString();

  if (findScreenName(screenName, serverIndex)) {
    m_Screens[serverIndex].markAsServer();
  } else {
    fixNoServer(screenName, serverIndex);
  }

  m_Screens.addScreenByPriority(Screen(clientName));
}

void ServerConfig::setConfigFile(const QString &configFile) const
{
  Settings::setValue(Settings::Server::ExternalConfigFile, configFile);
}

void ServerConfig::setUseExternalConfig(bool useExternalConfig) const
{
  Settings::setValue(Settings::Server::ExternalConfig, useExternalConfig);
}

bool ServerConfig::findScreenName(const QString &name, int &index)
{
  bool found = false;
  for (int i = 0; i < screens().size(); i++) {
    if (!screens()[i].isNull() && screens()[i].name().compare(name) == 0) {
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
  if (screens()[serverDefaultIndex].isNull()) {
    m_Screens[serverDefaultIndex].setName(name);
    m_Screens[serverDefaultIndex].markAsServer();
    index = serverDefaultIndex;
    fixed = true;
  }

  return fixed;
}

size_t ServerConfig::defaultClipboardSharingSize()
{
  return 3 * 1024; // 3 MiB
}

size_t ServerConfig::setClipboardSharingSize(size_t size)
{
  if (size) {
    size += 512; // Round up to the nearest megabyte
    size /= 1024;
    size *= 1024;
    setClipboardSharing(true);
  } else {
    setClipboardSharing(false);
  }
  using std::swap;
  swap(size, m_ClipboardSharingSize);
  return size;
}

QSettingsProxy &ServerConfig::settings()
{
  return Settings::proxy();
}
