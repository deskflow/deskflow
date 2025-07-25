/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

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
  virtual ~ArgsBase() = default;

  /// @brief This sets the type of the derived class
  enum class ClassType
  {
    Base,
    Server,
    Client
  };

  /// @brief Stores what type of object this is
  ClassType m_classType = ClassType::Base;

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
  std::string m_name;

  /// @brief Should the app add a tray icon
  bool m_disableTray = false;

  /// @brief Will cause the application to exit with OK code when set to true
  bool m_shouldExitOk = false;

  /// @brief Will cause the application to exit with fail code when set to true
  bool m_shouldExitFail = false;

  /// @brief Bind to this address
  std::string m_deskflowAddress;

  /// @brief Should the connections be TLS encrypted
  bool m_enableCrypto = false;

  /// @brief Contains the location of the TLS certificate file
  std::string m_tlsCertFile;

  /// @brief Stop this computer from sleeping
  bool m_preventSleep = false;

protected:
  /// @brief deletes pointers and sets the value to null
  template <class T> static inline void destroy(T *&p)
  {
    delete p;
    p = 0;
  }
};
} // namespace deskflow
