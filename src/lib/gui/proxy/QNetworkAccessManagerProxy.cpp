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
