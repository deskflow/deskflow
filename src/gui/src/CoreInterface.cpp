/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#include "CoreInterface.h"

#include <QCoreApplication>
#include <QProcess>
#include <stdexcept>

static const char kCoreBinary[] = "syntool";

CoreInterface::CoreInterface()
{
}

QString CoreInterface::getPluginDir()
{
	QStringList args("--get-plugin-dir");
	return run(args);
}

QString CoreInterface::getProfileDir()
{
	QStringList args("--get-profile-dir");
	return run(args);
}

QString CoreInterface::getInstalledDir()
{
	QStringList args("--get-installed-dir");
	return run(args);
}

QString CoreInterface::getArch()
{
	QStringList args("--get-arch");
	return run(args);
}

QString CoreInterface::getSubscriptionFilename()
{
	QStringList args("--get-subscription-filename");
	return run(args);
}

QString CoreInterface::activateSerial(const QString& serial)
{
	QStringList args("--subscription-serial");
	args << serial;

	return run(args);
}

QString CoreInterface::checkSubscription()
{
	QStringList args("--check-subscription");
	return run(args);
}

QString CoreInterface::run(const QStringList& args, const QString& input)
{
	QString program(
		QCoreApplication::applicationDirPath()
		+ "/" + kCoreBinary);

	QProcess process;
	process.setReadChannel(QProcess::StandardOutput);
	process.start(program, args);
	bool success = process.waitForStarted();

	QString output, error;
	if (success)
	{
		if (!input.isEmpty()) {
			process.write(input.toStdString().c_str());
		}

		if (process.waitForFinished()) {
			output = process.readAllStandardOutput().trimmed();
			error = process.readAllStandardError().trimmed();
		}
	}

	int code = process.exitCode();
	if (!error.isEmpty() || !success || code != 0)
	{
		throw std::runtime_error(
			QString("Code: %1\nError: %2")
				.arg(process.exitCode())
				.arg(error.isEmpty() ? "Unknown" : error)
				.toStdString());
	}

	return output;
}
