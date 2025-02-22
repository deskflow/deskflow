/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ElevateMode.h"

#include "gui/config/IConfigScopes.h"

#include <QString>

namespace deskflow::gui {

enum class ProcessMode
{
  kService,
  kDesktop
};

class IAppConfig
{
  using IConfigScopes = deskflow::gui::IConfigScopes;

public:
  virtual ~IAppConfig() = default;

  //
  // Getters
  //

  virtual IConfigScopes &scopes() const = 0;
  virtual QString tlsCertPath() const = 0;
  virtual int tlsKeyLength() const = 0;
  virtual bool tlsEnabled() const = 0;
  virtual ProcessMode processMode() const = 0;
  virtual ElevateMode elevateMode() const = 0;
  virtual QString logLevelText() const = 0;
  virtual const QString &screenName() const = 0;
  virtual bool preventSleep() const = 0;
  virtual bool logToFile() const = 0;
  virtual const QString &logFilename() const = 0;
  virtual QString coreServerName() const = 0;
  virtual QString coreClientName() const = 0;
  virtual void persistLogDir() const = 0;
  virtual bool languageSync() const = 0;
  virtual bool invertScrollDirection() const = 0;
  virtual int port() const = 0;
  virtual bool useExternalConfig() const = 0;
  virtual const QString &configFile() const = 0;
  virtual const QString &networkInterface() const = 0;
  virtual const QString &serverHostname() const = 0;
  virtual int logLevel() const = 0;
  virtual bool autoHide() const = 0;
  virtual bool enableService() const = 0;
  virtual bool closeToTray() const = 0;
  virtual bool isActiveScopeSystem() const = 0;
  virtual bool isActiveScopeWritable() const = 0;
  virtual bool clientGroupChecked() const = 0;
  virtual bool requireClientCerts() const = 0;

  //
  // Setters
  //

  virtual void setLoadFromSystemScope(bool loadFromSystemScope) = 0;
  virtual void setScreenName(const QString &screenName) = 0;
  virtual void setPort(int port) = 0;
  virtual void setNetworkInterface(const QString &networkInterface) = 0;
  virtual void setLogLevel(int logLevel) = 0;
  virtual void setLogToFile(bool logToFile) = 0;
  virtual void setLogFilename(const QString &logFilename) = 0;
  virtual void setElevateMode(ElevateMode elevateMode) = 0;
  virtual void setAutoHide(bool autoHide) = 0;
  virtual void setPreventSleep(bool preventSleep) = 0;
  virtual void setTlsCertPath(const QString &tlsCertPath) = 0;
  virtual void setTlsKeyLength(int tlsKeyLength) = 0;
  virtual void setTlsEnabled(bool tlsEnabled) = 0;
  virtual void setLanguageSync(bool languageSync) = 0;
  virtual void setInvertScrollDirection(bool invertScrollDirection) = 0;
  virtual void setEnableService(bool enableService) = 0;
  virtual void setCloseToTray(bool closeToTray) = 0;
  virtual void setRequireClientCerts(bool requireClientCerts) = 0;
};

} // namespace deskflow::gui
