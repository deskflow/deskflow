/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "QNetworkAccessManagerProxy.h"

#include <QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

namespace deskflow::gui::proxy {

void QNetworkAccessManagerProxy::init()
{
  m_network = std::make_shared<QNetworkAccessManager>();

  connect(m_network.get(), &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
    Q_EMIT finished(reply);
  });
}

void QNetworkAccessManagerProxy::get(const QNetworkRequest &request) const
{
  m_network->get(request);
}

} // namespace deskflow::gui::proxy
