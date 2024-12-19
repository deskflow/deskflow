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

#include "gui/proxy/QNetworkAccessManagerProxy.h"

#include <QObject>
#include <QString>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;
class VersionCheckerTests;

class VersionChecker : public QObject
{
  using QNetworkAccessManagerProxy = deskflow::gui::proxy::QNetworkAccessManagerProxy;

  Q_OBJECT

  friend class VersionCheckerTests;

public:
  explicit VersionChecker(std::shared_ptr<QNetworkAccessManagerProxy> network = nullptr);
  void checkLatest() const;
public slots:
  void replyFinished(QNetworkReply *reply);
signals:
  void updateFound(const QString &version);

private:
  static int compareVersions(const QString &left, const QString &right);

  /**
   * \brief Converts a string stage to a integer value
   * \param stage The string containing the stage version
   * \return An integer representation of the stage, the higher the number the
   * more recent the version
   */
  static int getStageVersion(QString stage);

  std::shared_ptr<QNetworkAccessManagerProxy> m_network;
};
