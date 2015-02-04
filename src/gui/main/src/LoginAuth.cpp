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

#include "LoginAuth.h"

#include "LoginDialog.h"
#include "AppConfig.h"
#include "QUtility.h"
#include "LoginResult.h"
#include "EditionType.h"

#include <QProcess>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <stdexcept>

void LoginAuth::checkEditionType()
{
	int edition = Unknown;
	int result = doCheckEditionType(edition);
	m_pLoginDialog->setLoginResult(result);
	m_pLoginDialog->setEditionType(edition);
	emit finished();
}

int LoginAuth::doCheckEditionType(int& edition)
{
	QString responseJson;

	try
	{
		responseJson = request(m_Email, m_Password);
	}
	catch (std::exception& e)
	{
		m_pLoginDialog->setError(e.what());
		return ExceptionError;
	}

	QRegExp resultRegex(".*\"result\".*:.*(true|false).*");
	if (resultRegex.exactMatch(responseJson)) {
		QString boolString = resultRegex.cap(1);
		if (boolString == "true") {
			QRegExp editionRegex(".*\"edition\".*:.*\"(.+)\",.*");
			if (editionRegex.exactMatch(responseJson)) {
				QString e = editionRegex.cap(1);
				edition = e.toInt();
			}
			return Ok;
		}
		else if (boolString == "false") {
			return InvalidEmailPassword;
		}
	}
	else {
		QRegExp errorRegex(".*\"error\".*:.*\"(.+)\".*");
		if (errorRegex.exactMatch(responseJson)) {

			// replace "\n" with real new lines.
			QString error = errorRegex.cap(1).replace("\\n", "\n");
			m_pLoginDialog->setError(error);
			return Error;
		}
	}

	m_pLoginDialog->setError(responseJson);
	return ServerResponseError;
}

QString LoginAuth::request(const QString& email, const QString& password)
{
	QString program(QCoreApplication::applicationDirPath() + "/syntool");
	QStringList args("--login-auth");

	QProcess process;
	process.setReadChannel(QProcess::StandardOutput);
	process.start(program, args);
	bool success = process.waitForStarted();

	QString out, error;
	if (success)
	{
		// hash password in case it contains interesting chars.
		QString credentials(email + ":" + hash(password) + "\n");
		process.write(credentials.toStdString().c_str());

		if (process.waitForFinished()) {
			out = process.readAllStandardOutput();
			error = process.readAllStandardError();
		}
	}

	out = out.trimmed();
	error = error.trimmed();

	if (out.isEmpty() ||
		!error.isEmpty() ||
		!success ||
		process.exitCode() != 0)
	{
		throw std::runtime_error(
			QString("Code: %1\nError: %2")
				.arg(process.exitCode())
				.arg(error.isEmpty() ? "Unknown" : error)
				.toStdString());
	}

	return out;
}
