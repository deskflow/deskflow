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
    m_ProfileDir = DataDirectories::profile();
    if (m_ProfileDir.empty()) {
        emit error(tr("Failed to get profile directory."));
    }
}

std::pair<bool, std::string> SslCertificate::runTool(const QStringList& args)
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
    std::string output;

    QString standardError;
    if (success && process.waitForFinished())
    {
        output = process.readAllStandardOutput().trimmed().toStdString();
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
        return {false, output};
    }

    return {true, output};
}

void SslCertificate::generateCertificate()
{
    auto filename = QString::fromStdString(getCertificatePath());

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
        arguments.append("rsa:2048");

        QDir sslDir(QString::fromStdString(getCertificateDirectory()));
        if (!sslDir.exists()) {
            sslDir.mkpath(".");
        }

        // key output filename
        arguments.append("-keyout");
        arguments.append(filename);

        // certificate output filename
        arguments.append("-out");
        arguments.append(filename);

        if (!runTool(arguments).first) {
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

    auto ret = runTool(arguments);
    bool success = ret.first;
    std::string output = ret.second;

    if (!success) {
        return;
    }

    // find the fingerprint from the tool output
    auto i = output.find_first_of('=');
    if (i != std::string::npos) {
        i++;
        auto fingerprint = output.substr(
            i, output.size() - i);

        Fingerprint::local().trust(QString::fromStdString(fingerprint), false);
        emit info(tr("SSL fingerprint generated."));
    }
    else {
        emit error(tr("Failed to find SSL fingerprint."));
    }
}

std::string SslCertificate::getCertificatePath()
{
    return getCertificateDirectory() + QDir::separator().toLatin1() + kCertificateFilename;
}

std::string SslCertificate::getCertificateDirectory()
{
    return m_ProfileDir + QDir::separator().toLatin1() + kSslDir;
}
