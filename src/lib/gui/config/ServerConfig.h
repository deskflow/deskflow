/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Hotkey.h"
#include "gui/config/ComputerConfig.h"
#include "gui/config/ComputerList.h"

#include <QList>

class QTextStream;
class QSettings;
class QString;
class QFile;
class QSize;
class ServerConfigDialog;

class ServerConfig : public ComputerConfig
{
  friend class ServerConfigDialog;
  friend QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);

public:
  ServerConfig();
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
  void commit();
  int numComputers() const;
  QString getServerName() const;
  void updateServerName();
  QString configFile() const;
  bool useExternalConfig() const;
  void addClient(const QString &clientName);

  /**
   * @brief ensureServer marks the server computer, adding it to the layout if missing
   */
  void ensureServer();

  /**
   * @brief resizeComputer sets the size of a computer, e.g. when its resolution is reported
   * @return true if the layout changed
   */
  bool resizeComputer(const QString &name, const QSize &size);

private:
  void recall();
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
  bool findComputerName(const QString &name, int &index);

private:
  HotkeyList m_Hotkeys;

  ComputerList m_computers;
};

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);
