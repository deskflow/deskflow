/*
 * barrier -- mouse and keyboard sharing utility
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

#include "SslCertificate.h"
#include "Fingerprint.h"
#include "common/DataDirectories.h"

#include <QProcess>
#include <QDir>
#include <QCoreApplication>

static const char kCertificateLifetime[] = "365";
static const char kCertificateSubjectInfo[] = "/CN=Barrier";
static const char kCertificateFilename[] = "Barrier.pem";
static const char kSslDir[] = "SSL";
static const char kUnixOpenSslCommand[] = "openssl";

#if defined(Q_OS_WIN)
static const char kWinOpenSslBinary[] = "openssl.exe";
static const char kConfigFile[] = "barrier.conf";
#endif

SslCertificate::SslCertificate(QObject *parent) :
    QObject(parent)
{
    m_ProfileDir = QString::fromStdString(DataDirectories::profile());
    if (m_ProfileDir.isEmpty()) {
        emit error(tr("Failed to get profile directory."));
    }
}

bool SslCertificate::runTool(const QStringList& args)
{
    QString program;
#if defined(Q_OS_WIN)
    program = QCoreApplication::applicationDirPath();
    program.append("\\").append(kWinOpenSslBinary);
#else
    program = kUnixOpenSslCommand;
#endif


    QStringList environment;
#if defined(Q_OS_WIN)
    environment << QString("OPENSSL_CONF=%1\\%2")
        .arg(QCoreApplication::applicationDirPath())
        .arg(kConfigFile);
#endif

    QProcess process;
    process.setEnvironment(environment);
    process.start(program, args);

    bool success = process.waitForStarted();

    QString standardError;
    if (success && process.waitForFinished())
    {
        m_ToolOutput = process.readAllStandardOutput().trimmed();
        standardError = process.readAllStandardError().trimmed();
    }

    int code = process.exitCode();
    if (!success || code != 0)
    {
        emit error(
            QString("SSL tool failed: %1\n\nCode: %2\nError: %3")
                .arg(program)
                .arg(process.exitCode())
                .arg(standardError.isEmpty() ? "Unknown" : standardError));
        return false;
    }

    return true;
}

void SslCertificate::generateCertificate()
{
    QString sslDirPath = QString("%1%2%3")
        .arg(m_ProfileDir)
        .arg(QDir::separator())
        .arg(kSslDir);

    QString filename = QString("%1%2%3")
        .arg(sslDirPath)
        .arg(QDir::separator())
        .arg(kCertificateFilename);

    QFile file(filename);
    if (!file.exists()) {
        QStringList arguments;

        // self signed certificate
        arguments.append("req");
        arguments.append("-x509");
        arguments.append("-nodes");

        // valide duration
        arguments.append("-days");
        arguments.append(kCertificateLifetime);

        // subject information
        arguments.append("-subj");

        QString subInfo(kCertificateSubjectInfo);
        arguments.append(subInfo);

        // private key
        arguments.append("-newkey");
        arguments.append("rsa:1024");

        QDir sslDir(sslDirPath);
        if (!sslDir.exists()) {
            sslDir.mkpath(".");
        }

        // key output filename
        arguments.append("-keyout");
        arguments.append(filename);

        // certificate output filename
        arguments.append("-out");
        arguments.append(filename);

        if (!runTool(arguments)) {
            return;
        }

        emit info(tr("SSL certificate generated."));
    }

    generateFingerprint(filename);

    emit generateFinished();
}

void SslCertificate::generateFingerprint(const QString& certificateFilename)
{
    QStringList arguments;
    arguments.append("x509");
    arguments.append("-fingerprint");
    arguments.append("-sha1");
    arguments.append("-noout");
    arguments.append("-in");
    arguments.append(certificateFilename);

    if (!runTool(arguments)) {
        return;
    }

    // find the fingerprint from the tool output
    int i = m_ToolOutput.indexOf("=");
    if (i != -1) {
        i++;
        QString fingerprint = m_ToolOutput.mid(
            i, m_ToolOutput.size() - i);

        Fingerprint::local().trust(fingerprint, false);
        emit info(tr("SSL fingerprint generated."));
    }
    else {
        emit error(tr("Failed to find SSL fingerprint."));
    }
}
