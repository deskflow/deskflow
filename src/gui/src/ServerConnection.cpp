/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2021 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "ServerConnection.h"

#include "MainWindow.h"
#include "ServerConfigDialog.h"
#include "ServerMessage.h"

#include <QMessageBox>


ServerConnection::ServerConnection(MainWindow& parent) :
    m_parent(parent)
{

}

void ServerConnection::update(const QString& line)
{
    ServerMessage message(line);

    if (!m_parent.appConfig().getUseExternalConfig() &&
        m_parent.isActiveWindow() &&
        message.isNewClientMessage() &&
        !m_ignoredClients.contains(message.getClientName()))
    {
        addClient(message.getClientName());
    }
}

void ServerConnection::addClient(const QString& clientName)
{
    if (!m_parent.serverConfig().isFull())
    {
        QMessageBox message(&m_parent);
        message.addButton(QObject::tr("Ignore"), QMessageBox::RejectRole);
        message.addButton(QObject::tr("Accept and configure"), QMessageBox::AcceptRole);
        message.setText(QObject::tr("%1 client has made a connection request").arg(clientName));

        if (message.exec() == QMessageBox::Accepted)
        {
            configureClient(clientName);
        }
        else
        {
            m_ignoredClients.append(clientName);
        }
    }
}

void ServerConnection::configureClient(const QString& clientName)
{
    auto& config = m_parent.serverConfig();
    config.addToFirstEmptyGrid(clientName);

    ServerConfigDialog dlg(&m_parent, config);
    dlg.exec();

    m_parent.restartSynergy();
}
