/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Constants.h"
#include "common/Hotkey.h"
#include "gui/config/ComputerConfig.h"
#include "gui/config/ComputerList.h"
#include "gui/config/DesktopLayout.h"
#include "gui/config/MonitorLayoutDraft.h"

#include <QList>

class QTextStream;
class QSettings;
class QString;
class QFile;
class ServerConfigDialog;

class ServerConfig : public ComputerConfig
{
  friend class ServerConfigDialog;
  friend QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);

public:
  explicit ServerConfig(int columns = kServerGridWidth, int rows = kServerGridHeight);
  ~ServerConfig() = default;

  bool operator==(const ServerConfig &sc) const;

  const ComputerList &computers() const
  {
    return m_computers;
  }

  //
  // New methods
  //
  const HotkeyList &hotkeys() const
  {
    return m_Hotkeys;
  }

  bool save(const QString &fileName) const;
  bool computerExists(const QString &computerName) const;
  void save(QFile &file) const;
  bool isFull() const;
  void commit();
  int numComputers() const;
  QString getServerName() const;
  void updateServerName();
  QString configFile() const;
  bool useExternalConfig() const;
  void addClient(const QString &clientName);
  void ensureLocalServer();
  bool hasMonitorLayout() const
  {
    return !m_monitorLayout.isEmpty();
  }
  deskflow::gui::MonitorLayoutDraft monitorLayoutDraft(bool reset = false) const;
  void setMonitorLayout(const deskflow::gui::MonitorLayoutDraft &draft)
  {
    m_monitorLayout = draft.layout.encode();
  }
  void clearMonitorLayout()
  {
    m_monitorLayout.clear();
  }
  QByteArray workspaceState() const
  {
    return m_workspaceState;
  }
  QByteArray monitorLayoutBytes() const
  {
    return m_monitorLayout;
  }
  bool setWorkspace(const QByteArray &state, const QByteArray &layout, bool apply);
  bool hasDesktopLayout() const
  {
    return !m_desktopLayout.isEmpty();
  }
  deskflow::gui::DesktopLayout desktopLayout() const;
  QString desktopLayoutError() const;
  bool setDesktopLayout(const std::vector<deskflow::experience::Desktop> &desktops);
  void restoreGridLayout()
  {
    m_desktopLayout.clear();
  }
  QString renameLayoutClient(const QString &oldName, const QString &newName);
  QString removeLayoutClient(const QString &name);

private:
  void recall();
  void setupComputers();
  QSettingsProxy &settings();
  ComputerList &computers()
  {
    return m_computers;
  }
  void setComputers(const ComputerList &computers)
  {
    m_computers = computers;
  }
  void addComputer(const Computer &computer)
  {
    m_computers.append(computer);
  }
  void setConfigFile(const QString &configFile) const;
  void setUseExternalConfig(bool useExternalConfig) const;
  HotkeyList &hotkeys()
  {
    return m_Hotkeys;
  }
  int adjacentComputerIndex(int idx, int deltaColumn, int deltaRow) const;
  bool findComputerName(const QString &name, int &index);
  bool fixNoServer(const QString &name, int &index);

private:
  HotkeyList m_Hotkeys;

  ComputerList m_computers;
  int m_columns;
  int m_rows;
  QByteArray m_monitorLayout;
  QByteArray m_workspaceState;
  QString m_desktopLayout; // Kept verbatim on recall, including unreadable future schemas.
};

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);
