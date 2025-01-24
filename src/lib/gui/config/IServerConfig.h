/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QFile>
#include <QString>

#include "ScreenList.h"

namespace deskflow::gui {

class IServerConfig
{
public:
  virtual ~IServerConfig() = default;
  virtual bool isFull() const = 0;
  virtual bool screenExists(const QString &screenName) const = 0;
  virtual bool save(const QString &fileName) const = 0;
  virtual void save(QFile &file) const = 0;
  virtual bool enableDragAndDrop() const = 0;
  virtual const ScreenList &screens() const = 0;
};

} // namespace deskflow::gui
