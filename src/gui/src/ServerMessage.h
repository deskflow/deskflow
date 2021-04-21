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

#ifndef SERVERMESSAGE_H
#define SERVERMESSAGE_H

#include <QString>

class ServerMessage
{
    QString m_message;
    QString m_clienName;

public:
    explicit ServerMessage(const QString& message);

    bool isNewClientMessage() const;
    bool isExitMessage() const;
    bool isConnectedMessage() const;
    bool isDisconnectedMessage() const;

    const QString& getClientName() const;

private:
    QString parseClientName(const QString& line) const;

};

#endif // SERVERMESSAGE_H
