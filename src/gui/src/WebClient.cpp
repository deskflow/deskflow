/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#include "WebClient.h"

#include "QUtility.h"

#include <QProcess>
#include <QMessageBox>
#include <QCoreApplication>
#include <stdexcept>

bool
WebClient::getEdition (int& edition, QString& errorOut) {
    QString responseJson = request ();

    /* TODO: This is horrible and should be ripped out as soon as we move
     *         to Qt 5. See issue #5630
     */

    QRegExp resultRegex (".*\"result\".*:.*(true|false).*");
    if (resultRegex.exactMatch (responseJson)) {
        QString boolString = resultRegex.cap (1);
        if (boolString == "true") {
            QRegExp editionRegex (".*\"edition\".*:.*\"([^\"]+)\".*");
            if (editionRegex.exactMatch (responseJson)) {
                QString e = editionRegex.cap (1);
                edition   = e.toInt ();
                return true;
            } else {
                throw std::runtime_error ("Unrecognised server response.");
            }
        } else {
            errorOut = tr ("Login failed. Invalid email address or password.");
            return false;
        }
    } else {
        QRegExp errorRegex (".*\"error\".*:.*\"([^\"]+)\".*");
        if (errorRegex.exactMatch (responseJson)) {
            errorOut = errorRegex.cap (1).replace ("\\n", "\n");
            return false;
        } else {
            throw std::runtime_error ("Unrecognised server response.");
        }
    }
}

bool
WebClient::setEmail (QString email, QString& errorOut) {
    if (email.isEmpty ()) {
        errorOut = tr ("Your email address cannot be left blank.");
        return false;
    }
    m_Email = email;
    return true;
}

bool
WebClient::setPassword (QString password, QString&) {
    m_Password = password;
    return true;
}

QString
WebClient::request () {
    QStringList args ("--login-auth");
    QString credentials (m_Email + ":" + hash (m_Password) + "\n");
    return m_CoreInterface.run (args, credentials);
}
