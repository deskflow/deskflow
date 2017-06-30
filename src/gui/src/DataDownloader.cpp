/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#include "DataDownloader.h"

DataDownloader::DataDownloader (QObject* parent)
    : QObject (parent), m_pReply (nullptr), m_IsFinished (false) {
    connect (&m_NetworkManager,
             SIGNAL (finished (QNetworkReply*)),
             SLOT (complete (QNetworkReply*)));
}

DataDownloader::~DataDownloader () {
}

void
DataDownloader::complete (QNetworkReply* reply) {
    m_Data = reply->readAll ();
    reply->deleteLater ();

    if (!m_Data.isEmpty ()) {
        m_IsFinished = true;
        emit isComplete ();
    }
}

QByteArray
DataDownloader::data () const {
    return m_Data;
}

void
DataDownloader::cancel () {
    m_pReply->abort ();
}

void
DataDownloader::download (QUrl url) {
    QNetworkRequest request (url);
    m_pReply = m_NetworkManager.get (request);
}
