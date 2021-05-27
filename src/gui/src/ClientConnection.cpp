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

#include "ClientConnection.h"

#include "MainWindow.h"

#include <QMessageBox>
#include <QHostAddress>

ClientConnection::ClientConnection(MainWindow& parent) :
    m_parent(parent)
{
}

void ClientConnection::update(const QString& line)
{
    if (m_checkConnection && checkMainWindow())
    {
        if (line.contains("failed to connect to server"))
        {
            m_checkConnection = false;
            if (!line.contains("server refused client with our name") &&
                !line.contains("Trying next address"))
            {
                showMessage(getMessage(line));
            }
        }
        else if (line.contains("connected to server"))
        {
            m_checkConnection = false;
        }
    }
}

bool ClientConnection::checkMainWindow()
{
    bool result = m_parent.isActiveWindow();

    if (m_parent.isMinimized() || m_parent.isHidden())
    {
        m_parent.showNormal();
        m_parent.activateWindow();
        result = true;
    }

    return result;
}

QString ClientConnection::getMessage(const QString& line) const
{
    QString message(QObject::tr("Connection failed.\nCheck the IP address on the server, your TLS and firewall settings."));

    if (line.contains("server already has a connected client with our name"))
    {
        message = QObject::tr("Connection failed.\nYou can’t name 2 computers the same.");
    }
    else
    {
        QHostAddress address(m_parent.appConfig().getServerHostname());
        if (address.isNull())
        {
            message = QObject::tr("We can’t connect to the server \"%1\" try to connect using the server IP address and check your firewall settings.")
                               .arg(m_parent.appConfig().getServerHostname());
        }
    }

    return message;
}

void ClientConnection::showMessage(const QString& message) const
{
    QMessageBox dialog(&m_parent);
    dialog.addButton(QObject::tr("Close"), QMessageBox::RejectRole);
    dialog.setText(message);
    dialog.exec();
}

void ClientConnection::setCheckConnection(bool checkConnection)
{
    m_checkConnection = checkConnection;
}
