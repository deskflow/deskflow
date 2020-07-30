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

#include "CoreInterface.h"

#include <QObject>
#include <base/String.h>

class SslCertificate : public QObject
{
Q_OBJECT

public:
    explicit SslCertificate(QObject *parent = 0);

public slots:
    /// @brief Generates a TLS cert and private key
    /// @param [in] QString path The path of the file to be generated
    /// @param [in] QString keyLength The size of the private key. default: 2048
    /// @param [in] bool Should the file be created regardless of if the file already exists
    void generateCertificate(const QString& path = QString(), const QString& keyLength = "2048", bool forceGen = false);

    /// @brief Get the key length of a TLS private key
    /// @param [in] QString path The path of the file to checked
    /// @return QString The key legnth as a string
    QString getCertKeyLength(const QString& path);

signals:
    void error(QString e);
    void info(QString i);
    void generateFinished();

private:
    bool runTool(const QStringList& args);
    void generateFingerprint(const QString& certificateFilename);

private:
    QString m_ProfileDir;
    QString m_ToolOutput;
    CoreInterface m_CoreInterface;
};
