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

#ifndef DATADOWNLOADER_H
#define DATADOWNLOADER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class DataDownloader : public QObject {
    Q_OBJECT

public:
    explicit DataDownloader (QObject* parent = 0);
    virtual ~DataDownloader ();

    QByteArray data () const;
    void cancel ();
    void download (QUrl url);
    bool
    isFinished () const {
        return m_IsFinished;
    }

signals:
    void isComplete ();

private slots:
    void complete (QNetworkReply* reply);

private:
    QNetworkAccessManager m_NetworkManager;
    QByteArray m_Data;
    QNetworkReply* m_pReply;
    bool m_IsFinished;
};

#endif // DATADOWNLOADER_H
