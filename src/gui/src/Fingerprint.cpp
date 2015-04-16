/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

static const char kDirName[] = "ssl/fingerprints";
static const char kLocalFilename[] = "local.txt";
static const char kTrustedServersFilename[] = "trusted-servers.txt";
static const char kTrustedClientsFilename[] = "trusted-clients.txt";

Fingerprint::Fingerprint(const QString& filename)
{
	m_Filename = filename;
}

void Fingerprint::trust(const QString& fingerprintText)
{
	CoreInterface coreInterface;
	QString profileDir = coreInterface.getProfileDir();

	QString dirName = QString("%1/%2")
	  .arg(profileDir)
	  .arg(kDirName);

	QDir dir(dirName);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	QString path = QString("%1/%2").arg(dirName).arg(m_Filename);
	QFile file(path);
	if (file.open(QIODevice::Append))
	{
		QTextStream out(&file);
		out << fingerprintText << "\n";
		file.close();
	}
}

bool Fingerprint::exists(const QString& fingerprintText)
{
	CoreInterface coreInterface;
	QString profileDir = coreInterface.getProfileDir();

	QString dirName = QString("%1/%2")
	  .arg(profileDir)
	  .arg(kDirName);

	if (!QDir(dirName).exists()) {
		return false;
	}

	QString path = QString("%1/%2").arg(dirName).arg(m_Filename);
	QFile file(path);

	if (file.open(QIODevice::ReadOnly))
	{
	   QTextStream in(&file);
	   while (!in.atEnd())
	   {
		  QString trusted = in.readLine();
		  if (fingerprintText == trusted) {
			  return true;
		  }
	   }
	   file.close();
	}

	return false;
}

bool Fingerprint::localFingerprintExists()
{
	CoreInterface coreInterface;
	QString profileDir = coreInterface.getProfileDir();

	QString dirName = QString("%1/%2")
	  .arg(profileDir)
	  .arg(kDirName);

	QString path = QString("%1/%2").arg(dirName).arg(kLocalFilename);
	QFile file(path);

	bool exist = false;
	if (file.exists()) {
		exist = true;
	}

	return exist;
}

Fingerprint Fingerprint::local()
{
	return Fingerprint(kLocalFilename);
}

Fingerprint Fingerprint::trustedServers()
{
	return Fingerprint(kTrustedServersFilename);
}

Fingerprint Fingerprint::trustedClients()
{
	return Fingerprint(kTrustedClientsFilename);
}

QString Fingerprint::localFingerprint()
{
	CoreInterface coreInterface;
	QString profileDir = coreInterface.getProfileDir();

	QString dirName = QString("%1/%2")
	  .arg(profileDir)
	  .arg(kDirName);

	QString path = QString("%1/%2").arg(dirName).arg(kLocalFilename);

	QFile file(path);
	QString fingerprint;
	if (file.open(QIODevice::ReadOnly))
	{
	   QTextStream in(&file);
	   while (!in.atEnd())
	   {
		  QString context = in.readLine();
		  if (!context.isEmpty()) {
			  fingerprint = context;
		  }
	   }
	   file.close();
	}

	return fingerprint;
}
