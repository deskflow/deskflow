/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
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

#include "PremiumAuth.h"
#include "QUtility.h"

#include <QProcess>
#include <QCoreApplication>

// we use syntool to authenticate because Qt's http library is very
// unreliable, and since we're writing platform specific code, use the
// synergy code since there we can use integ tests.
QString PremiumAuth::request(const QString& email, const QString& password)
{
	QString program(QCoreApplication::applicationDirPath() + "/syntool");
	QStringList args("--premium-auth");

	QProcess process;
	process.setReadChannel(QProcess::StandardOutput);
	process.start(program, args);

	if (process.waitForStarted())
	{
		// hash password in case it contains interesting chars.
		QString credentials(email + ":" + hash(password) + "\n");
		process.write(credentials.toStdString().c_str());

		if (process.waitForFinished()) {
			return process.readAll();
		}
	}

	return "";
}
