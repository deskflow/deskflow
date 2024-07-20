/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QList>

#include "CommonConfig.h"
#include "Hotkey.h"
#include "ScreenConfig.h"
#include "ScreenList.h"

class QTextStream;
class QSettings;
class QString;
class QFile;
class ServerConfigDialog;
class MainWindow;
class AppConfig;

class ServerConfig : public ScreenConfig, public synergy::gui::CommonConfig {
  friend class ServerConfigDialog;
  friend class ServerConnection;
  friend QTextStream &
  operator<<(QTextStream &outStream, const ServerConfig &config);

public:
  ServerConfig(
      int numColumns, int numRows, AppConfig *appConfig,
      MainWindow *mainWindow);

  ServerConfig(const ServerConfig &src) = default;
  ServerConfig(ServerConfig &&) = default;
  ~ServerConfig();
  ServerConfig &operator=(const ServerConfig &src) = default;
  ServerConfig &operator=(ServerConfig &&) = delete;

  bool operator==(const ServerConfig &sc) const;

  const ScreenList &screens() const { return m_Screens; }
  int numColumns() const { return m_NumColumns; }
  int numRows() const { return m_NumRows; }
  bool hasHeartbeat() const { return m_HasHeartbeat; }
  int heartbeat() const { return m_Heartbeat; }
  bool relativeMouseMoves() const { return m_RelativeMouseMoves; }
  bool win32KeepForeground() const { return m_Win32KeepForeground; }
  bool hasSwitchDelay() const { return m_HasSwitchDelay; }
  int switchDelay() const { return m_SwitchDelay; }
  bool hasSwitchDoubleTap() const { return m_HasSwitchDoubleTap; }
  int switchDoubleTap() const { return m_SwitchDoubleTap; }
  bool switchCorner(int c) const { return m_SwitchCorners[c]; }
  int switchCornerSize() const { return m_SwitchCornerSize; }
  const QList<bool> &switchCorners() const { return m_SwitchCorners; }
  const HotkeyList &hotkeys() const { return m_Hotkeys; }
  bool enableDragAndDrop() const { return m_EnableDragAndDrop; }
  bool disableLockToScreen() const { return m_DisableLockToScreen; }
  bool clipboardSharing() const { return m_ClipboardSharing; }
  size_t clipboardSharingSize() const { return m_ClipboardSharingSize; }
  static size_t defaultClipboardSharingSize();

  void saveSettings() override;
  void loadSettings() override;
  bool save(const QString &fileName) const;
  void save(QFile &file) const;
  int numScreens() const;
  int autoAddScreen(const QString name);
  const QString &getServerName() const;
  void updateServerName();
  const QString &getConfigFile() const;
  bool getUseExternalConfig() const;
  bool isFull() const;
  bool isScreenExists(const QString &screenName) const;
  void addClient(const QString &clientName);
  bool isHotkeysAvailable() const;
  QString getClientAddress() const;
  void setClientAddress(const QString &address);

protected:
  QSettings &settings();
  ScreenList &screens() { return m_Screens; }
  void setScreens(const ScreenList &screens) { m_Screens = screens; }
  void addScreen(const Screen &screen) { m_Screens.append(screen); }
  void setNumColumns(int n) { m_NumColumns = n; }
  void setNumRows(int n) { m_NumRows = n; }
  void haveHeartbeat(bool on) { m_HasHeartbeat = on; }
  void setHeartbeat(int val) { m_Heartbeat = val; }
  void setRelativeMouseMoves(bool on) { m_RelativeMouseMoves = on; }
  void setWin32KeepForeground(bool on) { m_Win32KeepForeground = on; }
  void haveSwitchDelay(bool on) { m_HasSwitchDelay = on; }
  void setSwitchDelay(int val) { m_SwitchDelay = val; }
  void haveSwitchDoubleTap(bool on) { m_HasSwitchDoubleTap = on; }
  void setSwitchDoubleTap(int val) { m_SwitchDoubleTap = val; }
  void setSwitchCorner(int c, bool on) { m_SwitchCorners[c] = on; }
  void setSwitchCornerSize(int val) { m_SwitchCornerSize = val; }
  void setEnableDragAndDrop(bool on) { m_EnableDragAndDrop = on; }
  void setDisableLockToScreen(bool on) { m_DisableLockToScreen = on; }
  void setClipboardSharing(bool on) { m_ClipboardSharing = on; }
  void setConfigFile(const QString &configFile);
  void setUseExternalConfig(bool useExternalConfig);
  size_t setClipboardSharingSize(size_t size);
  QList<bool> &switchCorners() { return m_SwitchCorners; }
  HotkeyList &hotkeys() { return m_Hotkeys; }

  void init();
  int adjacentScreenIndex(int idx, int deltaColumn, int deltaRow) const;

private:
  bool findScreenName(const QString &name, int &index);
  bool fixNoServer(const QString &name, int &index);
  int showAddClientDialog(const QString &clientName);
  void addToFirstEmptyGrid(const QString &clientName);

private:
  bool m_HasHeartbeat = false;
  int m_Heartbeat = 0;
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
  QString m_clientAddress = "";
  QList<bool> m_SwitchCorners;
  HotkeyList m_Hotkeys;

  AppConfig *m_pAppConfig;
  MainWindow *m_pMainWindow;
  ScreenList m_Screens;
  int m_NumColumns;
  int m_NumRows;
  size_t m_ClipboardSharingSize;
};

QTextStream &operator<<(QTextStream &outStream, const ServerConfig &config);

enum {
  kAutoAddScreenOk,
  kAutoAddScreenManualServer,
  kAutoAddScreenManualClient,
  kAutoAddScreenIgnore
};
