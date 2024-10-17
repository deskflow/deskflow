/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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

#include "base/String.h"

namespace deskflow {

/**
 * @brief This is the base Argument class that will store the generic
 *        arguments passed into the applications this will be derived
 *        from and expanded to include application specific arguments
 */
class ArgsBase
{
public:
  ArgsBase() = default;
  virtual ~ArgsBase();

  /// @brief This sets the type of the derived class
  enum Type
  {
    kBase,
    kServer,
    kClient
  };

  /// @brief Stores what type of object this is
  Type m_classType = kBase;

  /// @brief Should run as a daemon
  bool m_daemon = true;

  /// @brief Should the app restart automatically
  bool m_restartable = true;

  /// @brief Should the app use hooks
  bool m_noHooks = false;

  /// @brief The filename of the running process
  const char *m_pname = nullptr;

  /// @brief The logging level of the application
  const char *m_logFilter = nullptr;

  /// @brief The full path to the logfile
  const char *m_logFile = nullptr;

  /// @brief Contains the X-Server display to use
  const char *m_display = nullptr;

  /// @brief The name of the current computer
  String m_name;

  /// @brief Should the app add a tray icon
  bool m_disableTray = false;

  /// @brief Tell the client to talk through IPC to the daemon
  bool m_enableIpc = false;

  /// @brief Should drag drop support be enabled
  bool m_enableDragDrop = false;

  /// @brief Will cause the application to exit with OK code when set to true
  bool m_shouldExitOk = false;

  /// @brief Will cause the application to exit with fail code when set to true
  bool m_shouldExitFail = false;

  /// @brief Bind to this address
  String m_deskflowAddress;

  /// @brief Should the connections be TLS encrypted
  bool m_enableCrypto = false;

  /// @brief The dir to load settings from
  String m_profileDirectory;

  /// @brief The dir to load plugins from
  String m_pluginDirectory;

  /// @brief Contains the location of the TLS certificate file
  String m_tlsCertFile;

  /// @brief Stop this computer from sleeping
  bool m_preventSleep = false;

#if SYSAPI_WIN32
  bool m_debugServiceWait = false;
  bool m_pauseOnExit = false;
  bool m_stopOnDeskSwitch = false;
#endif

#if WINAPI_XWINDOWS
  bool m_disableXInitThreads = false;
#endif

protected:
  /// @brief deletes pointers and sets the value to null
  template <class T> static inline void destroy(T *&p)
  {
    delete p;
    p = 0;
  }
};
} // namespace deskflow
