/*
 * Deskflow -- mouse and keyboard sharing utility
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
