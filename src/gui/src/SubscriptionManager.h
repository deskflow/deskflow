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

#pragma once

#include <QObject>
#include <SerialKey.h>
#include <ActivationNotifier.h>

class AppConfig;

class SubscriptionManager: public QObject
{
    Q_OBJECT

public:
    SubscriptionManager(AppConfig* appConfig);
    SerialKey setSerialKey(QString serialKey);
    void update() const;
    Edition activeLicense() const;
    void skipActivation();

private:
    void notifyActivation(QString identity);

private:
    AppConfig* m_AppConfig;
    SerialKey m_serialKey;

signals:
    void serialKeyChanged (SerialKey) const;
    void editionChanged (Edition) const;
    void beginTrial (bool expiring) const;
    void endTrial (bool expired) const;
};
