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

#pragma once

#include <QString>

class Fingerprint {
private:
    Fingerprint (const QString& filename);

public:
    void trust (const QString& fingerprintText, bool append = true);
    bool isTrusted (const QString& fingerprintText);
    QStringList readList (const int readTo = -1);
    QString readFirst ();
    QString filePath () const;
    bool fileExists () const;

public:
    static Fingerprint local ();
    static Fingerprint trustedServers ();
    static Fingerprint trustedClients ();
    static QString directoryPath ();
    static QString localFingerprint ();
    static bool localFingerprintExists ();
    static void persistDirectory ();

private:
    QString m_Filename;
};
