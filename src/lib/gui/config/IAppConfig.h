/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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

#include "ElevateMode.h"

#include <QString>

namespace synergy::gui {

enum class ProcessMode { kService, kDesktop };

class IAppConfig {
public:
  virtual ~IAppConfig() = default;
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
  virtual bool invertConnection() const = 0;
  virtual void persistLogDir() const = 0;
  virtual QString serialKey() const = 0;
  virtual bool languageSync() const = 0;
  virtual bool invertScrollDirection() const = 0;
  virtual int port() const = 0;
  virtual bool useExternalConfig() const = 0;
  virtual const QString &configFile() const = 0;
  virtual const QString &networkInterface() const = 0;
};

} // namespace synergy::gui
