/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "Hotkey.h"
#include "gui/config/IServerConfig.h"
#include "gui/config/ScreenConfig.h"
#include "gui/config/ScreenList.h"

#include <QList>

const auto kDefaultColumns = 5;
const auto kDefaultRows = 3;

class QTextStream;
class QSettings;
class QString;
class QFile;
class ServerConfigDialog;
class MainWindow;
class AppConfig;

namespace synergy::gui {

enum class ServerProtocol
{
  kSynergy,
  kBarrier
};

// The default protocol was decided by a community vote.
const auto kDefaultProtocol = ServerProtocol::kBarrier;

} // namespace synergy::gui

class ServerConfig : public ScreenConfig, public deskflow::gui::IServerConfig
{
  using QSettingsProxy = deskflow::gui::proxy::QSettingsProxy;
  using ServerProtocol = synergy::gui::ServerProtocol;

  friend class ServerConfigDialog;
  friend QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);

public:
  ServerConfig(AppConfig &appConfig, MainWindow &mainWindow, int columns = kDefaultColumns, int rows = kDefaultRows);
  ~ServerConfig() override = default;

  bool operator==(const ServerConfig &sc) const;

  //
  // Overrides
  //
  const ScreenList &screens() const override
  {
    return m_Screens;
  }
  bool enableDragAndDrop() const override
  {
    return m_EnableDragAndDrop;
  }

  //
  // New methods
  //
  int numColumns() const
  {
    return m_Columns;
  }
  int numRows() const
  {
    return m_Rows;
  }
  bool hasHeartbeat() const
  {
    return m_HasHeartbeat;
  }
  int heartbeat() const
  {
    return m_Heartbeat;
  }
  ServerProtocol protocol() const
  {
    return m_Protocol;
  }
  bool relativeMouseMoves() const
  {
    return m_RelativeMouseMoves;
  }
  bool win32KeepForeground() const
  {
    return m_Win32KeepForeground;
  }
  bool hasSwitchDelay() const
  {
    return m_HasSwitchDelay;
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

  //
  // Overrides
  //
  bool save(const QString &fileName) const override;
  bool screenExists(const QString &screenName) const override;
  void save(QFile &file) const override;
  bool isFull() const override;

  //
  // New methods
  //
  void commit();
  int numScreens() const;
  int autoAddScreen(const QString name);
  const QString &getServerName() const;
  void updateServerName();
  const QString &configFile() const;
  bool useExternalConfig() const;
  void addClient(const QString &clientName);
  QString getClientAddress() const;
  void setClientAddress(const QString &address);

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
  void setNumColumns(int n)
  {
    m_Columns = n;
  }
  void setNumRows(int n)
  {
    m_Rows = n;
  }
  void haveHeartbeat(bool on)
  {
    m_HasHeartbeat = on;
  }
  void setHeartbeat(int val)
  {
    m_Heartbeat = val;
  }
  void setProtocol(ServerProtocol val)
  {
    m_Protocol = val;
  }
  void setRelativeMouseMoves(bool on)
  {
    m_RelativeMouseMoves = on;
  }
  void setWin32KeepForeground(bool on)
  {
    m_Win32KeepForeground = on;
  }
  void haveSwitchDelay(bool on)
  {
    m_HasSwitchDelay = on;
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
  void setEnableDragAndDrop(bool on)
  {
    m_EnableDragAndDrop = on;
  }
  void setDisableLockToScreen(bool on)
  {
    m_DisableLockToScreen = on;
  }
  void setClipboardSharing(bool on)
  {
    m_ClipboardSharing = on;
  }
  void setConfigFile(const QString &configFile);
  void setUseExternalConfig(bool useExternalConfig);
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
  int showAddClientDialog(const QString &clientName);
  void addToFirstEmptyGrid(const QString &clientName);

private:
  bool m_HasHeartbeat = false;
  int m_Heartbeat = 0;
  ServerProtocol m_Protocol = synergy::gui::kDefaultProtocol;
  bool m_RelativeMouseMoves = false;
  bool m_Win32KeepForeground = false;
  bool m_HasSwitchDelay = false;
  int m_SwitchDelay = 0;
  bool m_HasSwitchDoubleTap = false;
  int m_SwitchDoubleTap = 0;
  int m_SwitchCornerSize = 0;
  bool m_EnableDragAndDrop = false;
  bool m_DisableLockToScreen = false;
  bool m_ClipboardSharing = true;
  QString m_ClientAddress = "";
  QList<bool> m_SwitchCorners;
  HotkeyList m_Hotkeys;

  AppConfig *m_pAppConfig;
  MainWindow *m_pMainWindow;
  ScreenList m_Screens;
  int m_Columns;
  int m_Rows;
  size_t m_ClipboardSharingSize;
};

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);

enum
{
  kAutoAddScreenOk,
  kAutoAddScreenManualServer,
  kAutoAddScreenManualClient,
  kAutoAddScreenIgnore
};
