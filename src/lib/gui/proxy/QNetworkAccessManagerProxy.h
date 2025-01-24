/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QNetworkAccessManager>
#include <QObject>

namespace deskflow::gui::proxy {

class QNetworkAccessManagerProxy : public QObject
{
  Q_OBJECT

public:
  virtual void init();
  virtual void get(const QNetworkRequest &request) const;

signals:
  void finished(QNetworkReply *reply);

private:
  std::shared_ptr<QNetworkAccessManager> m_network;
};

} // namespace deskflow::gui::proxy
