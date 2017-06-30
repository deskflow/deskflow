/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Seamless Inc.
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

#ifndef ACTIVATIONNOTIFIER_H
#define ACTIVATIONNOTIFIER_H

#include <QObject>

class ActivationNotifier : public QObject {
    Q_OBJECT
public:
    explicit ActivationNotifier (QObject* parent = 0);

    void setIdentity (QString identity);
    void setUpdateInfo (QString const& fromVersion, QString const& toVersion,
                        QString const& serialKey);

public slots:
    void notify ();
    void notifyUpdate ();

signals:
    void finished ();

private:
    QString m_Identity;
    QString m_fromVersion;
    QString m_toVersion;
    QString m_serialKey;
};

#endif // ACTIVATIONNOTIFIER_H
