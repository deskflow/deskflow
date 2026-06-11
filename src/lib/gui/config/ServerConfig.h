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
  int heartbeat() const
  {
    return m_Heartbeat;
  }
  bool relativeMouseMoves() const
  {
    return m_RelativeMouseMoves;
  }
  bool win32KeepForeground() const
  {
    return m_Win32KeepForeground;
  }
  int switchDelay() const
  {
    return m_SwitchDelay;
  }
  bool hasSwitchDoubleTap() const
  {
    return m_HasSwitchDoubleTap;
  }
  int switchDoubleTap() const
  {
    return m_SwitchDoubleTap;
  }
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
  bool defaultLockToScreenState() const
  {
    return m_DefaultLockToScreenState;
  }
  bool disableLockToScreen() const
  {
    return m_DisableLockToScreen;
  }
  bool clipboardSharing() const
  {
    return m_ClipboardSharing;
  }
  size_t clipboardSharingSize() const
  {
    return m_ClipboardSharingSize;
  }
  static size_t defaultClipboardSharingSize();

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
  void setHeartbeat(int val)
  {
    m_Heartbeat = val;
  }
  void setRelativeMouseMoves(bool on)
  {
    m_RelativeMouseMoves = on;
  }
  void setWin32KeepForeground(bool on)
  {
    m_Win32KeepForeground = on;
  }
  void setSwitchDelay(int val)
  {
    m_SwitchDelay = val;
  }
  void haveSwitchDoubleTap(bool on)
  {
    m_HasSwitchDoubleTap = on;
  }
  void setSwitchDoubleTap(int val)
  {
    m_SwitchDoubleTap = val;
  }
  void setSwitchCorner(int c, bool on)
  {
    m_SwitchCorners[c] = on;
  }
  void setSwitchCornerSize(int val)
  {
    m_SwitchCornerSize = val;
  }
  void setDefaultLockToScreenState(bool on)
  {
    m_DefaultLockToScreenState = on;
  }
  void setDisableLockToScreen(bool on)
  {
    m_DisableLockToScreen = on;
  }
  void setClipboardSharing(bool on)
  {
    m_ClipboardSharing = on;
  }
  void setConfigFile(const QString &configFile) const;
  void setUseExternalConfig(bool useExternalConfig) const;
  size_t setClipboardSharingSize(size_t size);
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
  int m_Heartbeat = 0;
  bool m_RelativeMouseMoves = false;
  bool m_Win32KeepForeground = false;
  int m_SwitchDelay = 0;
  bool m_HasSwitchDoubleTap = false;
  int m_SwitchDoubleTap = 0;
  int m_SwitchCornerSize = 0;
  bool m_DefaultLockToScreenState = false;
  bool m_DisableLockToScreen = false;
  bool m_ClipboardSharing = true;
  QString m_ClientAddress = "";
  QList<bool> m_SwitchCorners;
  HotkeyList m_Hotkeys;

  ScreenList m_Screens;
  int m_columns;
  int m_rows;
  size_t m_ClipboardSharingSize = defaultClipboardSharingSize();
};

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);
