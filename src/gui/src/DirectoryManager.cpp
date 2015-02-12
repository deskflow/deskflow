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

#include "DirectoryManager.h"

#include <QCoreApplication>
#include <QProcess>
#include <QMessageBox>
#include <QObject>

static const char kGetPluginDirArg[] = "--get-plugin-dir";
static const char kGetProfileDirArg[] = "--get-profile-dir";

DirectoryManager::DirectoryManager()
{
}

QString DirectoryManager::getPluginDir()
{
	QStringList args(kGetPluginDirArg);
	return getDirViaSyntool(args);
}

QString DirectoryManager::getProfileDir()
{
	QStringList args(kGetProfileDirArg);
	return getDirViaSyntool(args);
}

QString DirectoryManager::getDirViaSyntool(QStringList& args)
{
	QString program(QCoreApplication::applicationDirPath() + "/syntool");

	QProcess process;
	process.setReadChannel(QProcess::StandardOutput);
	process.start(program, args);
	bool success = process.waitForStarted();

	QString out, error;
	if (success)
	{
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
		QMessageBox::critical(
				NULL, QObject::tr("Synergy"),
			QObject::tr("An error occured while calling syntool "
			   "with the first arg %1. Code: %2\nError: %3")
			.arg(args.at(0))
			.arg(process.exitCode())
			.arg(error.isEmpty() ? "Unknown" : error));
		return "";
	}

	return out;
}
