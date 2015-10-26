/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si, Std.
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

#include "WebClient.h"

#include "EditionType.h"
#include "QUtility.h"

#include <QProcess>
#include <QMessageBox>
#include <QCoreApplication>
#include <stdexcept>

int WebClient::getEdition(
		const QString& email,
		const QString& password,
		QMessageBox& message,
		QWidget* w)
{
	QString responseJson;
	int edition = Unknown;
	try {
		responseJson = request(email, password);
	}
	catch (std::exception& e)
	{
		message.critical(
			w, "Error",
			tr("An error occurred while trying to sign in. "
			"Please contact the helpdesk, and provide the "
			"following details.\n\n%1").arg(e.what()));
		return edition;
	}

	QRegExp resultRegex(".*\"result\".*:.*(true|false).*");
	if (resultRegex.exactMatch(responseJson)) {
		QString boolString = resultRegex.cap(1);
		if (boolString == "true") {
			QRegExp editionRegex(".*\"edition\".*:.*\"([^\"]+)\".*");
			if (editionRegex.exactMatch(responseJson)) {
				QString e = editionRegex.cap(1);
				edition = e.toInt();
			}

			return edition;
		}
		else if (boolString == "false") {
			message.critical(
				w, "Error",
				tr("Login failed, invalid email or password."));

			return edition;
		}
	}
	else {
		QRegExp errorRegex(".*\"error\".*:.*\"([^\"]+)\".*");
		if (errorRegex.exactMatch(responseJson)) {

			// replace "\n" with real new lines.
			QString error = errorRegex.cap(1).replace("\\n", "\n");
			message.critical(
				w, "Error",
				tr("Login failed, an error occurred.\n\n%1").arg(error));

			return edition;
		}
	}

	message.critical(
		w, "Error",
		tr("Login failed, an error occurred.\n\nServer response:\n\n%1")
		.arg(responseJson));

	return edition;
}

QString WebClient::request(
	const QString& email,
	const QString& password)
{
	QStringList args("--login-auth");
	// hash password in case it contains interesting chars.
	QString credentials(email + ":" + hash(password) + "\n");
	return m_CoreInterface.run(args, credentials);
}
