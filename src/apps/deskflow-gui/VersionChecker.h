/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
