/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Constants.h"
#include "gui/Hotkey.h"
#include "gui/config/ScreenConfig.h"
#include "gui/config/ScreenList.h"

#include <QList>

class QTextStream;
class QSettings;
class QString;
class QFile;
class ServerConfigDialog;

class ServerConfig : public ScreenConfig
{
  friend class ServerConfigDialog;
  friend QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);

public:
  explicit ServerConfig(int columns = kServerGridWidth, int rows = kServerGridHeight);
  ~ServerConfig() = default;

  bool operator==(const ServerConfig &sc) const;

  const ScreenList &screens() const
  {
    return m_Screens;
  }

  //
  // New methods
  //
  bool switchCorner(int c) const
  {
    return m_SwitchCorners[c];
  }
  int switchCornerSize() const
  {
    return m_SwitchCornerSize;
  }
  const QList<bool> &switchCorners() const
  {
    return m_SwitchCorners;
  }
  const HotkeyList &hotkeys() const
  {
    return m_Hotkeys;
  }

  bool save(const QString &fileName) const;
  bool screenExists(const QString &screenName) const;
  void save(QFile &file) const;
  bool isFull() const;
  void commit();
  int numScreens() const;
  QString getServerName() const;
  void updateServerName();
  QString configFile() const;
  bool useExternalConfig() const;
  void addClient(const QString &clientName);

private:
  void recall();
  void setupScreens();
  QSettingsProxy &settings();
  ScreenList &screens()
  {
    return m_Screens;
  }
  void setScreens(const ScreenList &screens)
  {
    m_Screens = screens;
  }
  void addScreen(const Screen &screen)
  {
    m_Screens.append(screen);
  }
  void setSwitchCorner(int c, bool on)
  {
    m_SwitchCorners[c] = on;
  }
  void setSwitchCornerSize(int val)
  {
    m_SwitchCornerSize = val;
  }
  void setConfigFile(const QString &configFile) const;
  void setUseExternalConfig(bool useExternalConfig) const;
  QList<bool> &switchCorners()
  {
    return m_SwitchCorners;
  }
  HotkeyList &hotkeys()
  {
    return m_Hotkeys;
  }
  int adjacentScreenIndex(int idx, int deltaColumn, int deltaRow) const;
  bool findScreenName(const QString &name, int &index);
  bool fixNoServer(const QString &name, int &index);

private:
  int m_SwitchCornerSize = 0;
  QList<bool> m_SwitchCorners;
  HotkeyList m_Hotkeys;

  ScreenList m_Screens;
  int m_columns;
  int m_rows;
};

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);
