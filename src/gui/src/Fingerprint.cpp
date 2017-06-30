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

#include "Fingerprint.h"

#include "CoreInterface.h"

#include <QDir>
#include <QTextStream>

static const char kDirName[]                = "SSL/Fingerprints";
static const char kLocalFilename[]          = "Local.txt";
static const char kTrustedServersFilename[] = "TrustedServers.txt";
static const char kTrustedClientsFilename[] = "TrustedClients.txt";

Fingerprint::Fingerprint (const QString& filename) {
    m_Filename = filename;
}

void
Fingerprint::trust (const QString& fingerprintText, bool append) {
    Fingerprint::persistDirectory ();

    QIODevice::OpenMode openMode;
    if (append) {
        openMode = QIODevice::Append;
    } else {
        openMode = QIODevice::WriteOnly;
    }

    QFile file (filePath ());
    if (file.open (openMode)) {
        QTextStream out (&file);
        out << fingerprintText << "\n";
        file.close ();
    }
}

bool
Fingerprint::fileExists () const {
    QString dirName = Fingerprint::directoryPath ();
    if (!QDir (dirName).exists ()) {
        return false;
    }

    QFile file (filePath ());
    return file.exists ();
}

bool
Fingerprint::isTrusted (const QString& fingerprintText) {
    QStringList list = readList ();
    foreach (QString trusted, list) {
        if (trusted == fingerprintText) {
            return true;
        }
    }
    return false;
}

QStringList
Fingerprint::readList (const int readTo) {
    QStringList list;

    QString dirName = Fingerprint::directoryPath ();
    if (!QDir (dirName).exists ()) {
        return list;
    }

    QFile file (filePath ());

    if (file.open (QIODevice::ReadOnly)) {
        QTextStream in (&file);
        while (!in.atEnd ()) {
            list.append (in.readLine ());
            if (list.size () == readTo) {
                break;
            }
        }
        file.close ();
    }

    return list;
}

QString
Fingerprint::readFirst () {
    QStringList list = readList (1);
    return list.at (0);
}

QString
Fingerprint::filePath () const {
    QString dir = Fingerprint::directoryPath ();
    return QString ("%1/%2").arg (dir).arg (m_Filename);
}

void
Fingerprint::persistDirectory () {
    QDir dir (Fingerprint::directoryPath ());
    if (!dir.exists ()) {
        dir.mkpath (".");
    }
}

QString
Fingerprint::directoryPath () {
    CoreInterface coreInterface;
    QString profileDir = coreInterface.getProfileDir ();

    return QString ("%1/%2").arg (profileDir).arg (kDirName);
}

Fingerprint
Fingerprint::local () {
    return Fingerprint (kLocalFilename);
}

Fingerprint
Fingerprint::trustedServers () {
    return Fingerprint (kTrustedServersFilename);
}

Fingerprint
Fingerprint::trustedClients () {
    return Fingerprint (kTrustedClientsFilename);
}
